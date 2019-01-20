#ifndef PERF_H
#define PERF_H

#include <cstdlib>
#include <string>

using namespace std;

namespace utils{

class Perf
{
    private:
        bool over;
        string sTag;
        int64_t start;
        Perf(const Perf&) {}
    public:
        Perf() = default;

        Perf(string tag);
        virtual ~Perf();
        void reset();
        void done();
};
}
#endif
