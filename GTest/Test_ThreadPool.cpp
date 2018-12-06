#include "ThreadPool.h"

void func1()
{
    std::cout <<"func1"<< std::endl;
}

struct func2{
    int operator()(){
        std::cout <<"func2"<< std::endl;
        return 22;
    }
};

int main()
{
    try{
        utils::ThreadPool executor {10};
        std::future<void> ff = executor.queue(func1);
        std::future<int> fg = executor.queue(func2{});
        auto fh = executor.queue([]()->std::string {
                std::cout <<"func3"<< std::endl;
                return"func3";
                });

        ff.get();
        std::cout << fg.get()<<" "<< fh.get()<< std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(2));
    }catch(std::exception& e){
        std::cout <<"sth wrong"<< e.what()<< std::endl;
    }


    std::cout<<"-----"<<std::endl;

    utils::ThreadPool pool(4);
    std::vector< std::future<int> > results;

    for(int i = 0; i < 8; ++i) {
        results.emplace_back(
          pool.queue([i] {
            std::cout << " task E " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            std::cout << " task X " << i << std::endl;
            return i*i;
        })
      );
    }

    for(auto && result: results)    //通过future.get()获取返回值
        std::cout << result.get() << ' ';
    std::cout << std::endl;

    return 0;
}
