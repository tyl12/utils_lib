#include <iostream>
#include "ScopeGuard.h"
using namespace std;

int main(){
    auto a = makeScopeGuard([]{ cout<<"should NOT called"<<endl; });

    ON_SCOPE_EXIT([]{cout<<"ON_SCOPE_EXIT1 called"<<endl;});
    ON_SCOPE_EXIT([]{cout<<"ON_SCOPE_EXIT2 called"<<endl;});

    int b=1;
    auto bb = makeScopeGuard([&]{ cout<<"should called."<<b<<endl; });
    b = 4;

    a.dismiss();
    return 0;
}

