#include <fcntl.h>
#include <math.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/stat.h>

#include <linux/ioctl.h>
#include <linux/spi/spidev.h>
#include <linux/types.h>

#include "eic.h"

const unsigned SPI_SPEED_HZ = 200000;
const unsigned SPI_BITS_PER_WORD = 8;

EIC::Comm::~Comm(void)
{
	this->end();
}

int EIC::Comm::begin(std::string path)
{
	int ret;
	int fd = open(path.c_str(), O_RDWR);
	if (fd == 0)
		return 1;
	this->fd = fd;

	uint32_t mode = 0;
	ret = ioctl(fd, SPI_IOC_WR_MODE32, &mode);
	if (ret < 0)
		return ret;
	
	return 0;
}

void EIC::Comm::end(void)
{
	if (this->fd != 0)
		close(fd);
	this->fd = 0;
}

int EIC::Comm::read(unsigned reg, unsigned &val)
{
	int ret;
	uint8_t reg_buf[2];
	uint8_t val_buf[2] = {0,};

	// Register and READ bit
	reg_buf[0] = (reg >> 8) & 0x3;
	reg_buf[0] |= (1<<7);
	reg_buf[1] = reg;

	struct spi_ioc_transfer tr[] = {
		{
			.tx_buf = (uintptr_t)reg_buf,
			.len = 2,
			.speed_hz = SPI_SPEED_HZ,
			.delay_usecs = 4,
			.bits_per_word = SPI_BITS_PER_WORD,
			.cs_change = 0,
		},
		{
			.rx_buf = (uintptr_t)val_buf,
			.len = 2,
			.speed_hz = SPI_SPEED_HZ,
			.delay_usecs = 0,
			.bits_per_word = SPI_BITS_PER_WORD,
			.cs_change = 0,
		}
	};

	ret = ioctl(this->fd, SPI_IOC_MESSAGE(2), tr);
	if (ret < 2)
		return ret;

	// Value
	val = val_buf[0] << 8 | val_buf[1];
	
	return 0;
}

int EIC::Comm::write(unsigned reg, unsigned val)
{
	int ret;
	uint8_t reg_buf[2];
	uint8_t val_buf[2] = {0,};

	// Address
	reg_buf[0] = (reg >> 8) & 0x3;
	reg_buf[1] = reg;

	// Value
	val_buf[0] = val >> 8;
	val_buf[1] = val;

	struct spi_ioc_transfer tr[] = {
		{
			.tx_buf = (uintptr_t)reg_buf,
			.len = 2,
			.speed_hz = SPI_SPEED_HZ,
			.delay_usecs = 4,
			.bits_per_word = SPI_BITS_PER_WORD,
			.cs_change = 0,
		},
		{
			.tx_buf = (uintptr_t)val_buf,
			.len = 2,
			.speed_hz = SPI_SPEED_HZ,
			.delay_usecs = 0,
			.bits_per_word = SPI_BITS_PER_WORD,
			.cs_change = 0,
		}
	};

	ret = ioctl(this->fd, SPI_IOC_MESSAGE(2), tr);
	if (ret < 2)
		return ret;

	return 0;
}

int EIC::Comm::read32(unsigned reg_h, unsigned reg_l, unsigned &val)
{
	unsigned val_h, val_l;
	this->read(reg_h, val_h);
	this->read(reg_l, val_l);
	this->read(reg_h, val); // This seems superfluous... maybe required to clear something?
	val = (val_h << 16) | val_l;
	return 0;
}

int EIC::Comm::init(void)
{
	int ret;
	unsigned _lineFreq = this->lineFreq;
	unsigned _pgagain = this->pgaGain;
	unsigned _ugain = this->voltageGain;
	unsigned _igainA = this->currentGain1;
	unsigned _igainB = 0; // Unused on single split-phase
	unsigned _igainC = this->currentGain1;
	unsigned vSagTh;
	unsigned sagV;
	unsigned FreqHiThresh;
	unsigned FreqLoThresh;

	if (_lineFreq == LINE_FREQ_60HZ) {
		sagV = 90;
		FreqHiThresh = 61 * 100;
		FreqLoThresh = 59 * 100;
	} else {
		sagV = 190;
		FreqHiThresh = 51 * 100;
		FreqLoThresh = 49 * 100;
	}
	vSagTh = (sagV * 100 * sqrt(2)) / (2 * _ugain / 0x8000);

	//Initialize registers
	ret = this->write(SoftReset, 0x789A);     // 70 Perform soft reset
	if (ret)
		return ret;
	//Assume rest will succeed...
	this->write(CfgRegAccEn, 0x55AA);   // 7F enable register config access
	this->write(MeterEn, 0x0001);       // 00 Enable Metering

	this->write(SagPeakDetCfg, 0x143F); // 05 Sag and Voltage peak detect period set to 20ms
	this->write(SagTh, vSagTh);         // 08 Voltage sag threshold
	this->write(FreqHiTh, FreqHiThresh);  // 0D High frequency threshold
	this->write(FreqLoTh, FreqLoThresh);  // 0C Lo frequency threshold
	this->write(EMMIntEn0, 0xB76F);     // 75 Enable interrupts
	this->write(EMMIntEn1, 0xDDFD);     // 76 Enable interrupts
	this->write(EMMIntState0, 0x0001);  // 73 Clear interrupt flags
	this->write(EMMIntState1, 0x0001);  // 74 Clear interrupt flags
	this->write(ZXConfig, 0xD654);      // 07 ZX2, ZX1, ZX0 pin config - set to current channels, all polarity

	//Set metering config values (CONFIG)
	this->write(PLconstH, 0x0861);    // 31 PL Constant MSB (default) - Meter Constant = 3200 - PL Constant = 140625000
	this->write(PLconstL, 0xC468);    // 32 PL Constant LSB (default) - this is 4C68 in the application note, which is incorrect
	this->write(MMode0, _lineFreq);   // 33 Mode Config (frequency set in main program)
	this->write(MMode1, _pgagain);    // 34 PGA Gain Configuration for Current Channels - 0x002A (x4) // 0x0015 (x2) // 0x0000 (1x)
	this->write(PStartTh, 0x1D4C);    // 35 All phase Active Startup Power Threshold - 50% of startup current = 0.02A/0.00032 = 7500
	this->write(QStartTh, 0x1D4C);    // 36 All phase Reactive Startup Power Threshold
	this->write(SStartTh, 0x1D4C);    // 37 All phase Apparent Startup Power Threshold
	this->write(PPhaseTh, 0x02EE);    // 38 Each phase Active Phase Threshold = 10% of startup current = 0.002A/0.00032 = 750
	this->write(QPhaseTh, 0x02EE);    // 39 Each phase Reactive Phase Threshold
	this->write(SPhaseTh, 0x02EE);    // 3A Each phase Apparent Phase Threshold

	//Set metering calibration values (CALIBRATION)
	this->write(PQGainA, 0x0000);     // 47 Line calibration gain
	this->write(PhiA, 0x0000);        // 48 Line calibration angle
	this->write(PQGainB, 0x0000);     // 49 Line calibration gain
	this->write(PhiB, 0x0000);        // 4A Line calibration angle
	this->write(PQGainC, 0x0000);     // 4B Line calibration gain
	this->write(PhiC, 0x0000);        // 4C Line calibration angle
	this->write(PoffsetA, 0x0000);    // 41 A line active power offset FFDC
	this->write(QoffsetA, 0x0000);    // 42 A line reactive power offset
	this->write(PoffsetB, 0x0000);    // 43 B line active power offset
	this->write(QoffsetB, 0x0000);    // 44 B line reactive power offset
	this->write(PoffsetC, 0x0000);    // 45 C line active power offset
	this->write(QoffsetC, 0x0000);    // 46 C line reactive power offset

	//Set metering calibration values (HARMONIC)
	this->write(POffsetAF, 0x0000);   // 51 A Fund. active power offset
	this->write(POffsetBF, 0x0000);   // 52 B Fund. active power offset
	this->write(POffsetCF, 0x0000);   // 53 C Fund. active power offset
	this->write(PGainAF, 0x0000);     // 54 A Fund. active power gain
	this->write(PGainBF, 0x0000);     // 55 B Fund. active power gain
	this->write(PGainCF, 0x0000);     // 56 C Fund. active power gain

	//Set measurement calibration values (ADJUST)
	this->write(UgainA, _ugain);      // 61 A Voltage rms gain
	this->write(IgainA, _igainA);     // 62 A line current gain
	this->write(UoffsetA, 0x0000);    // 63 A Voltage offset - 61A8
	this->write(IoffsetA, 0x0000);    // 64 A line current offset - FE60
	this->write(UgainB, _ugain);      // 65 B Voltage rms gain
	this->write(IgainB, _igainB);     // 66 B line current gain
	this->write(UoffsetB, 0x0000);    // 67 B Voltage offset - 1D4C
	this->write(IoffsetB, 0x0000);    // 68 B line current offset - FE60
	this->write(UgainC, _ugain);      // 69 C Voltage rms gain
	this->write(IgainC, _igainC);     // 6A C line current gain
	this->write(UoffsetC, 0x0000);    // 6B C Voltage offset - 1D4C
	this->write(IoffsetC, 0x0000);    // 6C C line current offset

	this->write(CfgRegAccEn, 0x0000); // 7F end configuration

	return 0;
}

double EIC::Comm::GetLineVoltage(int ch)
{
	unsigned val;
	unsigned reg;
	if (ch == 0)
		reg = EIC::UrmsA;
	else if (ch == 1)
		reg = EIC::UrmsB;
	else if (ch == 2)
		reg = EIC::UrmsC;
	else
		return 1;
	this->read(reg, val);
	return (double)val / 100.0;
}

double EIC::Comm::GetLineCurrent(int ch)
{
	unsigned val;
	unsigned reg;
	if (ch == 0)
		reg = EIC::IrmsA;
	else if (ch == 1)
		reg = EIC::IrmsB;
	else if (ch == 2)
		reg = EIC::IrmsC;
	else
		return 1;
	this->read(reg, val);
	return (double)val / 1000.0;
}

double EIC::Comm::GetTemperature(void)
{
	unsigned val;
	this->read(EIC::Temp, val);
	if (val & 0x8000)
	 	return -1.0 * (double)val;
	return (double)val;
}

double EIC::Comm::GetFrequency(void)
{
	unsigned val;
	this->read(EIC::Freq, val);
	return (double)val / 100.0;
}
