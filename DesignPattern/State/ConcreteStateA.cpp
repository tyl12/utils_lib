#include "ConcreteStateB.h"
#include "ConcreteStateA.h"
#include "Context.h"
#include <iostream>
using namespace std;

State * ConcreteStateA::m_pState = NULL;

ConcreteStateA::~ConcreteStateA()
{
    delete m_pState;
}

State * ConcreteStateA::Instance()
{
    if (NULL == m_pState)
    {
        m_pState = new ConcreteStateA();
    }
    return m_pState;
}

void ConcreteStateA::handle(Context * c)
{
    cout << "Doing something in State A.\n Done,change state to B" << endl;
    c->changeState(ConcreteStateB::Instance());
}
