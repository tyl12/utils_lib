#ifndef MARKER_H
#define MARKER_H

#include <functional>

using namespace std;

class Marker
{
    private:
        function<void(void)> mFunc;

    public:
        Marker(function<void(void)>&& begin, function<void(void)>&& end):
            mFunc(move(end))
        {
            begin();
        }

        Marker(function<void(void)>&& end):
            mFunc(move(end))
        { }

        virtual ~Marker()
        {
            if (mFunc)
                mFunc();
        }
};

#endif
