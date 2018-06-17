#include <iostream>
#include "test.h"
using namespace std;
using namespace tttt;

class B{
    private:
        int b;
};

int main()
{
    testclass<B> a;
    a.test();
    return 0;
}
