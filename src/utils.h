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

using namespace std;

namespace utils{
/*
 * 删除字符串头尾的空格字符,
 * input: inplace 操作，会改变输入字符串,
 * output: 输入字符串经过trim操作之后的结果字符串的引用
 */
string& trim(string &s);
vector<string> split_by_delim(const string& line, const char ch);

int launch_cmd(const char* cmd, vector<string>& output);
vector<string> get_current_mac_addrs();

bool isDebugEnv();

int check_passwd(const char* name = NULL);
string getLoginUser();

int64_t get_time_ms();
string get_date_sec();

int redirect_stdout_stderr(const char* fname);
int restore_stdout();

}
