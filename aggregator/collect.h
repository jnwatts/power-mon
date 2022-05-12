#pragma once

#include <mutex>

#include <mosquitto.h>

#include "database.h"

class Collect
{
public:
    Collect();
    ~Collect();

    int begin(void);
    void end(void);
    int poll(void);

    int subscribe(channel &ch);
    std::vector<data> collect();

    std::string topic;

protected:
    mosquitto *mosq = nullptr;
    bool connected = false;
    std::map<std::string, channel> channels;
    std::vector<data> messages;
    std::mutex mutex;

    int mosq_connect(void);
    void mosq_disconnect(void);
    static void mosq_message_cb(struct mosquitto *mosq, void *self, const struct mosquitto_message *msg);
    static void mosq_log_cb(struct mosquitto *mosq, void *obj, int level, const char *str);
};