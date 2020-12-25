#ifndef SECURE_H
#define SECURE_H

#include <string>
#include <stdio.h>
#include <string.h>
#include <shadow.h>
#include <unistd.h>
#include <pwd.h>

namespace utils{
int check_passwd(const char* name );
std::string getLoginUser();
int redirect_stdout_stderr(const char* fname);
int restore_stdout();
}

#endif
