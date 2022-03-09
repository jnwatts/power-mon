#pragma once

namespace EIC {
enum {
	LINE_FREQ_60HZ = (1<<12|1<<8|1<<7|1<<2|1<<0),
	LINE_FREQ_50HZ = (1<<8|1<<7|1<<2|1<<0),
};
enum {
	PGA_GAIN_1X = 0x0,
	PGA_GAIN_2X = 0x15,
	PGA_GAIN_4X = 0x2A,
};
enum {
	VOLTAGE_GAIN_JAMECO_157041 = 37106,
};
enum {
	CURRENT_GAIN_SCT_013_030 = 39473, // blindly using SCT-013-000 100A/50mA, but I have 30A/1v
};
}
