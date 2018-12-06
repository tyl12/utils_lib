
#ifndef TEST_H
#define TEST_H
#include <mutex>
#include <memory>

namespace tttt{

template<typename T>
class __attribute__ ((visibility("default"))) testclass{
    public:
        __attribute__ ((visibility ("default")))
        void test();

        static std::shared_ptr<T> get();
    private:
        static std::shared_ptr<T> m_pSington;
        static std::mutex m_Mutex;
};

__attribute__ ((visibility("default")))
void testfunc();
}

#endif
