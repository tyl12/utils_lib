#include <algorithm>
#include <functional>
using namespace std;

template< typename Lambda >
class ScopeGuard
{

    private:
    bool committed; // not mutable
    Lambda rollbackLambda;

    public:
    ScopeGuard() = delete;
    template <typename L> ScopeGuard(const ScopeGuard<L>&) = delete;
    template <typename L> void operator=(const ScopeGuard<L>&) = delete;
    template <typename L> void operator=(ScopeGuard<L>&&) = delete;

    // make sure this is not a copy ctor
    template <typename L>
    explicit ScopeGuard(L&& _l):
        committed(false),
        rollbackLambda(std::forward<L>(_l)) // avoid copying unless necessary
    {}

    // move constructor
    ScopeGuard(ScopeGuard&& that):
        committed(that.committed),
        rollbackLambda(std::move(that.rollbackLambda)) {
            that.committed = true;
        }

    ~ScopeGuard()
    {
        if (!committed)
            rollbackLambda(); // what if this throws?
    }
    void commit() { committed = true; } // no need for const
};

template<typename rLambda>
ScopeGuard< rLambda > makeScopeGuard(rLambda&& _r)
{
    return ScopeGuard< rLambda >( std::forward<rLambda>( _r ));
}

#define DO_STRING_JOIN2(arg1, arg2) arg1 ## arg2
#define STRING_JOIN2(arg1, arg2) DO_STRING_JOIN2(arg1, arg2)

#define ON_SCOPE_EXIT(func) \
    auto STRING_JOIN2(scope_exit_, __LINE__) = makeScopeGuard(func)


