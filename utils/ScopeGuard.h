#include <algorithm>
#include <functional>
#include <iostream>
using namespace std;

template< typename Lambda >
class ScopeGuard
{

    private:
    bool dismissed; // not mutable
    Lambda rollbackLambda;

    public:
    ScopeGuard() = delete;
    /*
    ScopeGuard(const ScopeGuard<Lambda>&) = delete;
    void operator=(const ScopeGuard<Lambda>&) = delete;
    void operator=(ScopeGuard<Lambda>&&) = delete;
    */
    ScopeGuard(const ScopeGuard&) = delete;
    void operator=(const ScopeGuard&) = delete;
    void operator=(ScopeGuard&&) = delete;

    // is_nothrow_constructible: identifies whether T is a constructible type using the set of argument
    // types specified by Arg, and such construction is known not to throw any exception.
    explicit ScopeGuard(Lambda&& _l) noexcept(std::is_nothrow_constructible<Lambda, Lambda&&>::value) :
        dismissed(false),
        rollbackLambda(std::forward<Lambda>(_l)) // avoid copying unless necessary
    {}

    // move constructor
    ScopeGuard(ScopeGuard&& that) noexcept(std::is_nothrow_constructible<Lambda, Lambda&&>::value) :
        dismissed(that.dismissed),
        rollbackLambda(std::forward<Lambda>(that.rollbackLambda))
    {
        that.dismissed = true;
    }

    ~ScopeGuard() noexcept
    {
        if (!dismissed){
            try{
                rollbackLambda(); // what if this throws?
            }catch(...){cerr<<__FUNCTION__<<" throw exception!"<<endl;}
        }
    }
    void dismiss() { dismissed = true; } // no need for const
};

template<typename rLambda>
ScopeGuard<rLambda> makeScopeGuard(rLambda&& _r)
{
    return ScopeGuard<rLambda>( std::forward<rLambda>( _r ));
}

#define DO_STRING_JOIN2(arg1, arg2) arg1 ## arg2
#define STRING_JOIN2(arg1, arg2) DO_STRING_JOIN2(arg1, arg2)

#define ON_SCOPE_EXIT(func) \
    auto STRING_JOIN2(scope_exit_, __LINE__) = makeScopeGuard(func)


