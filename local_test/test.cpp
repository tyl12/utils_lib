#include <iostream>
#include <functional>
#include <memory>
#include <thread>
using namespace std;

int main()
{

#if 0
    thread t;
    {
        int a=9;
        t=thread([&](){
            cout<<"thread E"<<endl;
            cout<<a<<endl;
        });
    }
    int* p = new int[20];
    delete []p;
    t.join();


    cout<<"-----1"<<endl;
#endif
#if 1
    //auto sp = unique_ptr<FILE, function<void(FILE*)>>(std::fopen("test.txt", "w"), [](FILE* f){
    auto sp = unique_ptr<FILE, function<void(FILE*)>>(std::fopen("test.txt", "w"), [](auto f){
        cout<<"close file"<<endl;
        fclose(f);});

    if (sp!=NULL)
    {
        cout<<"write out"<<endl;
        fputs ("fopen example\n",sp.get());
    }
    cout<<"-----2"<<endl;

    const int N = 30;
    //auto ptr1 = unique_ptr<int[]>(new int[N]);
    auto ptr1 = unique_ptr<int[], function<void(int[])>>(new int[N], [](auto s){
        delete[] s;
    });
    for (auto s=0;s<N;s++)
    {
        ptr1[s] = s;
        cout<<ptr1[s]<<endl;
//        printf("%d: %d\n", s, ptr1[s]);
//        printf("%d: %d\n", s, *(ptr1[s]));
    }

    int s1=0;
    auto fs1 = [&](){
        cout<<s1<<endl;
    };

    s1=2;
    auto fs2=fs1;

    fs1();
    fs2();


    char s4[20];
    auto fs4 = [=](){
        volatile char p=s4[4];
        cout<<"test"<<endl;
    };

    
    cout<<sizeof(fs1)<<endl;
    fs4();
    cout<<sizeof(fs4)<<endl;


#endif
    return 0;
}
