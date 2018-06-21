#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <sys/ioctl.h>
#include <sys/poll.h>

#include "JOut.h"

//#define DEBUG

#define USE_STREAM

#ifndef USE_STREAM
#define FIFO_NONBLOCK
#endif

static int generateFifo(const char* path){
    //check whether path exists,
    //  if yes, check whether it's fifo,
    //      if yes, return
    //      if not, remove the file and continue
    //  create fifo

    if( access( path, F_OK ) != -1 ){
        struct stat buffer;
        if (stat(path, &buffer) != 0){
            perror("stat error.");
            exit(-1);
        }
        if (S_ISFIFO(buffer.st_mode)){
            printf("%s: fifo %s already exists\n", __FUNCTION__, path);
            return 0;
        }
        else{
            printf("%s: %s already exists but not fifo, remove and recreate\n", __FUNCTION__, path);
            if (remove(path) !=0){
                perror("remove file failed");
                exit(-1);
            }
        }
    }
    if (mkfifo(path, 0666) != 0){
        perror("mkfifo failed");
        exit(-1);
    }
    else{
        printf("mkfifo success\n");
    }
    return 0;

}

JOut::JOut(string addr, string port, ostream& co, ostream& ce):
    _addr(addr),
    _port(port),
    _cout(co),
    _cerr(ce),
    _fifoName(JOUT_FIFO),
    _useSocket(true),
    socketFd(-1)
{
    printf("%s E\n", __FUNCTION__);
    printf("%s X\n", __FUNCTION__);
}

JOut::JOut(string fifoName, ostream& co, ostream& ce):
    _cout(co),
    _cerr(ce),
    _fifoName(fifoName),
    _useSocket(false),
    socketFd(-1)
{
    printf("%s E\n", __FUNCTION__);
    printf("%s X\n", __FUNCTION__);
}

JOut::~JOut()
{
    printf("%s E\n", __FUNCTION__);
    recover();
    printf("%s X\n", __FUNCTION__);
}

int JOut::redirect()
{
    printf("%s E\n", __FUNCTION__);
    bExit = false;

    int ret;
    /* create the FIFO (named pipe) */
    ret = generateFifo(_fifoName.c_str());
    if (ret) return ret;

    if (!_useSocket){
        printf("%s: use direct fifo redirect\n",__FUNCTION__);
    }
    else{
        printf("%s: use socket redirect\n",__FUNCTION__);
#ifdef  FIFO_NONBLOCK
        int p[2];
        if (pipe(p) <0)
            printf("FAIL to create pipe\n");
        rdPipeFd = p[0];
        wrPipeFd = p[1];
#endif
        //continue even socket server is still not ready
        prepareSocket();
        startTransit();
    }
    ret =_redirect();
    printf("%s X\n", __FUNCTION__);
    return ret;
}
int JOut::_redirect()
{
    //must after jout ops to avoid blocking
    out_fifo.open(_fifoName.c_str());

    // Save old states
    old_outbuf = _cout.rdbuf();
    old_errbuf = _cerr.rdbuf();
    // Do actual buffer switch
    _cout.rdbuf(out_fifo.rdbuf());
    _cerr.rdbuf(out_fifo.rdbuf());
    return 0;
}

int JOut::prepareSocket(){
    printf("%s E\n",__FUNCTION__);
    unique_lock<mutex> l(lock);
    if (socketFd >= 0)
        return 0;

    // Create a socket
    int s0 = socket(AF_INET, SOCK_STREAM, 0);
    if (s0 < 0) {
        perror("Cannot create a socket");
        return false;
    }

    // Fill in the address of server
    struct sockaddr_in peeraddr;
    int peeraddr_len;
    memset(&peeraddr, 0, sizeof(peeraddr));
    const char* peerHost = _addr.c_str();

    // Resolve the server address (convert from symbolic name to IP number)
    struct hostent *host = gethostbyname(peerHost);
    if (host == NULL) {
        perror("Cannot define host address"); exit(1);
    }
    peeraddr.sin_family = AF_INET;
    short peerPort = (short) atoi(_port.c_str());
    peeraddr.sin_port = htons(peerPort);

    // Print a resolved address of server (the first IP of the host)
    printf(
        "peer addr = %d.%d.%d.%d, port %d\n",
        host->h_addr_list[0][0] & 0xff,
        host->h_addr_list[0][1] & 0xff,
        host->h_addr_list[0][2] & 0xff,
        host->h_addr_list[0][3] & 0xff,
        (int) peerPort
    );

    // Write resolved IP address of a server to the address structure
    memmove(&(peeraddr.sin_addr.s_addr), host->h_addr_list[0], 4);

    // Connect to a remote server
    int res = connect(s0, (struct sockaddr*) &peeraddr, sizeof(peeraddr));
    if (res < 0) {
        perror("Cannot connect");
        return -1;
    }
    socketFd = s0;
    printf("Socket connected.\n");
    //NOT do start here, as pipe open may block in some scenarios.
    //start();

    printf("%s X\n",__FUNCTION__);
    return 0;
}

int JOut::recover()
{
    printf("%s E\n",__FUNCTION__);
    {
        unique_lock<mutex> l(lock);
        if (bExit){
            printf("%s: already exit\n",__FUNCTION__); return 0;
        }
        bExit = true;
        _cout<<endl;
        _cerr<<endl;

        //recover the ostream buf
        if (old_outbuf != nullptr){
            _cout.rdbuf(old_outbuf);
            old_outbuf = nullptr;
        }
        if (old_errbuf != nullptr){
            _cerr.rdbuf(old_errbuf);
            old_errbuf = nullptr;
        }
        out_fifo.close();
    }

    if (_useSocket){
        stopTransit();
    }

    printf("%s X\n",__FUNCTION__);
    return 0;
}
void JOut::stopTransit()
{
    printf("%s E\n",__FUNCTION__);

    if (fifoFd != -1){
        close(fifoFd); //cause read() return 0;
    }
#ifdef  FIFO_NONBLOCK
    //for nonblock open, we need use select to trigger read() return.
    const char msg[] = "stop";
    write(wrPipeFd, msg, strlen(msg));
    close(wrPipeFd);
    close(rdPipeFd);
#endif
    if (t_log.joinable())
        t_log.join();
    fifoFd = -1;

    if (socketFd >= 0 )
        close(socketFd);
    socketFd = -1;
    printf("%s X\n",__FUNCTION__);
}

void JOut::startTransit()
{
    printf("%s E\n",__FUNCTION__);
    {
        unique_lock<mutex> l(lock);
        if (t_log.joinable())
            return;
    }
    bExit = false;
    //Notes: the block mode open for read may block until open for write happen.
    //so simply put to thread to handle.
#ifndef USE_STREAM
    t_log = std::thread([&]() {
        printf("nativelogthread: E\n");
#ifndef  FIFO_NONBLOCK
        fifoFd = open(_fifoName.c_str(), O_RDONLY);//by default , it's block
        const  int BUFLEN = 512;
        char buf[BUFLEN];
        while(bExit == false ) {
            memset(buf, 0, BUFLEN);
            int ret = read(fifoFd, buf, BUFLEN);
            if (ret == 0){
                printf("fifo read return 0. assuming closed\n");
                break;
            }
            else if (ret == -1 && errno == EINTR){
                printf("fifo read return -1 & EINTR, retry read.\n");
                continue;
            }
            else if (ret < 0){
                printf("fifo read return %d. assuming error\n",ret);
                break;
            }
            #ifdef DEBUG
            printf("nativelogthread: read return %d, buf=\"%s\"\n", ret, buf);
            #endif
            log(buf);
        }
#else
        fifoFd = open(_fifoName.c_str(), O_RDONLY | O_NONBLOCK);
        struct pollfd fds[2];
        int nfds=2;
        memset(fds, 0 , sizeof(fds));
        fds[0].fd = fifoFd;
        fds[0].events = POLLIN;

        fds[1].fd = rdPipeFd;
        fds[1].events = POLLIN;

        const  int BUFLEN = 512;
        char buf[BUFLEN];
        //int timeout = (100); //millisecond
        int timeout = -1;
        while(bExit == false ) {
            int rc = poll(fds, nfds, timeout);
            if (rc < 0)
            {
                perror("  poll() failed");
                break;
            }
            if (rc == 0)
            {
                printf("  poll() timed out.\n");
                continue;
            }
            if(fds[1].revents == POLLIN){
                printf("pipe exit request. quit\n");
                break;
            }
            #ifdef DEBUG
            printf("revents = %d\n", fds[0].revents);
            #endif
            if(fds[0].revents == POLLIN)
            {
                memset(buf, 0, BUFLEN);
retry_read:
                int ret = read(fifoFd, buf, BUFLEN);
                if (ret == 0){
                    printf("fifo read return 0. assuming closed\n");
                    //continue;
                    break;
                }
                else if (ret == -1 && errno == EINTR){
                    printf("fifo read return -1 & EINTR, retry read.\n");
                    goto retry_read;
                }
                else if (ret == -1 && errno == EAGAIN){
                    printf("fifo read return -1 & EAGAIN, retry select.\n");
                    continue;
                }
                else if (ret < 0){
                    printf("fifo read return %d. assuming error\n",ret);
                    break;
                }
                #ifdef DEBUG
                printf("nativelogthread: read return %d, buf=\"%s\"\n", ret, buf);
                #endif
                log(buf);
            }
        }
#if 0
        //use select
        fifoFd = open(_fifoName, O_RDONLY | O_NONBLOCK);
        fd_set readfds;
        const  int BUFLEN = 512;
        char buf[BUFLEN];
        while(bExit == false ) {
            memset(buf, 0, BUFLEN);
            FD_ZERO(&readfds);
            FD_SET(fifoFd, &readfds);
            FD_SET(rdPipeFd, &readfds);
            int max_sd = (fifoFd>rdPipeFd)?fifoFd:rdPipeFd;
            int activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
            if ((activity < 0) && (errno!=EINTR)) {
                printf("select error\n");
            }
            if (FD_ISSET(rdPipeFd, &readfds)){
                printf("rdPipeFd set\n");
                continue; //check bExit by next loop.
            }
            if (FD_ISSET(fifoFd, &readfds)){
retry_read:
                int ret = read(fifoFd, buf, BUFLEN);
                if (ret == 0){
                    printf("fifo read return 0. assuming closed\n");
                    break;
                }
                else if (ret == -1 && errno == EINTR){
                    printf("fifo read return -1 & EINTR, retry read.\n");
                    goto retry_read;
                }
                else if (ret == -1 && errno == EAGAIN){
                    printf("fifo read return -1 & EAGAIN, retry select.\n");
                    continue;
                }
                else if (ret < 0){
                    printf("fifo read return %d. assuming error\n",ret);
                    break;
                }
            }
            //printf("nativelogthread: read return %d, buf=\"%s\"\n", ret, buf);
            log(buf);
        }
#endif
#endif
        printf("nativelogthread: X\n");
    });
#else
    t_log = std::thread([&]() {
        printf("nativelogthread: E\n");

        instream.open(_fifoName, std::ifstream::in);
        while(bExit == false ) {
            string msg;
            bool ret;
            if ( std::getline(instream, msg)){ //"\n" will be consumed by getline.
                #ifdef DEBUG
                printf("nativelogthread: getline return buf=\"%s\"\n", msg.c_str());
                #endif
                msg += "\n";
                log(msg);
            }
            else{
                printf("nativelogthread: fifo getline fail. assuming error\n");
                continue;
            }
        }
        printf("nativelogthread: X\n");
    });
#endif
    printf("%s X\n",__FUNCTION__);
}

void JOut::log(string& logstr){
    if (prepareSocket()==0){
        write(socketFd, logstr.c_str(), logstr.size());
        //printf("##@@##: \"%s\": %d, %d\n", logstr.c_str(), logstr.size(), strlen(logstr.c_str()));
        //fflush(socketFd);
    }
    else
        printf("skip log. socket is still not ready...\n");
}

void JOut::log(const char* logstr){
    if (prepareSocket()==0){
        write(socketFd, logstr, strlen(logstr));
        //fflush(socketFd);
    }
    else
        printf("skip log. socket is still not ready...\n");
}

void test(){
    //JOut jout("localhost", "9090", std::cout, std::cerr);

    //if direct fifo is used, there must be a corresponding reader, otherwise it will block.
    JOut jout("/tmp/myfifo", std::cout, std::cerr);

    printf("%s: before redirect\n",__FUNCTION__);
    cout<<"FUNCTION"<<__FUNCTION__<<__LINE__<<endl;
    usleep(1*1000*1000);
    cout<<"FUNCTION"<<__FUNCTION__<<__LINE__<<endl;
    usleep(1*1000*1000);


    printf("%s: do redirect\n",__FUNCTION__);

    jout.redirect();

    printf("%s: after redirect\n",__FUNCTION__);

    cout<<"FUNCTION"<<__FUNCTION__<<__LINE__<<endl;
    usleep(1*1000*1000);
    cout<<"FUNCTION"<<__FUNCTION__<<__LINE__<<endl;
    usleep(1*1000*1000);

    jout.recover();

    printf("%s: after recover\n",__FUNCTION__);
    cout<<"FUNCTION"<<__FUNCTION__<<__LINE__<<endl;
    usleep(1*1000*1000);
    cout<<"FUNCTION"<<__FUNCTION__<<__LINE__<<endl;
    usleep(1*1000*1000);
}

int main(int argc, char *argv[]) {
    test();
    return 0;
}

