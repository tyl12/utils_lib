#ifndef SYNC_QUEUE_H
#define SYNC_QUEUE_H

#include <queue>
#include <thread>
#include <mutex>
#include <iostream>
#include <condition_variable>

template <typename T, size_t N=0>
class SyncQueue final
{
    public:

        SyncQueue():
            _max_capability(N)
        {
            std::cout<<__FUNCTION__<<":"<<_max_capability<<std::endl;
        }
        bool reset(){
            {
                std::unique_lock<std::mutex> mlock(_mutex);
                while (!_queue.empty())
                    _queue.pop();
            }
            _cond.notify_all();
            return true;
        }

        ~SyncQueue(){
            reset();
        }

        size_t size(){
            std::unique_lock<std::mutex> mlock(_mutex);
            return _queue.size();
        }

        bool pop(T& item)
        {
            std::unique_lock<std::mutex> mlock(_mutex);
            _cond.wait(mlock, [this](){ return !_is_empty(); });
            item = _queue.front();
            _queue.pop();
            //std::cout<<__FUNCTION__<<":pop "<<item<<std::endl;
            _cond.notify_one();
            return true;
        }

        bool try_pop(T& item){
            {
                std::unique_lock<std::mutex> mlock(_mutex);
                if (_is_empty()){
                    std::cout<<__FUNCTION__<<": queue is empty"<<std::endl;
                    return false;
                }
                item = _queue.front();
                _queue.pop();
            }
            _cond.notify_one();
            return true;
        }

        template <typename... Args>
        bool push(Args&&... args)
        {
            {
                std::unique_lock<std::mutex> mlock(_mutex);
                _cond.wait(mlock, [this](){ return !_is_full();});
                _queue.push(std::forward<Args>(args)...);
                std::cout<<__FUNCTION__<<":push ";
                int s[] = {(std::cout<<args<<"," , 0)...};
                std::cout<<std::endl;
            }
            _cond.notify_one();
            return true;
        }

        bool try_push(T&& item)
        {
            {
                std::unique_lock<std::mutex> mlock(_mutex);
                if (_is_full()){
                    std::cout<<__FUNCTION__<<": queue is full"<<std::endl;
                    return false;
                }
                _queue.push(std::forward<T>(item));
            }
            _cond.notify_one();
            return true;
        }

        bool is_full(){
            std::unique_lock<std::mutex> mlock(_mutex);
            return _is_full();
        }

        bool is_empty(){
            std::unique_lock<std::mutex> mlock(_mutex);
            return _is_empty();
        }

    private:
        bool _is_full(){
            if (_max_capability > 0 && _queue.size() == _max_capability) return true;
            return false;
        }

        bool _is_empty(){
            return (_queue.size() == 0)? true: false;
        }

        std::queue<T> _queue;
        std::mutex _mutex;
        std::condition_variable _cond;
        size_t _max_capability;
};

#endif //SYNC_QUEUE_H
