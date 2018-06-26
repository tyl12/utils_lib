#ifndef CONTEXT_H
#define CONTEXT_H

#include "State.h"

class Context
{
public:
    Context();
    virtual ~Context() {};

    void changeState(State * st);
    void request();

private:
    State *m_pState;
};
#endif
