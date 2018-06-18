#ifndef PERF_H
#define PERF_H

#include <cstdlib>
#include <string>
#include "utils.h"

using namespace std;

namespace utils{
class perf
{
private:
    bool over;
    string sTag;
    int64_t start;
    perf() = default;
    perf(const perf&) {}
public:
    perf(string tag)
    {
        over = false;
        sTag = tag;
        start = get_time_ms();
        printf("PERF-%s: start\n", sTag.c_str());
    }

    virtual ~perf()
    {
        if (!over)
        {
            done();
        }
    }

    void reset()
    {
        start = get_time_ms();
        over = false;
        printf("PERF-%s: restart\n", sTag.c_str());
    }

    void done()
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
};
}
#endif
