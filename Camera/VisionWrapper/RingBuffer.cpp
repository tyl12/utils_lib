#include <iostream>
#include "RingBuffer.h"

int self_test(){
    RingBuffer<int> m(5);
    m.push(1);

    m.push(2);
    m.push(3);
    m.push(4);
    m.push(5);

    m.show();
    assert(m.size() == 5);

    m.pop();
    m.pop();
    m.pop();
    m.show();

    assert(m.size() == 2);
    assert(m.top() == 4);
    m.push(11);
    m.show();
    assert(m.size() == 3);
    m.pop();
    m.push(12);
    m.show();

    assert(m.size() == 3);
    assert(m.top() == 5);
    m.pop();
    assert(m.top() == 11);
    m.pop();
    assert(m.top() == 12);
    m.pop();

    m.show();
    assert(m.size() == 0);

    assert(m.isEmpty() == true);
    assert(m.isFull() == false);

    m.push(1);
    m.push(2);
    m.push(3);
    m.push(4);
    m.push(5);
    m.show();
    assert(m.isFull() == true);

    m.push_force(21);
    m.push_force(22);
    m.push_force(23);
    m.show();
    assert(m.size() == 5);

    return 0;
}

#ifdef DEBUG_RingBuffer
int main()
{
    self_test();

    return 0;
}
#endif
