#include <stdio.h>
#include <regex>
#include <stdlib.h>
#include <locale.h>
#include <unistd.h>
#include <algorithm>
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
#include <iterator>
#include <dirent.h>


//for format_string
#include <iostream>
#include <vector>
#include <cstdarg>
#include <cstring>


//#include <boost/ref.hpp>
//#include <boost/asio.hpp>
//#include <boost/bind.hpp>
//#include <boost/function.hpp>

#include "utils.h"

using namespace std;

namespace utils{

void libutils_init(void) __attribute__((constructor));
void libutils_fini(void) __attribute__((destructor));

static bool bDebugLibUtils=false;


/*
static const char ENV_DEBUG[]="DEBUG_LIBUTILS";

bool isDebugEnvSet(){
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
*/

/**************************************************************************/
void libutils_init(void){
    LOGD("libsutils constructor");
}
void libutils_fini(void){
    LOGD("libsutils destructor");
}

/**************************************************************************/
Marker::Marker(function<void(void)>&& begin, function<void(void)>&& end):
    mFunc(move(end))
{
    if (gDebugUtils)
        begin();
}

Marker::Marker(function<void(void)>&& end):
    mFunc(move(end))
{ }

Marker::~Marker(){
    if (gDebugUtils)
        if (mFunc)
            mFunc();
}

/**************************************************************************/
Perf::Perf(string tag)
{
    over = false;
    sTag = tag;
    startTime = get_time_ms();
    LOGD("[PERF]%s: start", sTag.c_str());
}
void Perf::start(string tag)
{
    over = false;
    sTag = tag;
    startTime = get_time_ms();
    LOGD("[PERF]%s: start", sTag.c_str());
}

Perf::~Perf()
{
    if (!over)
    {
        done();
    }
}

void Perf::reset()
{
    startTime = get_time_ms();
    over = false;
    LOGD("[PERF]%s: restart", sTag.c_str());
}

void Perf::done()
{
    if (!over)
    {
        auto end = get_time_ms();
        LOGD("[PERF]%s: done, cost %.2f sec", sTag.c_str(), (end - startTime) / 1000.0);
        over = true;
    }
    else
    {
        LOGD("[PERF]%s: already done", sTag.c_str());
    }
}
/**************************************************************************/

//
string merge_intvector_to_string_with_traits(vector<int> data){
    std::ostringstream vts;

    Marker _m(
        [&]{LOGD("X, output:[%s]", vts.str().c_str());}
        );

    if (!data.empty()) {
        // Convert all but the last element to avoid a trailing ","
        std::copy(data.begin(), data.end()-1, std::ostream_iterator<int>(vts, ", "));
        // Now add the last element with no delimiter
        vts << data.back();
        cout<<"merged vector to string: "<<endl;
    }
    return vts.str();
}

int wait_for_network_block(){
    while(true){
        string cmd1 = "ping -q -c 1 -W 1 www.baidu.com >/dev/null";
        int ret1 = system(cmd1.c_str());
        string cmd2 = "ping -q -c 1 -W 1 www.xinhuanet.com >/dev/null";
        int ret2 = system(cmd2.c_str());

        if (ret1 == 0 || ret2 == 1){
            LOGD("ping success");
            return 0;
        }
        LOGE("ping failed. retry");
        usleep(5*1000*1000);
    }
    return -1;
}

bool comparemd5_withcmd(string& cmd, const string& md5)
{
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

static const string WHITE_SPACE_STR=" \n\r\t\f\v"; //[\n\t\r\f\v]等价于正则表达式的\s,匹配任意空白符

#define TRIM_V1 1
#if TRIM_V1

//inplace trim
string& ltrim_inplace(string& s)
{
    Marker _m(
        [&]{LOGD("E,  input:[%s]", s.c_str());},
        [&]{LOGD("X, output:[%s]", s.c_str());}
        );

    size_t start=s.find_first_not_of(WHITE_SPACE_STR);
    if (start == string::npos) s.clear();
    else s.erase(0, start);
    return s;
}
string& rtrim_inplace(string& s)
{
    Marker _m(
        [&]{LOGD("E,  input:[%s]", s.c_str());},
        [&]{LOGD("X, output:[%s]", s.c_str());}
        );
    size_t end=s.find_last_not_of(WHITE_SPACE_STR);
    if (end == string::npos) s.clear();
    else s.erase(end+1);
    return s;
}
string& lrtrim_inplace(string& s)
{
    Marker _m(
        [&]{LOGD("E,  input:[%s]", s.c_str());},
        [&]{LOGD("X, output:[%s]", s.c_str());}
        );
    return rtrim_inplace(ltrim_inplace(s));
}

//not inplace, use find_first/last_not_of
string ltrim(const string& s)
{
    size_t start = s.find_first_not_of(WHITE_SPACE_STR);
    string output = (start == string::npos)? "" : s.substr(start);

    Marker _m(
        [&]{LOGD("E,  input:[%s]", s.c_str());},
        [&]{LOGD("X, output:[%s]", output.c_str());}
        );

    return output;
}
string rtrim(const string& s)
{
    size_t end = s.find_last_not_of(WHITE_SPACE_STR);
    string output = (end == string::npos)? "" : s.substr(0, end+1); //##@@##

    Marker _m(
        [&]{LOGD("E,  input:[%s]", s.c_str());},
        [&]{LOGD("X, output:[%s]", output.c_str());}
        );

    return output;
}
string lrtrim(const string& s)
{
    string output = rtrim(ltrim(s));
    Marker _m(
        [&]{LOGD("E,  input:[%s]", s.c_str());},
        [&]{LOGD("X, output:[%s]", output.c_str());}
        );

    return output;
}
#elif TRIM_V2

/*
int isspace ( int c );
Check if character is a white-space
Checks whether c is a white-space character.

For the "C" locale, white-space characters are any of:
' '	(0x20)	space (SPC)
'\t'	(0x09)	horizontal tab (TAB)
'\n'	(0x0a)	newline (LF)
'\v'	(0x0b)	vertical tab (VT)
'\f'	(0x0c)	feed (FF)
'\r'	(0x0d)	carriage return (CR)

Other locales may consider a different selection of characters as white-spaces, but never a character that returns true for isalnum.
For a detailed chart on what the different ctype functions return for each character of the standard ASCII character set, see the reference for the <cctype> header.
In C++, a locale-specific template version of this function (isspace) exists in header <locale>.
*/

//inplace
string& ltrim_inplace(string& s)
{
    auto it = find_if(s.begin(), s.end(), [](char& c){
        return !isspace<char>(c, locale::classic());
    });
    s.erase(s.begin(), it);
    return s;
}
string& rtrim_inplace(string& s)
{
    auto it = find_if(s.rbegin(), s.rend(), [](char& c){
        return !isspace<char>(c, locale::classic());
    });
    s.erase(it.base(), s.end());
    return s;
}
string& lrtrim_inplace(string& s)
{
    return rtrim_inplace(ltrim_inplace(s));
}
#elif TRIM_V3
//not inplace
string ltrim(const string& s)
{
    return regex_replace(s, regex("^\\s+"), string(""));
}
string rtrim(const string& s)
{
    return regex_replace(s, regex("\\s+$"), string(""));
}
string lrtrim(const string& s)
{
    rtrim(ltrim(s));
}
#endif

vector<string> split_by_delim(const string& line, const char ch){
    if (gDebugUtils) LOGD("input:[%s]", line.c_str());
    vector<string> output;
    string val;
    std::istringstream tokenStream(line);
    while (getline(tokenStream, val, ch)){
        if (gDebugUtils) LOGD("split:[%s]", val.c_str());
        output.push_back(val);
    }
    return output;
}
vector<string> split_by_delims(const string& str, const string& delims){
    string patternstr=string("[^")+delims+"]*["+delims+"]";
    cout<<"search pattern="<<patternstr<<endl;

    regex pattern(patternstr);
    vector<string> output;
    smatch result;
    string s = str;
    int i = 0;
    while (regex_search(s, result, pattern)){
        if (result.ready()){
            cout<<"index="<<i<<", result=["<<result[0]<<"]"<<endl;
            string tmp = result[0];
            char delim = *tmp.rbegin(); //or: tmp.back();
            if (delims.find(delim) != string::npos){
                tmp.erase(tmp.end()-1);
            }
            output.emplace_back(tmp);
            s = result.suffix();
            i++;
        }
    }
    if (s!="")
        output.emplace_back(s);
    return output;
}

//c11
vector<string> split_by_regex_iterator(const string& s, const regex& pattern){
    if (gDebugUtils) LOGD("input:[%s]", s.c_str());
    vector<string> output;

    auto begin = sregex_iterator(s.begin(), s.end(), pattern);
    auto end = sregex_iterator();
    for (auto i = begin; i != end; ++i)
        output.emplace_back((*i).str());
    return output;
}

vector<string> split_by_regex_search(const string& str, const regex& pattern){
    if (gDebugUtils) LOGD("input:[%s]", str.c_str());
    vector<string> output;
    smatch result;
    string s = str;
    int i = 0;
    //while (!s.empty() && regex_search(s, result, delims)){
    while (regex_search(s, result, pattern)){
        if (result.ready()){
            /*
            cout<<"prefix=["<<result.prefix()<<"]"<<endl;
            cout<<"suffix=["<<result.suffix()<<"]"<<endl;
            */
            /*
            for (auto x:result){
                cout<<x<<endl;
            }
            */
            if (gDebugUtils) cout<<"index="<<i<<", result=["<<result[0]<<"]"<<endl;
            output.emplace_back(result[0]);
            s = result.suffix();
            i++;
        }
    }
    return output;
}

/*
std::string::substr
string substr (size_t pos = 0, size_t len = npos) const;
Generate substring
Returns a newly constructed string object with its value initialized to a copy of a substring of this object.

The substring is the portion of the object that starts at character position pos and spans len characters (or until the end of the string, whichever comes first).
*/
vector<string> split_by_find(const string& s, const string& delims){
    if (gDebugUtils) LOGD("input:[%s]", s.c_str());
    vector<string> output;
    string val;

    size_t start = 0;
    while(start != string::npos && start < s.length()){
        size_t pos = s.find_first_of(delims, start);
        if (gDebugUtils) LOGD("--> start=%zu, pos=%zu", start, pos);
        if (pos == start){
            output.push_back("");
            start = pos+1;
            continue;
        }
        if (pos == string::npos){
            output.push_back(s.substr(start));
            break;
        }
        /*if (pos > start && pos != string::npos)*/{
            output.push_back(s.substr(start, pos-start));//not include the delims found
            start = pos+1;
            continue;
        }
        LOGE("sth. wrong...");
    }
    return output;
}


int launch_cmd(const char* cmd_in, vector<string>& output){
    FILE *fpipe;
    string result;
    char val[1024] = {0};

    if (gDebugUtils) LOGD("cmd:[%s]", cmd_in);
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
            if (gDebugUtils) LOGD("[%s]", val);
            result += val;
            //result[result.size()-1]='\0';//tony:wrong!
            result.erase(result.end()-1);//remove tailing '\n'
            lrtrim_inplace(result);
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

bool startsWith(const string& s, const string& sub){
#if 0
    return s.find(sub)==0?true:false;
#else
    if (strncmp(s.c_str(), sub.c_str(), sub.size()) == 0)
        return true;
    return false;
#endif
}

int endsWith(const string& s, const string& sub){
#if 0
    return s.rfind(sub)==(s.length()-sub.length())?true:false;
#else
    if (s.size() < sub.size())
        return false;
    if (strncmp(s.c_str() + (s.size()-sub.size()), sub.c_str(), sub.size()) == 0)
        return true;
    return false;
#endif
}

vector<string> get_file_list(const char* filedir, const char* str_start=NULL, const char* str_end=NULL)
{
    vector<string> result;
    struct dirent *ptr;

    shared_ptr<DIR> dir(opendir(filedir), [](DIR* dir){ closedir(dir);});

    LOGD("filelist:");
    while((ptr=readdir(dir.get()))!=NULL)
    {
        //跳过'.'和'..'两个目录
        if((strlen(ptr->d_name) == 1 &&  ptr->d_name[0] == '.') ||
           (strlen(ptr->d_name) == 2 &&  ptr->d_name[0] == '.' && ptr->d_name[1] == '.' ))
            continue;

        if (str_start != NULL && !startsWith(ptr->d_name, str_start))
            continue;

        if (str_end != NULL && !endsWith(ptr->d_name, str_end))
            continue;

        LOGD("%s",ptr->d_name);
        result.push_back(string(ptr->d_name));
    }

    sort(result.begin(), result.end());

    LOGD("sorted file list:");
    int i = 0;
    for (auto s:result){
        LOGD("%02d:%s", i, s.c_str());
        i++;
    }
    return result;
}

vector<string> get_current_mac_addrs(){
    vector<string> output;
    if (launch_cmd("ifconfig | grep HWaddr | awk '{print $NF}'", output) != 0){
        LOGE("Fail to execute %s", __PRETTY_FUNCTION__);
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
        LOGD("get spentry error. [%s]", strerror(errno));
        return -1;
    }

    if(strcmp(sp->sp_pwdp, (char*)crypt(user_passwd, sp->sp_pwdp)) == 0) {
        LOGD("passwd check success");
        return 0;
    }
    else {
        LOGD("passwd check fail");
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

std::string get_date_str_sec()
{
    using namespace std::chrono;
    char tp[64] = { '\0' };
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);
    strftime(tp, 64, "%Y_%m_%d_%H_%M_%S", &tm);
    return std::string(tp);
}

std::string get_date_str_sec_from_time(time_t t)
{
    char tp[64] = {'\0'};
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

int exec_cmd(const char *shell_cmd)
{
    LOGD("[thread-id:%zu]exec: \"%s\"", pthread_self(), shell_cmd);
    pid_t status = system(shell_cmd);
    if (-1 == status) {
        LOGE("system error!");
        return -1;
    }
    LOGD("exit status value = [0x%x]", status);
    LOGD("exit status = [%d]", WEXITSTATUS(status));
    if (WIFEXITED(status)) {
        if (0 == WEXITSTATUS(status)) {
            LOGD("run system call <%s> successfully.", shell_cmd);
        }
        else {
            LOGD("run system call <%s> fail, script exit code: %d", shell_cmd, WEXITSTATUS(status));
        }
    }
    return WEXITSTATUS(status);
}

//execute the cmd in subprocess in sync mode and read the stdout
#define BUF_LEN (256)
int exec_popen(const char* cmd) {

    char buffer[BUF_LEN];
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe){
        LOGD("ERROR! fail to popen %s", cmd);
        throw std::runtime_error("popen() failed!");
    }
    while (!feof(pipe.get())) {
        if (fgets(buffer, BUF_LEN, pipe.get()) != nullptr)
            LOGD("%s",buffer);
    }
    return 0;
}

#define SHELL_BIN               ("bash")
int exec_shell_script(const char* script_dir, const char* script_file){
    string cmd = string(SHELL_BIN) + " " + script_dir + "/" + script_file;
    LOGD("start to execute: %s", script_file);
    cmd = cmd + " 2>&1 | sed  's/^/["+ script_file +"] /'";
    int ret = exec_cmd(cmd.c_str());
    if (ret){
        LOGD("ERROR: execute %s failed. retry.", script_file);
        return 1;
    }
    LOGD("execute %s success. continue", script_file);
    return 0;
}

int wait_system_boot_complete(const vector<string>& mountlist_input){
    auto mountlist = mountlist_input;
    if (mountlist.size() == 0){
        mountlist = vector<string>({
            "/data",
            "/mnt/runtime/default/emulated",
            "/storage/emulated",
            "/mnt/runtime/read/emulated",
            "/mnt/runtime/write/emulated"
        });
    }

    LOGD("start to check mount list");
    for (const auto& mnt:mountlist){
        LOGD("check for mount point: %s", mnt.c_str());
        while(true){
            string cmd="mountpoint -q " + mnt;
            LOGD("execute cmd: %s", cmd.c_str());
            if (exec_cmd(cmd.c_str()) == 0){
                LOGD("%s is mounted", mnt.c_str());
                break; //continue to next mnt point
            }
            else{
                LOGD("%s is NOT mounted, wait", mnt.c_str());
                sleep(10);
                continue;
            }
        }
    }
    sleep(10);
    return 0;
}

string format_string(const std::string& format, ...)
{
    va_list args;
    va_start (args, format);
    size_t len = std::vsnprintf(NULL, 0, format.c_str(), args);
    va_end (args);
    std::vector<char> vec(len + 1);
    va_start (args, format);
    std::vsnprintf(&vec[0], len + 1, format.c_str(), args);
    va_end (args);
    return &vec[0];
}

}


