#include <iostream>
#include <memory>
#include <cassert>
using namespace std;

template <typename T>
class naive_function;

template <typename ReturnValue, typename... Args>
class naive_function<ReturnValue(Args...)> {
    private:
        class ICallable {
            public:
                virtual ~ICallable() = default;
                virtual ReturnValue Invoke(Args...) = 0;
        };

        template <typename T>
        class CallableT : public ICallable {
            public:
                CallableT(const T& t)
                    : t_(t) {
                    }
                ~CallableT() override = default;
                ReturnValue Invoke(Args... args) override {
                    return t_(args...);
                }
            private:
                T t_;
        };


        std::unique_ptr<ICallable> callable_;
    public:
        template <typename T>
        naive_function& operator=(T t) {
            callable_ = std::make_unique<CallableT<T>>(t);
            return *this;
        }

        template <typename T>
        naive_function(T t) {
            callable_ = std::make_unique<CallableT<T>>(t);
        }

        naive_function() { }

        ReturnValue operator()(Args... args) const {
            assert(callable_);
            return callable_->Invoke(args...);
        }
};
void func() {
    cout << "func" << endl;
}
struct functor {
    void operator()() {
        cout << "functor" << endl;
    }
};

typedef void (*fp)(int, int);
void funcf(int a, int b){
    cout<<a<<"--"<<b<<endl;
};

void mf(fp fpp){
    fpp(4,5);
    cout<<"------"<<endl;
}

template <typename S>
void testf2(S s){
    s(1,2);
}

void testfunc(int a, int b)
{
    cout<<a << b<<endl;
}

int main() {
    naive_function<void()> f1 = [](){cout<<"test"<<endl;};
    f1();

    naive_function<void()> f;
    f = func;
    f();
    f = functor();
    f();
    f = []() { cout << "lambda" << endl; };
    f();

    fp ap = funcf;
    ap(2,3);

    mf(ap);


    int a =0;
    f=[&](){
        cout<<a<<endl;
        a=9;
    };
    f();
    cout<<a<<endl;
    a=10;
    f();

    testf2(testfunc);

    return 0;
}

