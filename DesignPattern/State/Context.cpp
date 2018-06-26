#include "Context.h"
#include "ConcreteStateA.h"

Context::Context()
{
    m_pState = ConcreteStateA::Instance();
}

void Context::changeState(State * st) {
    m_pState = st;
}

void Context::request() {
    m_pState->handle(this);
}
