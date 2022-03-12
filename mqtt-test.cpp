#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>

#include "report.h"
#include "time.h"

using namespace power;

bool g_run = false;

void handle_sigint(int sig)
{
	g_run = false;
	signal(SIGINT, SIG_DFL);
}

int main(int argc, char *argv[])
{
	Report report;
	signal(SIGINT, handle_sigint);

	report.channels = {
		{.topic = "jwatts/power/kitchen" },
		{.topic = "jwatts/power/house" },
	};

	printf("Connect...\n");
	report.begin();

	report.channels[0].push({
		power::clock_t::now(),
		1,
		1.234,
		1.234*1.234,
	});
	report.channels[0].push({
		power::clock_t::now(),
		2,
		1.234,
		1.234*1.234,
	});
	report.channels[0].push({
		power::clock_t::now(),
		3,
		1.234,
		1.234*1.234,
	});
	report.channels[1].push({
		power::clock_t::now(),
		4,
		1.234,
		1.234*1.234,
	});
	report.channels[1].push({
		power::clock_t::now(),
		5,
		1.234,
		1.234*1.234,
	});

	printf("Loops start\n");
	g_run = true;
	constexpr std::chrono::milliseconds TICK_PERIOD = 1000ms;
	power::timestamp_t next_tick = power::clock_t::now();
	int n = 10;
	while (g_run) {
		if (power::clock_t::now() >= next_tick) {
			printf("power tick\n");
			power::timestamp_t timestamp = power::clock_t::now();
			double v[] = { n, n };
			double c[] = { n, n };
			n++;
			report.channels[0].push({timestamp, v[0], c[0], v[0]*c[0]});
			report.channels[1].push({timestamp, v[1], c[1], v[1]*c[1]});
			while (next_tick < power::clock_t::now())
				next_tick += TICK_PERIOD;
		}
		report.poll();
		std::this_thread::sleep_for(100ms);
	}

	printf("End...\n");
	report.end();

	printf("Exit\n");
	return 0;
}
