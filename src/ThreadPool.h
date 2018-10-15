#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include<iostream>
#include<functional>
#include<thread>
#include<condition_variable>
#include<future>
#include<atomic>
#include<vector>
#include<queue>

// 命名空间
namespace MySpace {
    class ThreadPool;
}

class MySpace::ThreadPool{
    using Task= std::function<void()>;
    Task EmptyTask = Task([](){});
private:
    // 线程池
    std::vector<std::thread> pool;
    // 任务队列
    std::queue<Task> tasks;
    // 同步
    std::mutex m_task;
    std::condition_variable cv_task;
    // 是否关闭提交
    std::atomic<bool> _exit;
public:
    // 构造
    ThreadPool(size_t NumThreads = 0): _exit {false}{
        if (NumThreads < 1){
            NumThreads = std::thread::hardware_concurrency();
        }
        NumThreads = NumThreads <1 ? 1: NumThreads;

        std::cout<<"create Threads Number: "<<NumThreads<<std::endl;
        for(size_t i =0; i< NumThreads;++i){
            //pool.emplace_back(&ThreadPool::schedule,this);// push_back(std::thread{...})
            pool.emplace_back(std::bind(&ThreadPool::schedule,this));
        }
    }
#if 0
    //another way:
    ThreadPool(size_t size =4): _exit {false}{
        //以下为构造一个任务，即构造一个线程
        pool.emplace_back([this] {
            for(;;) {
                std::function<void()> task;   //线程中的函数对象,默认为空函数，无执行体
                {
                    std::unique_lock<std::mutex> lock(this->m_task);
                    this->cv_task.wait(lock, [this]{ return this->_exit || !this->tasks.empty(); });
                    if(this->_exit && this->tasks.empty())
                        return;
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                task(); //调用函数，运行函数
            }
        });
    }
#endif

    // 析构
    ~ThreadPool(){
        std::cout<<__FUNCTION__<<" E"<<std::endl;
        {
            std::unique_lock<std::mutex> lock(m_task);
            this->_exit.store(true);
            cv_task.notify_all();  //通知所有wait状态的线程竞争对象的控制权，唤醒所有线程执行
        }
        for(std::thread &t: pool){
            t.join(); //因为线程都开始竞争了，所以一定会执行完，join可等待线程执行完
            /* or // 让线程“自生自灭”
            thread.detach();
            */
        }
        std::cout<<__FUNCTION__<<" X"<<std::endl;
    }


    // 提交一个任务
    template<class F,class...Args>
        auto queue(F&& f,Args&&... args)->std::future<decltype(f(args...))>{//利用尾置限定符  std future用来获取异步任务的结果
        //auto queue(F&& f,Args&&... args)->std::future<typename std::result_o<F(Args...)>::type>{
            if(_exit.load()){// _exit == true ??
                throw std::runtime_error("enqueue on stopped ThreadPool");
            }

            //packaged_task是对任务的一个抽象，我们可以给其传递一个函数来完成其构造。之后将任务投递给任何线程去完成，通过
            //packaged_task.get_future()方法获取的future来获取任务完成后的产出值
            //using ResType = typename std::result_of<F(Args...)>::type;
            using ResType=decltype(f(args...));// typename std::result_of<F(Args...)>::type, 函数 f 的返回值类型
            auto sp_task = std::make_shared<std::packaged_task<ResType()>>(
                std::bind(std::forward<F>(f), std::forward<Args>(args)...)
            );
            //future为期望，get_future获取任务完成后的产出值
            //获取future对象，如果task的状态不为ready，会阻塞当前调用者
            std::future<ResType> future = sp_task->get_future();
            {// 添加任务到队列
                std::lock_guard<std::mutex> lock {m_task};
                // push(Task{...})//将task投递给线程去完成，vector尾部压入
                tasks.emplace([sp_task](){ (*sp_task)(); });

                cv_task.notify_all();// 唤醒线程执行
            }
            return future;
        }
private:
    // 获取一个待执行的 task
    Task get_one_task(){
        std::unique_lock<std::mutex> lock {m_task};
        this->cv_task.wait(lock, [this]{ return this->_exit || !this->tasks.empty(); });
        if(this->_exit && this->tasks.empty())
            //return EmptyTask;
            return std::function<void()>(); // 空function, ->bool false.

        Task task {std::move(tasks.front())};// 取一个 task
        tasks.pop();
        return task;
    }

    // 任务调度
    void schedule(){
        while(true){
            if(Task task = get_one_task()){
                task();
            }else{
                std::cout<<"get no task"<<std::endl;
                return;
            }
        }
    }
};

#endif

