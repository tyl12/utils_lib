#include <iostream>
using namespace std;

void t2( int(int, int));


int inputf(int a, int b)
{
    return a+b;
}

typedef int(*inputf2)(int,int);

int main()
{
    t2(inputf);
    return 0;
}
