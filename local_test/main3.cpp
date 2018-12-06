
template <typename T> // 嗯，需要一个T
class TypeToID
{
public:
    static int const ID = -1;	// 用最高位表示它是一个指针
};

template <> class TypeToID<int>
{
public:
    static int const ID = 1;
};

template <> class TypeToID<double>
{
public:
    static int const ID = 2;
};

template <> class TypeToID<char>
{
public:
    static int const ID = 3;
};
template <> class TypeToID<char *>
{
public:
    static int const ID = 4;
};

template <typename T> // 嗯，需要一个T
class TypeToID<T*> // 我要对所有的指针类型特化，所以这里就写T*
{
public:
      typedef T		 SameAsT;
    static int const ID = 0x80000000;	// 用最高位表示它是一个指针
};

template <> class TypeToID<int *>
{
public:
    static int const ID = 6;
};



template <typename T> struct X {};

template <typename T> struct Y
{
    typedef X<T> ReboundType;				// 类型定义1
    typedef typename X<T>::MemberType  MemberType;	// 类型定义2
    //typedef UnknownType MemberType3;			// 类型定义3
void foo()
    {
        X<T> instance0;
        typename X<T>::MemberType instance1;
        //WTF instance2
        //大王叫我来巡山 - + &
    }
};



#include <iostream>
using namespace std;



//void testfunc( int (*inf)(int, int));
void testfunc( int(int, int));
void testfunc2( int(int, int));

int inputf(int a, int b)
{
    return a+b;
}


int main()
{
    cout<<TypeToID<int>::ID<<endl;
    cout<<TypeToID<double>::ID<<endl;
    cout<<TypeToID<char>::ID<<endl;
    cout<<TypeToID<int*>::ID<<endl;
    cout<<TypeToID<char*>::ID<<endl;
    cout<<TypeToID<double*>::ID<<endl;
    //cout << "ID of float*: " << TypeToID< TypeToID<char*>::SameAsT >::ID << endl;
    //Y<int> m;
    //
    testfunc(inputf);
    testfunc2(inputf);

    return 0;
}
