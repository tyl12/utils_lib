#include <cstdlib>
#include <chrono>
#include <string>
#include "Perf.h"

namespace utils{

static int64_t get_time_ms()
{
    using namespace std::chrono;
    auto now = system_clock::now();
    auto now_millis = time_point_cast<milliseconds>(now);
    auto value = now_millis.time_since_epoch();

    return value.count();
}

Perf::Perf(string tag)
{
    over = false;
    sTag = tag;
    start = get_time_ms();
    printf("PERF-%s: start\n", sTag.c_str());
}

Perf::~Perf()
{
    if (!over)
    {
        done();
    }
}

void Perf::reset()
{
    start = get_time_ms();
    over = false;
    printf("PERF-%s: restart\n", sTag.c_str());
}

void Perf::done()
{
    if (!over)
    {
        auto end = get_time_ms();
        printf("PERF-%s: done, cost %.2f sec\n", sTag.c_str(), (end - start) / 1000.0);
        over = true;
    }
    else
    {
        printf("PERF-%s: already done\n", sTag.c_str());
    }
}
}
