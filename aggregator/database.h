#pragma once
#if !defined(EXPAND_MY_SSQLS_STATICS)
#   define MYSQLPP_SSQLS_NO_STATICS
#endif

#include <vector>

#include <mysql++.h>
#include <ssqls.h>

sql_create_2(channel, 1, 0,
    mysqlpp::sql_int, ch_id,
    mysqlpp::sql_text, name)

sql_create_5(data, 1, 5,
    mysqlpp::sql_int, ch_id,
    mysqlpp::sql_bigint, timestamp,
    mysqlpp::sql_float, voltage,
    mysqlpp::sql_float, current,
    mysqlpp::sql_float, power)

class Database
{
public:
    Database() = default;
    ~Database() = default;

    int begin();
    int end();
    int poll();

    mysqlpp::Connection *conn();
    int load_channels();
    int push_data(std::vector<data> &data);

    std::map<std::string, channel> channels;
};