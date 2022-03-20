#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>

#include "eic.h"
#include "report.h"
#include "time.h"

using namespace power;

bool g_run = false;

constexpr std::chrono::milliseconds TICK_PERIOD = 1000ms;

void handle_sigint(int sig)
{
	g_run = false;
	signal(SIGINT, SIG_DFL);
}

int main(int argc, char *argv[])
{
	EIC::Comm eic;
	Report report;
	int ret;

	report.channels = {
		{.topic = "MQTT/TOPIC/PATH/kitchen"},
		{.topic = "MQTT/TOPIC/PATH/house"},
	};
	Report::channel_t &kitchen = report.channels[0];
	Report::channel_t &house = report.channels[1];
	if (report.begin()) {
		fprintf(stderr, "Failed to initialize report\n");
		return 1;
	}

	signal(SIGINT, handle_sigint);
	g_run = true;

	eic.voltageGain = 3778;
	eic.currentGain1 = 2523; // Kitchen
	eic.currentGain2 = 3999; // House

	ret = eic.begin("/dev/spidev0.0");
	if (ret) {
		fprintf(stderr, "Failed to begin eic\n");
		return 1;
	}

	ret = eic.init();
	if (ret) {
		fprintf(stderr, "Failed to init eic\n");
		return 1;
	}

	power::timestamp_t next_tick = power::clock_t::now();

	while (g_run) {
		if (power::clock_t::now() >= next_tick) {
			printf("power tick\n");
			power::timestamp_t timestamp = power::clock_t::now();
			double v[] = { eic.GetLineVoltage(0), eic.GetLineVoltage(2) };
			double c[] = { eic.GetLineCurrent(0), eic.GetLineCurrent(2) };
			// double t = eic.GetTemperature();
			// double f = eic.GetFrequency();
			// printf("Voltage: %f/%f V\n", v[0], v[1]);
			// printf("Current: %f/%f A\n", c[0], c[1]);
			// printf("Temp: %f C\n", t);
			// printf("Freq: %f Hz\n", f);
			kitchen.push({timestamp, v[0], c[0], v[0]*c[0]});
			house.push({timestamp, v[1], c[1], v[1]*c[1]});
			while (next_tick < power::clock_t::now())
				next_tick += TICK_PERIOD;
		}
		report.poll();
		std::this_thread::sleep_for(100ms);
	}

	report.end();

	return 0;
}
