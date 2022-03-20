#pragma once

#include <chrono>

using namespace std::chrono_literals;

namespace power {

// TODO: Upgrade to utc_clock when support becomes available
using clock_t = std::chrono::system_clock;
using timestamp_t = std::chrono::time_point<clock_t>;

}