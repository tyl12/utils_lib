#include "observer.h"
int main()
{
    //目标(被观察者)
    CBloger* pBloger = new CBloger();
    CPortal* pPortal = new CPortal();
    //观察者. 一个观察者可以观察多个目标
    CRSSReader* pRssReader = new CRSSReader("rss reader");
    CMailReader* pMailReader = new CMailReader("mail reader");
    pBloger->Attach(pRssReader);  //bloger注册观察者
    pBloger->Attach(pMailReader); //bloger注册观察者
    pPortal->Attach(pRssReader);  //portal注册观察者
    pPortal->Attach(pMailReader); //portal注册观察者
    //博客发布信息
    pBloger->Publish("博客分享设计模式");
    cout << endl;
    //门户发布信息
    pPortal->Publish("门户分享设计模式");
    cout << "\nportal detached mail reader" << endl;

    pPortal->Detach(pMailReader);

    cout << "portal observers count: " << pPortal->GetObserversCount() << endl << endl;
    pPortal->Publish("门户分享设计模式");

    //system("pause");
    return 0;
}
