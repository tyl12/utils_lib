#include <iostream>
#include <string>
using namespace std;

class Interface
{
    public:
        virtual ~Interface(){}
        virtual void write(string &) = 0;
        virtual void read(string &) = 0;
};

class Core: public Interface
{
    public:
        virtual ~Core()
        {
            cout << "dtor-Core\n";
        }
        /*virtual*/void write(string &b)
        {
            b += "MESSAGE|";
        }
        void read(string &){};
};

class Decorator: public Interface
{
    Interface *inner;
    public:
    Decorator(Interface *c)
    {
        inner = c;
    }
    virtual ~Decorator()
    {
        delete inner;
    }
    /*virtual*/void write(string &b)
    {
        inner->write(b);
    }
    /*virtual*/void read(string &b)
    {
        inner->read(b);
    }
};

class Wrapper: public Decorator
{
    string forward, backward;
    public:
    Wrapper(Interface *c, string str): Decorator(c)
    {
        forward = str;
        string::reverse_iterator it;
        it = str.rbegin();
        for (; it != str.rend(); ++it)
            backward +=  *it;
    }
    virtual ~Wrapper()
    {
        cout << "dtor-" << forward << "  ";
    }
    void write(string &s){
        Decorator::write(s);
        s += "]" + backward;
        s = forward + "]" + s;
    }
    void read(string &s){
        Decorator::read(s);
    }
};

int main()
{
    Interface *object = new Wrapper(new Wrapper(new Wrapper(new Core(), "123"), "abc"), "987");
    string buf;
    object->write(buf);
    cout << "main: " << buf << endl;
    object->read(buf);
    delete object;
}

/*
main: 987]abc]123]MESSAGE|321]cba]789]
Wrapper: 987
Wrapper: abc
Wrapper: 123
Core: MESSAGE
Wrapper: 321
Wrapper: cba
Wrapper: 789
dtor-987  dtor-abc  dtor-123  dtor-Core
*/
