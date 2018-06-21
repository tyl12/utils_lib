#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <thread>
#include <chrono>
#include <iostream>

//#include <boost/ref.hpp>
//#include <boost/asio.hpp>
//#include <boost/bind.hpp>
//#include <boost/function.hpp>

#include "utils.h"

using namespace std;

namespace utils{

static bool gDebug=false;



int wait_for_network_block(){
    while(true){
        string cmd1 = "ping -q -c 1 -W 1 www.baidu.com >/dev/null";
        int ret1 = system(cmd1.c_str());
        string cmd2 = "ping -q -c 1 -W 1 www.xinhuanet.com >/dev/null";
        int ret2 = system(cmd2.c_str());

        if (ret1 == 0 || ret2 == 1){
            cout<<__FUNCTION__<<" ping success"<<endl;
            return 0;
        }
        cout<<__FUNCTION__<<" ping failed, retry"<<endl;
        usleep(5*1000*1000);
    }
    return -1;
}

bool comparemd5_withcmd(string& cmd, const string& md5)
{
    cout<<__FUNCTION__<<endl;
    vector<string> output;
    int ret = launch_cmd(cmd.c_str(), output);
    string md5_of_file = output[0];
    if (strcmp(md5_of_file.c_str(), md5.c_str()) == 0){
        cout<<"check md5 success for cmd: " << cmd <<endl;
        return true;
    }
    cout<<"check md5 fail for cmd: " << cmd<<endl;
    cout<<"expected:"<<md5<<endl;
    cout<<"real:"<<md5_of_file<<endl;
    return false;
}

bool comparemd5(const char* file, const string& md5, bool withroot = false)
{
    cout<<__FUNCTION__<<endl;

    string cmd = string("md5sum ") + file + " | awk '{print $1}' ";
    if (withroot)
        cmd = string("sudo  ") + cmd;

    vector<string> output;
    int ret = launch_cmd(cmd.c_str(), output);
    string md5_of_file = output[0];

    if (strcmp(md5_of_file.c_str(), md5.c_str()) == 0){
        cout<<"check md5 success for file: " << file<<endl;
        return true;
    }
    cout<<"check md5 fail for file: " << file<<endl;
    cout<<"expected:"<<md5<<endl;
    cout<<"real:"<<md5_of_file<<endl;
    return false;
}


string& trim(string &s)
{
    if (gDebug) printf("%s: input: %s\n", __FUNCTION__, s.c_str());
    if (s.empty())
    {
        return s;
    }
    s.erase(0,s.find_first_not_of(" "));
    s.erase(s.find_last_not_of(" ") + 1);
    if (gDebug) printf("%s: output: %s\n", __FUNCTION__, s.c_str());
    return s;
}

vector<string> split_by_delim(const string& line, const char ch){
    if (gDebug) printf("%s: input: %s\n", __FUNCTION__, line.c_str());
    vector<string> output;
    string val;
    std::istringstream tokenStream(line);
    while (getline(tokenStream, val, ch)){
        if (gDebug) printf("%s: split: %s\n", __FUNCTION__, val.c_str());
        output.push_back(val);
    }
    return output;
}

int launch_cmd(const char* cmd_in, vector<string>& output){
    FILE *fpipe;
    string result;
    char val[1024] = {0};

    if (gDebug) printf("%s: cmd: %s\n", __FUNCTION__, cmd_in);
    /*
    string cmd = string("bash -c \"") + cmd_in + "\"";
    if (0 == (fpipe = (FILE*)popen(cmd.c_str(), "r"))) {
    */
    if (0 == (fpipe = (FILE*)popen(cmd_in, "r"))) {
        perror("popen() failed.");
        return -1;
    }

    /*
     * Reads characters from stream and stores them as a C string into str until (num-1) characters
     * have been read or either a newline or the end-of-file is reached, whichever happens first.
     * A newline character makes fgets stop reading, but it is considered a valid character by the
     * function and included in the string copied to str.
     * A terminating null character is automatically appended after the characters copied to str.
     */
    while (fgets(val, sizeof(val)-1,fpipe) != nullptr) {
        if (val[strlen(val)-1] == '\n'){
            if (gDebug) printf("%s: %s\n", __FUNCTION__, val);
            result += val;
            result.erase(result.end()-1);//remove tailing '\n'
            trim(result);
            if (!result.empty())
                output.push_back(move(result));
            result.clear();
            continue;
        }
        result += val;
    }
    pclose(fpipe);

    return 0;
}


vector<string> get_current_mac_addrs(){
    vector<string> output;
    if (launch_cmd("ifconfig | grep HWaddr | awk '{print $NF}'", output) != 0){
        cout<<"Fail to execute "<<__FUNCTION__<<endl;
    }
    return output;
    /*
    replace(mac.begin(), mac.end(), '\n', ';');
    if ( mac.compare("") == 0){
        mac = launch_cmd("ifconfig | grep \"硬件地址\" | awk '{print $NF}'");
        replace(mac.begin(), mac.end(), '\n', ';');
    }
    if ( mac.compare("") == 0){
        cout<<"Failed to get mac addr"<<endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    */
}

static const char ENV_DEBUG[]="DEBUG";
bool isDebugEnv(){
    const char *debug = getenv(ENV_DEBUG);
    if (debug == nullptr){
        cout<<"ENV_DEBUG is not set, assume false"<<endl;
        return false;
    }
    if (strcmp(debug, "true") == 0){
        return true;
    }
    return false;
}

#include <stdio.h>
#include <string.h>
#include <shadow.h>
#include <unistd.h>
#include <pwd.h>

static const char user_passwd[]="invalid_passwd";
int check_passwd(const char* name ){
    const char *username = NULL;
    if (name == NULL || strcmp(name, "") == 0)
        username = getlogin();
    else
        username = name;

    cout<<"current username: " << username<<endl;
    struct spwd  *sp;
    sp = getspnam(username);
    if(sp == NULL) {
        printf("get spentry error. %s\n", strerror(errno));
        return -1;
    }

    if(strcmp(sp->sp_pwdp, (char*)crypt(user_passwd, sp->sp_pwdp)) == 0) {
        printf("passwd check success\n");
        return 0;
    }
    else {
        printf("passwd check fail\n");
        return -1;
    }
}

string getLoginUser(){
#if 0
    char *name;
    name = getlogin();
    if (!name){
        cout<<"getlogin() error"<<endl;
        name="";
    }
    else
        cout<<"This is the login info: "<<name<<endl;;
    return name;
#else
    struct passwd *pwd = getpwuid(getuid());
    if (pwd){
        cout<<"getpwuid() return: " << pwd->pw_name<<endl;
        return pwd->pw_name;
    }
    return "";
#endif
}

int64_t get_time_ms()
{
    using namespace std::chrono;
    auto now = system_clock::now();
    auto now_millis = time_point_cast<milliseconds>(now);
    auto value = now_millis.time_since_epoch();

    return value.count();
}

std::string get_date_sec()
{
    using namespace std::chrono;
    char tp[64] = { '\0' };
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);
    strftime(tp, 64, "%Y_%m_%d_%H_%M_%S", &tm);
    return std::string(tp);
}

std::string get_ts(void) {
    /*
    char buffsec[32];
    struct tm *sTm;
    time_t now = time (0);
    //sTm = gmtime (&now);
    sTm = localtime (&now);
    strftime (buff, sizeof(buff), "%Y-%m-%d %H:%M:%S.00:", sTm);
    //printf ("%s %s\n", buff, "Event occurred now");
    return buff;
    */

    char buffsec[32];
    char buffms[32];
    int millisec;
    struct tm* tm_info;
    struct timeval tv;

    gettimeofday(&tv, NULL);

    millisec = lrint(tv.tv_usec/1000.0); // Round to nearest millisec
    if (millisec>=1000) { // Allow for rounding up to nearest second
        millisec -=1000;
        tv.tv_sec++;
    }

    tm_info = localtime(&tv.tv_sec);

    strftime(buffsec, sizeof(buffsec), "%Y:%m:%d %H:%M:%S", tm_info);
    sprintf(buffms, "%s.%03d", buffsec, millisec);
    return buffms;
}

const int stdoutfd(dup(fileno(stdout)));
const int stderrfd(dup(fileno(stdout)));
int redirect_stdout_stderr(const char* fname){
    int newdest = open(fname, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    fflush(stdout);
    fflush(stderr);
    dup2(newdest, fileno(stdout));
    dup2(newdest, fileno(stderr));
    close(newdest);
    return 0;
}

int restore_stdout(){
    fflush(stdout);
    dup2(stdoutfd, fileno(stdout));
    dup2(stderrfd, fileno(stderr));
    //comment if it need reused
    //close(stdoutfd);
    //close(stderrfd);
    return 0;
}

}
