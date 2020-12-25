
#include <string>
#include <stdio.h>
#include <string.h>
#include <shadow.h>
#include <unistd.h>
#include <pwd.h>
#include "utils.h"

using namespace std;

namespace utils{

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
