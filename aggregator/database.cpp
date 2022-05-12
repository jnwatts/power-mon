#define EXPAND_MY_SSQLS_STATICS

#include <stdio.h>

#include "database.h"

int Database::begin()
{
    int result;

    result = this->load_channels();
    if (result)
        return 1;

    return 0;
}

int Database::end()
{
    return 0;
}

int Database::poll()
{
    return 0;
}

mysqlpp::Connection *Database::conn()
{
    auto *conn = new mysqlpp::Connection(true);

    try {
        if (!conn->connect("DBNAME", "DBHOST", "DBUSER", "DBPASS")) {
            perror("Failed to connect");
            delete conn;
            return nullptr;
        }
    } catch (std::exception &e) {
        fprintf(stderr, "%s", e.what());
        delete conn;
        return nullptr;
    }

    return conn;
}

int Database::load_channels()
{
    int result = 0;
    auto *conn = this->conn();
    if (!conn)
        return 1;

    printf("Loading channels...\n");
    try {
        mysqlpp::Query query = conn->query("SELECT ch_id, name FROM channels");
        std::vector<channel> res;
        query.storein(res);
        for (auto row : res) {
            this->channels[row.name] = row;
            printf("\t%s\n", row.name.c_str());
        }
    } catch (std::exception &e) {
        fprintf(stderr, "%s", e.what());
        result = 1;
    }

    delete conn;
    return 0;
}

int Database::push_data(std::vector<data> &data)
{
    int result = 0;
    auto *conn = this->conn();
    if (!conn)
        return 1;

    try {
        auto query = conn->query();
        mysqlpp::Query::MaxPacketInsertPolicy<mysqlpp::NoTransaction> policy(1*1024*1024);
        query.insertfrom(data.begin(), data.end(), policy);
    } catch (std::exception &e) {
        fprintf(stderr, "%s", e.what());
        result = 1;
    }

    delete conn;
    return result;
}
