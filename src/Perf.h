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
        start = get_time();
        LOG_I("PERF-%s: start", sTag.c_str());
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
        start = get_time();
        over = false;
        LOG_I("PERF-%s: restart", sTag.c_str());
    }

    void done()
    {
        if (!over)
        {
            auto end = get_time();
            LOG_I("PERF-%s: done, cost %.2f sec", sTag.c_str(), (end - start) / 1000.0);
            over = true;
        }
        else
        {
            LOG_I("PERF-%s: already done", sTag.c_str());
        }
    }
};
}
#endif
