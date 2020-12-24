#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <regex>
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

#include <sys/time.h>
#include <math.h>
#include <time.h>

//#include <boost/ref.hpp>
//#include <boost/asio.hpp>
//#include <boost/bind.hpp>
//#include <boost/function.hpp>

using namespace std;

namespace utils{

#define LOGD(fmt, ...)  do{ printf("%s DBG: %s:%d: " fmt "\n", get_ts().c_str(), __PRETTY_FUNCTION__,__LINE__, ##__VA_ARGS__); } while(0)
#define LOGE(fmt, ...)  do{ printf("%s ERR: %s:%d: " fmt "\n", get_ts().c_str(), __PRETTY_FUNCTION__,__LINE__, ##__VA_ARGS__); } while(0)

//#define LOGD(fmt, ...)  do{ printf("DEBUG: %s:%d: "fmt"\n", __FUNCTION__,__LINE__, __VA_ARGS__); } while(0)
//#define LOGE(fmt, ...)  do{ printf("ERROR: %s:%d: "fmt"\n", __FUNCTION__,__LINE__, __VA_ARGS__); } while(0)

#define LOGD2(fmt, args...) fprintf (stdout, "%s DBG: %s:%d: " fmt "\n", get_ts().c_str(), __PRETTY_FUNCTION__,__LINE__,args)
#define LOGE2(fmt, args...) fprintf (stderr, "%s ERR: %s:%d: " fmt "\n", get_ts().c_str(), __PRETTY_FUNCTION__,__LINE__,args)

template<typename ... Args>
string string_format( const std::string& format, Args ... args )
{
    size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1; // Extra space for '\0'
    std::unique_ptr<char[]> buf( new char[ size ] );
    snprintf( buf.get(), size, format.c_str(), args ... );
    return string( buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
}

#define string_format2(format, args...)\
    ({ \
        size_t size = snprintf( nullptr, 0, format , args ) + 1; \
        std::unique_ptr<char[]> buf( new char[ size ] ); \
        snprintf( buf.get(), size, format, args); \
        string( buf.get(), buf.get() + size - 1 );\
    })


bool startsWith(const string& s, const string& sub);
bool endsWith(const string& s, const string& sub);

bool startsWithIgnoreCase(const string& s, const string& sub);
bool endsWithIgnoreCase(const string& s, const string& sub);

bool toUpperCaseInplace(string& s);
bool toLowerCaseInplace(string& s);

/*
 * 删除字符串头尾的空格字符,
 * input: inplace 操作，会改变输入字符串,
 * output: 输入字符串经过trim操作之后的结果字符串的引用
 */

string& ltrim_inplace(string &s);
string& rtrim_inplace(string &s);
string& lrtrim_inplace(string &s);

string ltrim(const string &s);
string rtrim(const string &s);
string lrtrim(const string &s);

vector<string> split_by_delim(const string& line, const char ch);
vector<string> split_by_delims(const string& str, const string& delims);
vector<string> split_by_regex_iterator(const string& s, const regex& pattern);
vector<string> split_by_regex_search(const string& s, const regex& delims);
vector<string> split_by_find(const string& s, const string& delims);

string merge_intvector_to_string_with_traits(vector<int> data);

int launch_cmd(const char* cmd, vector<string>& output);
vector<string> get_current_mac_addrs();

int check_passwd(const char* name = NULL);
string getLoginUser();

string get_ts(void);

int64_t get_time_ms();
string get_date_str_sec();
std::string get_date_str_sec_from_time(time_t t);

int redirect_stdout_stderr(const char* fname);
int restore_stdout();

int exec_popen(const char* cmd);
int exec_shell_script(const char* script_dir, const char* script_file);

int wait_android_boot_complete(const vector<string>& mountlist_input);

string format_string(const std::string& format, ...);

//utils class used to identify object type
template<typename T>
class VersionError;
//usage: VersionError<decltype(m)> a;


}
#endif



