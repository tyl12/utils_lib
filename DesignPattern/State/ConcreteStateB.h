#ifndef CONCRETESTATEB_H
#define CONCRETESTATEB_H
#include "State.h"
#include "Context.h"

class ConcreteStateB : public State
{
public:
    virtual ~ConcreteStateB();
    static State * Instance();
    virtual void handle(Context * c);

private:
    ConcreteStateB() {};
    static State * m_pState;
};
#endif
