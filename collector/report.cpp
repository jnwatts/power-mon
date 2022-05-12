#include <assert.h>
#include <chrono>
#include <errno.h>
#include <string.h>
#include <mqtt_protocol.h>

#include "report.h"

using namespace power;

static int g_mosquitto_users = 0;

Report::Report()
{
    if (g_mosquitto_users <= 0)
        mosquitto_lib_init();
    g_mosquitto_users++;
}

Report::~Report()
{
    this->end();
    if (g_mosquitto_users == 1)
        mosquitto_lib_cleanup();
    g_mosquitto_users--;
}

int Report::begin(void)
{
    int result;
    this->mosq = mosquitto_new("MQTTUSERNAME-1", false, this);
    mosquitto_int_option(this->mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);
    mosquitto_int_option(this->mosq, MOSQ_OPT_TLS_USE_OS_CERTS, 1);
    mosquitto_int_option(this->mosq, MOSQ_OPT_SSL_CTX_WITH_DEFAULTS, 0);
    mosquitto_reconnect_delay_set(this->mosq, 1, 5, false);
    mosquitto_log_callback_set(this->mosq, Report::mosq_log_cb);
    mosquitto_username_pw_set(this->mosq, "MQTTUSERNAME", "MQTTPASSWORD");

    result = mosquitto_loop_start(this->mosq);
    if (result) {
        printf("loop start failed\n");
        return result;
    }

    this->mosq_connect();
    this->next_publish = clock_t::now() + this->publish_period;
    return 0;
}

void Report::end(void)
{
    if (this->mosq) {
        this->mosq_disconnect();
        mosquitto_loop_stop(this->mosq, true);
        mosquitto_destroy(this->mosq);
    }
    this->mosq = nullptr;
}

int Report::poll(void)
{
    if (!this->mosq)
        return 0;

    if (clock_t::now() >= this->next_publish) {
        printf("report tick\n");
        for (auto &ch : this->channels)
            this->publish_channel(ch);
        while (this->next_publish < clock_t::now())
            this->next_publish += this->publish_period;
    }
    return 0;
}

void Report::channel_t::push(reading_t reading)
{
    this->readings.push_back(reading);
    //TODO: Trim expired readings
}

std::string Report::serialize(reading_t &r) {
    char buf[1024];
    size_t len = snprintf(buf, sizeof(buf),
        "{\"timestamp\": \"%s\","
        "\"voltage\": %f,"
        "\"current\": %f,"
        "\"power\": %f}",
        std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(r.timestamp.time_since_epoch()).count()).c_str(),
        r.voltage,
        r.current,
        r.power);
    return {buf, len};
}

void Report::publish_channel(channel_t &ch)
{
    int result;

    if (ch.readings.empty())
        return;

    reading_t &last = ch.readings.back();
    if (last.timestamp > ch.latest_reading) {
        result = this->mosq_publish_message(ch.topic + "/latest", this->serialize(last), true);
        if (result)
            return;
        ch.latest_reading = last.timestamp;
    }

    int n_messages = 0;
    while (ch.readings.size() > 0) {
        if (n_messages >= this->messages_per_publish)
            break;

        std::string msg = "[";
        int n_readings = 0;
        auto iter = ch.readings.rbegin();
        for (; iter != ch.readings.rend(); iter++) {
            if (n_readings >= this->readings_per_message)
                break;
            if (n_readings > 0)
                msg += ",";
            msg += this->serialize(*iter);
            n_readings++;
        }
        msg += "]";

        result = this->mosq_publish_message(ch.topic + "/all", msg, false);
        if (result)
            return;

        if (n_readings > 0 && iter != ch.readings.rend()) {
            ch.readings.erase(iter.base(), ch.readings.end());
        } else if (iter == ch.readings.rend()) {
            ch.readings.clear();
        }

        n_messages++;
    }
}

int Report::mosq_connect(void)
{
    int result;

    // mosquitto_int_option(this->mosq, MOSQ_OPT_SSL_CTX_WITH_DEFAULTS, 0);

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

void Report::mosq_disconnect(void)
{
    mosquitto_disconnect(this->mosq);
}

int Report::mosq_publish_message(std::string topic, std::string payload, bool retain)
{
    int result;

    mosquitto_property *publish_props = nullptr;

    result = mosquitto_property_add_int32(&publish_props, MQTT_PROP_MESSAGE_EXPIRY_INTERVAL, std::chrono::seconds(this->reading_expiration).count());
    if (result)
        return result;

    result = mosquitto_publish_v5(this->mosq, nullptr, topic.c_str(), payload.size(), payload.c_str(), 2, retain, publish_props);
    if (result) {
        // if (result == MOSQ_ERR_NO_CONN)
            // this->mosq_disconnect();
        return result;
    }

    return 0;
}

void Report::mosq_log_cb(struct mosquitto *mosq, void *obj, int level, const char *str)
{
    (void)(mosq);
    (void)(obj);

    printf("MQTT [%d] %s\n", level, str);
}
