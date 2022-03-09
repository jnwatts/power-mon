#pragma once

#include <string>
#include "eic_registers.h"
#include "eic_constants.h"

namespace EIC {
class Comm {
public:
	Comm() = default;
	~Comm();

	int begin(std::string path);
	void end(void);

	int read(unsigned reg, unsigned &val);
	int write(unsigned reg, unsigned val);

	int read32(unsigned reg_h, unsigned reg_l, unsigned &val);

	int init(void);

	double GetLineVoltage(int ch);
	double GetLineCurrent(int ch);
	double GetTemperature(void);
	double GetFrequency(void);

	int fd = 0;

	unsigned lineFreq = EIC::LINE_FREQ_60HZ;
	unsigned pgaGain = EIC::PGA_GAIN_2X;
	unsigned voltageGain = EIC::VOLTAGE_GAIN_JAMECO_157041;
	unsigned currentGain1 = EIC::CURRENT_GAIN_SCT_013_030;
	unsigned currentGain2 = EIC::CURRENT_GAIN_SCT_013_030;
};
}
