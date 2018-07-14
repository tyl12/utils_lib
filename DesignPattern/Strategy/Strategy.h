#include <iostream>
#include <string>
#include <memory>
using namespace std;
//strategy抽象类，用作接口
class Strategy
{
    public:
        virtual string substitute(string str)=0;
        virtual ~Strategy()
        {
            cout<<" in the destructor of Strategy"<<endl;
        }
};
class ChineseStrategy:public Strategy
{
    public:
        string substitute(string str)
        {
            int index=str.find("520");
            string tempstr=str.replace(index,3,"我爱你");
            return tempstr;
        }
        ~ChineseStrategy()
        {
            cout<<"in the destructor of ChineseStrategy"<<endl;
        }
};
class EnglishStrategy:public Strategy
{
    public:
        string substitute(string str)
        {
            int index=str.find("520");
            string tempstr=str.replace(index,3,"i love ou");
            return tempstr;
        }
        ~EnglishStrategy()
        {
            cout<<" in the destructor of ChineseStrategy"<<endl;
        }
};
//Context类

class Translator
{
    private:
        auto_ptr<Strategy> strategy;

        //在客户代码中加入算法（stategy）类型的指针。
    public:
        ~Translator()
        {
            cout<<" in the destructor of Translator"<<endl;
        }
        void set_strategy(auto_ptr<Strategy> strategy)
        {
            this->strategy=strategy;
        }
        string translate(string str)
        {
            if(0==strategy.get())
                return "";
            return strategy->substitute(str);
        }

};
