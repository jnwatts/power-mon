#include <assert.h>
#include <chrono>
#include <errno.h>
#include <string.h>
#include <mqtt_protocol.h>
#include <cJSON.h>

#include "collect.h"

#ifdef DEBUG
#define DEBUG_PRINTF printf
#else
#define DEBUG_PRINTF(...) (void)(0)
#endif

static int g_mosquitto_users = 0;

Collect::Collect()
{
    if (g_mosquitto_users <= 0)
        mosquitto_lib_init();
    g_mosquitto_users++;
}

Collect::~Collect()
{
    this->end();
    if (g_mosquitto_users == 1)
        mosquitto_lib_cleanup();
    g_mosquitto_users--;
}

int Collect::begin(void)
{
    //TODO: Consider mosquitto_subscribe_simple()?


    int result;
    this->mosq = mosquitto_new("MQTTCLIENTID", false, this);
    mosquitto_int_option(this->mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
    mosquitto_int_option(this->mosq, MOSQ_OPT_TLS_USE_OS_CERTS, 1);
    mosquitto_int_option(this->mosq, MOSQ_OPT_SSL_CTX_WITH_DEFAULTS, 0);
    mosquitto_int_option(this->mosq, MOSQ_OPT_RECEIVE_MAXIMUM, 10000);
    mosquitto_int_option(this->mosq, MOSQ_OPT_SEND_MAXIMUM, 10000);
    mosquitto_reconnect_delay_set(this->mosq, 1, 5, false);
    mosquitto_log_callback_set(this->mosq, Collect::mosq_log_cb);
    mosquitto_message_callback_set(this->mosq, Collect::mosq_message_cb);
    mosquitto_username_pw_set(this->mosq, "MQTTUSERNAME", "MQTTPASSWORD");

    result = mosquitto_loop_start(this->mosq);
    if (result) {
        printf("loop start failed\n");
        return result;
    }

    this->mosq_connect();

    return 0;
}

void Collect::end(void)
{
    if (this->mosq) {
        this->mosq_disconnect();
        mosquitto_loop_stop(this->mosq, true);
        mosquitto_destroy(this->mosq);
    }
    this->mosq = nullptr;
}

int Collect::poll(void)
{
    return 0;
}

int Collect::subscribe(channel &ch)
{
    int result;
    std::string topic;

    this->channels[ch.name] = ch;

    // topic = this->topic + "/" + ch.name + "/latest";
    // printf("Subscribe %s\n", topic.c_str());
    // result = mosquitto_subscribe(this->mosq, nullptr, topic.c_str(), 2);
    // if (result) {
    //     printf("Failed to subscribe to %s", topic.c_str());
    //     return 1;
    // }

    topic = this->topic + "/" + ch.name + "/all";
    result = mosquitto_subscribe(this->mosq, nullptr, topic.c_str(), 2);
    if (result) {
        printf("Failed to subscribe to %s", topic.c_str());
        return 1;
    }

    return 0;
}

std::vector<data> Collect::collect()
{
    const std::lock_guard<std::mutex> lock(this->mutex);
    std::vector<data> messages = std::move(this->messages);
    return messages;
}

int Collect::mosq_connect(void)
{
    int result;

    mosquitto_property *connect_props = nullptr;

    result = mosquitto_property_add_int32(&connect_props, MQTT_PROP_SESSION_EXPIRY_INTERVAL, 3600);
    if (result) {
        printf("prop failed\n");
        return result;
    }

    result = mosquitto_connect_bind_v5(this->mosq, "push.sroz.net", 8883, 60, nullptr, connect_props);
    if (result) {
        if (result == MOSQ_ERR_ERRNO) {
            fprintf(stderr, "Error: %s\n", strerror(errno));
        } else {
            fprintf(stderr, "Unable to connect (%s).\n", mosquitto_strerror(result));
        }
        printf("connect failed\n");
        return result;
    }

    return 0;
}

void Collect::mosq_disconnect(void)
{
    mosquitto_disconnect(this->mosq);
}

void Collect::mosq_message_cb(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    auto *self = (Collect*)obj;
    std::string topic = msg->topic;
    std::string channel = topic.substr(self->topic.size() + 1);
    channel = channel.substr(0, channel.find("/"));
    int ch_id = self->channels[channel].ch_id;

    auto parse_reading = [self, &ch_id](cJSON *o, data &d) -> int {
        cJSON *i;
        d.ch_id = ch_id;

        i = cJSON_GetObjectItem(o, "timestamp");
        if (!cJSON_IsString(i)) {
            printf("Key error: timestamp\n");
            return 1;
        }
        d.timestamp = std::stoull(cJSON_GetStringValue(i));

        i = cJSON_GetObjectItem(o, "voltage");
        if (!cJSON_IsNumber(i)) {
            printf("Key error: voltage\n");
            return 1;
        }
        d.voltage = cJSON_GetNumberValue(i);

        i = cJSON_GetObjectItem(o, "current");
        if (!cJSON_IsNumber(i)) {
            printf("Key error: current\n");
            return 1;
        }
        d.current = cJSON_GetNumberValue(i);

        i = cJSON_GetObjectItem(o, "power");
        if (!cJSON_IsNumber(i)) {
            printf("Key error: power\n");
            return 1;
        }
        d.power = cJSON_GetNumberValue(i);

        return 0;
    };

    const std::lock_guard<std::mutex> lock(self->mutex);

    DEBUG_PRINTF("m%d T:%s (C:%s) %s\n", msg->mid, topic.c_str(), channel.c_str(), std::string((char*)msg->payload, msg->payloadlen).c_str());
    cJSON *j = cJSON_ParseWithLength((char*)msg->payload, msg->payloadlen);
    if (!j) {
        printf("Parse error\n");
        return;
    }
    if (topic.find("/latest") != std::string::npos) {
        data d;
        if (!cJSON_IsObject(j))
            return;
        if (parse_reading(j, d) == 0) {
            self->messages.push_back(d);
        } else {
            printf("Parse failed: %s\n", cJSON_PrintUnformatted(j));
        }
    } else if (topic.find("/all") != std::string::npos) {
        cJSON *o;
        if (!cJSON_IsArray(j))
            return;
        cJSON_ArrayForEach(o, j) {
            data d;
            if (parse_reading(o, d) == 0) {
                self->messages.push_back(d);
            } else {
                printf("Parse failed: %s\n", cJSON_PrintUnformatted(o));
            }
        }
    }
    cJSON_Delete(j);
}

void Collect::mosq_log_cb(struct mosquitto *mosq, void *obj, int level, const char *str)
{
    (void)(mosq);
    (void)(obj);

    DEBUG_PRINTF("MQTT [%d] %s\n", level, str);
}
