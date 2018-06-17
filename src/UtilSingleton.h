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
                if (!m_pSington) {
                    std::lock_guard<std::mutex> gLock(m_Mutex);
                    if (nullptr == m_pSington) {
                        m_pSington = std::make_shared<T>(std::forward<Args>(args)...);
                    }
                }
                return m_pSington;
            }

        __attribute__ ((visibility ("default")))
        static void DesInstance(){
            if (m_pSington) {
                m_pSington.reset();
                m_pSington = nullptr;
            }
        }

    private:
        explicit UtilSingleton();
        UtilSingleton(const UtilSingleton&) = delete;
        UtilSingleton& operator=(const UtilSingleton&) = delete;
        ~UtilSingleton();

    private:
        static std::shared_ptr<T> m_pSington;
        static std::mutex m_Mutex;
};

template<typename T>
std::shared_ptr<T> UtilSingleton<T>::m_pSington = nullptr;

template<typename T>
std::mutex UtilSingleton<T>::m_Mutex;

}

#endif
