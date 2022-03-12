#pragma once

#include <list>
#include <numeric>
#include <vector>
#include <string>

#include <mosquitto.h>

#include "time.h"

namespace power {

class Report
{
public:
    struct reading_t {
        timestamp_t timestamp;
        float voltage;
        float current;
        float power;
    };

    struct channel_t {
        std::string topic;
        std::list<reading_t> readings;
        timestamp_t latest_reading;

        void push(reading_t reading);
    };

    Report();
    ~Report();

    int begin(void);
    void end(void);
    int poll(void);

    std::string serialize(reading_t &r);

    std::vector<channel_t> channels;
    std::chrono::milliseconds publish_period = std::chrono::milliseconds(5000);
    std::chrono::hours reading_expiration = std::chrono::hours(24);
    int readings_per_message = 100;
    int messages_per_publish = 5;

protected:
    timestamp_t next_publish;

    mosquitto *mosq = nullptr;
    bool connected = false;

    void publish_channel(channel_t &ch);

    int mosq_connect(void);
    void mosq_disconnect(void);
    int mosq_publish_message(std::string topic, std::string payload, bool retain = false);
    static void mosq_log_cb(struct mosquitto *mosq, void *obj, int level, const char *str);
};

}