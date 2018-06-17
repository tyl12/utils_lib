#include <iostream>
#include "test.h"
using namespace std;

namespace tttt{

template<typename T>
void  __attribute__((visibility("default"))) testclass<T>::test(){
    cout<<"323"<<endl;
    return;
}

template<typename T>
std::shared_ptr<T>  /* __attribute__((visibility("default")))*/ testclass<T>::get(){
    m_pSington = std::make_shared<T>();
    return m_pSington;
}


__attribute__ ((visibility("default")))
void testfunc(){
    cout<<"sdfasdfasd"<<endl;
    return;
}
}
