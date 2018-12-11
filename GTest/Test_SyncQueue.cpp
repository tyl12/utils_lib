#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include "SyncQueue.h"

using namespace std;

int main()
{
    SyncQueue<int, 10> sq;
    thread t([&](){
        for (int i=0;i<20;i++){
            cout<<"push:"<<i<<endl;
            sq.push(i);
            usleep(10);
        }

    });

    usleep(5*1000*1000);
    if (sq.size() != 10){
        cout<<"ERROR: push size not match"<<endl;
        return -1;
    }

    thread tt([&](){
        for (int i=0;i<10;i++){
            int m;
            sq.pop(m);
            usleep(10);
            cout<<"pop:"<<m<<endl;
        }
    });

    usleep(5*1000*1000);
    if (sq.size() != 10){
        cout<<"ERROR: pop size not match"<<endl;
        return -1;
    }

    thread ttt([&](){
        for (int i=0;i<10;i++){
            int m;
            sq.pop(m);
            usleep(10);
            cout<<"pop:"<<m<<endl;
        }
    });

    usleep(5*1000*1000);
    if (sq.size() != 0){
        cout<<"ERROR: size not match"<<endl;
        return -1;
    }


    t.join();
    tt.join();
    ttt.join();

    cout<<"Test_SyncQueue SUCCESS!"<<endl;

    return 0;
}

