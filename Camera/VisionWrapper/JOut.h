#ifndef JOUT_H
#define JOUT_H

#include <string>
#include <iostream>
#include <ostream>
#include <sstream>
#include <fstream>
#include <mutex>
#include <atomic>
#include <thread>

using std::cout;
using std::mutex;
using std::unique_lock;
using std::string;
using std::endl;
using std::to_string;
using std::ostream;
using std::thread;
using std::ifstream;
using std::stringstream;

const char JOUT_FIFO[] = "/tmp/myfifo";
class JOut
{
    private:
        string _addr;
        string _port;
        string _fifoName;
        bool _useSocket;
        mutex lock;
        int socketFd;
        bool bExit;

        thread t_log;

        stringstream outstream;
        ifstream instream;
        int fifoFd;
        int wrPipeFd;
        int rdPipeFd;

        std::streambuf* old_outbuf = nullptr;
        std::streambuf* old_errbuf = nullptr;
        ostream& _cout;
        ostream& _cerr;
        std::ofstream out_fifo;

    public:
        JOut(string addr, string port, ostream& co, ostream& ce);
        JOut(string fifoName, ostream& co, ostream& ce);
        int redirect(string addr, string port, ostream& co, ostream& ce);
        int redirect();
        int recover();
        virtual ~JOut();

    private:
        int _redirect();
        int prepareSocket();
        void log(const char* logstr);
        void log(string& logstr);

        void stopTransit();
        void startTransit();
};

#if 0
template <typename T>
JOut & operator<<(JOut & dest, T t)
{
    string tmp = to_string(t);
    dest.log(tmp);
    return dest;
}

JOut & operator<<(JOut & dest, const char* t)
{
    dest.log(t);
    return dest;
}

JOut & operator<<(JOut & dest, string t)
{
    dest.log(t);
    return dest;
}
//JOut & operator<<(JOut & dest, ostream& (*pfun)(ostream&))
//{
//    dest.log("\n");
//    return  dest;
//}

#endif

#endif
