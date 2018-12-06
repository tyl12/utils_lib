//UtilSingleton.h

#ifndef UTIL_SINGLETON_H
#define UTIL_SINGLETON_H

#include <mutex>
#include <memory>

namespace utils{

template<typename T>
class  __attribute__ ((visibility ("default"))) UtilSingleton {
    public:
        template<typename ...Args>
        __attribute__ ((visibility ("default")))
        static std::shared_ptr<T> GetInstance(Args&&... args){
#if 1
            std::call_once(mOnceFlag, [&args...](){
                m_pSington = std::make_shared<T>(std::forward<Args>(args)...);
            });
#else
            std::lock_guard<std::mutex> _l(m_Mutex);
            if (!m_pSington) {
                m_pSington = std::make_shared<T>(std::forward<Args>(args)...);
            }
#endif
            return m_pSington;
        }
#if 0
        __attribute__ ((visibility ("default")))
        static void DesInstance(){
            std::lock_guard<std::mutex> _l(m_Mutex);
            if (m_pSington) {
                m_pSington.reset();
                m_pSington = nullptr;
            }
        }
#endif

    private:
        explicit UtilSingleton();
        UtilSingleton(const UtilSingleton&) = delete;
        UtilSingleton& operator=(const UtilSingleton&) = delete;
        ~UtilSingleton();

    private:
        static std::shared_ptr<T> m_pSington;
        static std::once_flag mOnceFlag;
        static std::mutex m_Mutex;
};

template<typename T>
std::shared_ptr<T> UtilSingleton<T>::m_pSington = nullptr;

template<typename T>
std::once_flag UtilSingleton<T>::mOnceFlag;

template<typename T>
std::mutex UtilSingleton<T>::m_Mutex;

}

#endif
