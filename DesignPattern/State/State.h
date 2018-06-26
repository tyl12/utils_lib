#ifndef STATE_H
#define STATE_H
class Context; //前置声明
class State
{
public:
    State() {};
    virtual ~State() {};

    virtual void handle(Context *context) = 0;
};
#endif
