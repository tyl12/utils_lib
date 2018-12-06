#ifndef THREAD_GUARD_H
#define THREAD_GUARD_H

#include <thread>
#include <iostream>

using namespace std;

namespace utils{
class thread_guard{
    private:
        thread mThread;
    public:
        explicit thread_guard(thread&& t): mThread(move(t)){
            cout<<__PRETTY_FUNCTION__<<endl;
        }
        thread_guard(thread_guard&& o) noexcept: mThread(move(o.mThread)){
            cout<<__PRETTY_FUNCTION__<<endl;
        }
        thread_guard& operator=(thread_guard&& o) noexcept{
            cout<<__PRETTY_FUNCTION__<<endl;
            mThread=move(o.mThread);
            return *this;
        }
        ~thread_guard(){
            cout<<__PRETTY_FUNCTION__<<endl;
            join();
        }
        bool joinable(){
            return mThread.joinable();
        }
        void join(){
            if (mThread.joinable())
                mThread.join();
        }
        thread_guard(thread& t) = delete;
        thread_guard(const thread& t) = delete;
        thread_guard& operator=(thread& t) = delete;
};
}

#endif
