#include <iostream>
#include "ScopeGuard.h"
using namespace std;

int main(){
    auto a = makeScopeGuard([]{ cout<<"a"<<endl; });

    ON_SCOPE_EXIT([]{cout<<"test"<<endl;});

    int b=1;
    auto bb = makeScopeGuard([&]{ cout<<b<<endl; });
    b = 4;

    //a.commit();
    return 0;
}

