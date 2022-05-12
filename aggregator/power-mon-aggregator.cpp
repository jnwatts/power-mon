#include <chrono>
#include <thread>
#include <vector>

#include <mosquitto.h>
#include <mqtt_protocol.h>
#include <signal.h>
#include <stdio.h>

#include "database.h"
#include "collect.h"

using namespace std::chrono_literals;

bool g_run = false;

void handle_sigint(int sig)
{
    g_run = false;
    signal(SIGINT, SIG_DFL);
}

int main(int argc, char const *argv[])
{
    int result;
    Database db;
    Collect c;

    c.topic = "MQTT/TOPIC/PATH";

    result = db.begin();
    if (result) {
        printf("DB begin fail\n");
        return 1;
    }

    result = c.begin();
    if (result) {
        printf("C begin fail\n");
        return 1;
    }

    for (auto &[_, ch] : db.channels) {
        if (c.subscribe(ch)) {
            printf("subscribe failed\n");
            return 1;
        }
    }

    signal(SIGINT, handle_sigint);
    g_run = true;

    while (g_run) {
        db.poll();
        c.poll();
        std::vector<data> d = c.collect();
        if (d.size())
            db.push_data(d);
        std::this_thread::sleep_for(100ms);
    }

    c.end();
    db.end();

    return 0;
}
