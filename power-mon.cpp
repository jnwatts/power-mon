#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>

#include "eic.h"

using namespace std::chrono_literals;

bool g_run = false;

void handle_sigint(int sig)
{
	g_run = false;
	signal(SIGINT, SIG_DFL);
}

int main(int argc, char *argv[])
{
	EIC::Comm eic;
	unsigned reg;
	unsigned val;
	int ret;

	signal(SIGINT, handle_sigint);
	g_run = true;

	eic.voltageGain = 3778;
	eic.currentGain1 = 2523; // Kitchen
	eic.currentGain2 = 3999; // House

	ret = eic.begin("/dev/spidev0.0");
	if (ret)
		return 1;

	ret = eic.init();
	if (ret)
		return 1;

	std::this_thread::sleep_for(1s);

	while (g_run) {
		double v[] = { eic.GetLineVoltage(0), eic.GetLineVoltage(2) };
		double c[] = { eic.GetLineCurrent(0), eic.GetLineCurrent(2) };
		double t = eic.GetTemperature();
		double f = eic.GetFrequency();
		printf("Voltage: %f/%f V\n", v[0], v[1]);
		printf("Current: %f/%f A\n", c[0], c[1]);
		printf("Temp: %f C\n", t);
		printf("Freq: %f Hz\n", f);
		std::this_thread::sleep_for(1s);
	}
	
	return 0;
}
