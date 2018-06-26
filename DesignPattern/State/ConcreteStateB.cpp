#include "ConcreteStateB.h"
#include "ConcreteStateA.h"
#include "Context.h"
#include <iostream>
using namespace std;

State * ConcreteStateB::m_pState = NULL;

ConcreteStateB::~ConcreteStateB()
{
    delete m_pState;
}

State * ConcreteStateB::Instance()
{
    if (NULL == m_pState)
    {
        m_pState = new ConcreteStateB();
    }
    return m_pState;
}

void ConcreteStateB::handle(Context * c)
{
    cout << "Doing something in State B.\n Done,change state to A" << endl;
    c->changeState(ConcreteStateA::Instance());
}
