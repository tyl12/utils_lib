//
// Created by david on 17-12-8.
//

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <vector>
#include <string>
#include <utility>
#include <iostream>
#include <fstream>
#include <linux/videodev2.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "ports_map_videos.h"

using namespace std;

#define BUF_SIZE 256
//#define MAX_PORTS 32

vector<pair<string, string>> getPortsMapVideos() {
    vector<pair<string, string>> port_node_table;
    FILE* videos_info;
    FILE* port_info;
    size_t len = BUF_SIZE;
    int status;

    // we should allocate memory to `videos_output` before invoking `getline(..)`,
    // otherwise, you may encounter segmentation fault (memory conflict)
    // char* videos_output = NULL;
    char* videos_output = (char*) malloc(BUF_SIZE);
    char* port_output = (char*) malloc(BUF_SIZE);
    char* find_port_cmd = (char*) malloc(BUF_SIZE);
    memset(videos_output, 0, BUF_SIZE);
    memset(port_output, 0, BUF_SIZE);
    memset(find_port_cmd, 0, BUF_SIZE);

    videos_info = popen("ls /dev | grep video", "r");

    if (videos_info == NULL) {
        printf("error occurs while opening pipe...\n");
        exit(EXIT_FAILURE);
    }
    /* `fgets(..)` also works (compare to `getline(..)`) */
    while (getline(&videos_output, &len, videos_info) != -1) {
        if (videos_output[strlen(videos_output)-1] == '\n')
            videos_output[strlen(videos_output)-1] = '\0';

#ifdef DEBUG_CAMERA_PCI
        printf("handle node: %s\n",videos_output);
#endif

        sprintf(find_port_cmd,
                //"udevadm info --name=/dev/video%d | sed -n 's/P:\\s.*\\/\\(.*\\)\\/video4linux.*/\\1/p'",
                "udevadm info --name=/dev/%s | sed -n 's/P:\\s.*\\/\\([0-9.-]\\+\\):[0-9.-]*\\/video4linux.*/\\1/p'",
                videos_output);

#ifdef DEBUG_CAMERA_PCI
        printf("exec cmd: %s\n",find_port_cmd);
#endif

        port_info = popen(find_port_cmd, "r");
        if (port_info == NULL)
        {
            printf("error occurs while opening pipe...\n");
            exit(EXIT_FAILURE);
        }

        // read info from command `udevadm` to `output2`, such as `1-10:1.0`
        fgets(port_output, BUF_SIZE, port_info);

        // update `ports` array (the array index starts with 0, the physical
        // port index starts 1, so the port number should abstract 1)
        if (port_output[0] != '\0'){
            if (port_output[strlen(port_output)-1] == '\n')
                port_output[strlen(port_output)-1] = '\0';
#ifdef DEBUG_CAMERA_PCI
            printf("cmd result: %s\n", port_output);
#endif
            port_node_table.push_back(make_pair(string(port_output), string("/dev/") + videos_output));
#ifdef DEBUG_CAMERA_PCI
            cout<<port_output<<" -> " << string("/dev/") + videos_output<<endl;
#endif
        }

        status = pclose(port_info);
        if (status == -1)
        {
            printf("ERROR: failure to close pipe...\n");
            exit(EXIT_FAILURE);
        }
    }

    // a stream opened by popen(..) should be closed by pclose(..)
    status = pclose(videos_info);
    if (status == -1)
    {
        printf("ERROR: failure to close pipe...\n");
        exit(0);
    }

    free(videos_output);
    free(port_output);
    free(find_port_cmd);

    return port_node_table;
}

vector<pair<string, string>> wrapper_pciport_videonode()
{
    auto port_node_table = getPortsMapVideos();

    // print port and corresponding video number
    for (auto s: port_node_table){
        cout<<"***** Found: pci-port: "<<s.first<<",  video-node: "<<s.second<<endl;
    }
    return port_node_table;
}

int initAllCamerasWithShell(){
    try{
        ifstream f("data_xm/v4l2.cfg");
        if (!f) // if file doesn't exist, EXIT
            cerr<<"data_xm/v4l2.cfg file not found. skip."<<endl;
        else{
            //vector<string> lines;
        }
    }
    catch (exception &e) { cerr << "exception: " << e.what() << "\n"; return 1;}
    catch (...) { cerr << "unknown exception \n"; return 1;}
    return 0;
}

static int sendV4l2ShellCmd(int portid, string videonode)
{
    string cfg = string("data_xm/port-") + to_string(portid) + string("-shell.cfg");
    try{
        ifstream f(cfg);
        if (!f) // if file doesn't exist, EXIT
            cerr<<cfg<<" not found. skip."<<endl;
        else{
            cout<<"send v4l2 shell config"<<endl;

            string line;
            while(getline(f, line)){ // keep going until eof or error
                //lines.push_back(line); // add line to lines

                auto pos = line.find( "#" ) ;
                if( pos == 0 ) continue;
                else if( pos != string::npos ) line.erase(pos) ;
                if (line.empty()) continue;

                line += string(" -d ") + videonode;
                cout<<"exec cmd:"<<line<<endl;

                //FILE* fp = popen("v4l2-ctl --set-fmt-video=width=640,height=480,pixelformat=MJPG -d /dev/video0", "r");
                FILE* fp = popen(line.c_str(), "r");
                if(NULL == fp){
                    cerr<<"fail to execute popen"<<endl;
                }
                else{
                    char result[1024];
                    while(fgets(result, sizeof(result), fp) != NULL){
                        cout<<"    "<<result;
                    }
                    pclose(fp);
                }
            }

        }
    }
    catch (exception &e) { cerr << "exception: " << e.what() << "\n"; return 1;}
    catch (...) { cerr << "unknown exception \n"; return 1;}
    return 0;
}

static int sendV4l2Cmd(int portid, string videonode)
{
    string cfg = string("data_xm/port-"+to_string(portid)+".cfg");
    try{
        ifstream f(cfg);
        if (!f) // if file doesn't exist, EXIT
            cerr<<cfg<<" not found. skip."<<endl;
        else{
            cout<<"send v4l2 config"<<endl;
            //TODO: send v4l2 command
        }
    }
    catch (exception &e) { cerr << "exception: " << e.what() << "\n"; return 1;}
    catch (...) { cerr << "unknown exception \n"; return 1;}
    return 0;
}

#ifdef DEBUG_CAMERA_PCI
//for debug
int main()
{
    auto resultvect = wrapper_pciport_videonode();
    cout<<endl<<"All pci-port/video-node mapping:"<<endl;
    for (auto p:resultvect){
        string portid = p.first;
        string videonode = p.second;
        cout<<"pci-port: "<<portid<<" video-node: "<<videonode<<endl;
    }
    return 0;
}
#endif
