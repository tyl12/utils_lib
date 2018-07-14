#include "Strategy.h"
int main(int argc, char *argv)
{
    string str("321520");
    Translator *translator=new Translator;
    //未指定strategy的时候
    cout<<"No Strategy"<<endl;
    translator->translate(str);
    cout<<"---------------"<<endl;
    //翻译成中文
    auto_ptr<Strategy> s1(new ChineseStrategy);
    translator->set_strategy(s1);
    cout<<"Chinese Strategy"<<endl;
    cout<<translator->translate(str)<<endl;
    cout<<"---------------"<<endl;
    //翻译成英文
    auto_ptr<Strategy> s2(new EnglishStrategy);
    translator->set_strategy(s2);
    cout<<"English Strategy"<<endl;
    cout<<translator->translate(str)<<endl;
    cout<<"----------------"<<endl;
    delete translator;
    return 0;

}
