#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <queue>
#include <fstream>
#include <time.h>
#include <thread>
#include <atomic>
#include <mutex>              // std::mutex, std::unique_lock
#include <condition_variable> // std::condition_variable
//#include <regex>
#include <future>
#include <cstdlib>
#include <unordered_map>
#include <algorithm>

//#include <opencv2/opencv.hpp>			// C++

#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>

#include "opencv2/core/version.hpp"

//the API below is exposed to JNI caller in form of .so lib
#include <memory>
#include <type_traits>
#include "CameraModule.h"
#include "CameraModule.h"
#include "utils.h"
#include "thread_guard.h"

//NOTES: the f**king x11 head file define Bool->int, which will cause rapidjson compile failure.
//WA: include x11 after all others.
#include <X11/Xlib.h>


using namespace std;

static bool is_gui_present = true;
static vector<unique_ptr<CameraModule>> node_table;

static mutex cam_lock;
static const char CAMERA_TEST_DIR[]="/tmp/camera_test";

#define ENABLE_OVERVIEW_WINDOW

//enableAsync = false:
//loop all cameras: open one camera, read,..., close one camera
//
//enableAsync = true:
//open all camera, asycn read, wait all cameras read done, merge all frames on one image, draw the image, ..., close all cameras
int test_scan(bool enableAsync=false, float scale=0.5, int framecnt=20*30){
    if (!is_gui_present){
        LOGD("No gui present, return directly");
    }

    vector<future<cv::Mat>> async_tasks;
    auto camCnt = node_table.size();

    async_tasks.reserve(node_table.size());
    if (enableAsync)
        for (auto& node:node_table) node->startSync();

    int row;
    int col;
    if (camCnt >=6){
        row = camCnt/2;
        col = 2;
    }else{
        row = camCnt;
        col = 1;
    }
    LOGD("Total camera cnt= %ld, row= %d, col= %d", camCnt, row, col);

    const auto& cfg = node_table[0]->getCameraConfig();
    int cam_w = cfg.get_width();
    int cam_h = cfg.get_height();

    //int div = row>col?row:col;
    //int resized_w = cam_w/div;
    //int resized_h = cam_h/div;
    int resized_w = cam_w * scale;
    int resized_h = cam_h * scale;

    cv::Mat win_mat(cv::Size(resized_w * col, resized_h * row), CV_8UC3);

    //setup window for later access
    if (is_gui_present){
        for (auto& node:node_table){
            const auto& cfg = node->getCameraConfig();
            string desc = cfg.getDetailInfo();
            cv::namedWindow(desc.c_str());
        }
        cv::namedWindow("overview");
    }

    auto launch_flag=launch::async;
    if (enableAsync){
        launch_flag=launch::async;
    }
    else{
        launch_flag=launch::deferred;
    }
    int count = framecnt;
    while(count-- >= 0){
        for (auto& node:node_table){
            auto task = async(launch_flag, [=,&node](){
                if (!enableAsync)
                    node->startSync();
                const auto& cfg = node->getCameraConfig();
                string winName = cfg.getDetailInfo();

                //cout<<__FUNCTION__<<": handle "<<winName<<endl;

                cv::Mat frame;
                for (int i=0;i<10;i++){
                    if (!node->getFrame(frame)){
                        LOGD("%s: Failed to fetch frame", winName.c_str());
                        if (!enableAsync)
                            node->stop();
                        return frame;
                    }
                }
                cv::putText(frame,
                            winName.c_str(),
                            cv::Point(5,50), // Coordinates
                            cv::FONT_HERSHEY_COMPLEX_SMALL, // Font
                            2.0, // Scale. 2.0 = 2x bigger
                            cv::Scalar(0,255,0), // BGR Color
                            1, // Line Thickness
                            CV_AA); // Anti-alias


                cv::Mat scaledMat;
                cv::Size size(resized_w, resized_h);
                cv::resize(frame, scaledMat, size);

                unique_lock<mutex> l(::cam_lock);
                cv::imshow(winName.c_str(), frame);
                cv::waitKey(1);
                //cout<<winName<<": post frame done" << endl;
                if (!enableAsync)
                    node->stop();

                return scaledMat;
            });
            async_tasks.push_back(move(task));
        }
        int idx = 0;
        for (auto&& task: async_tasks){
            int row_idx = idx/col;
            int col_idx = idx-row_idx*col;

            //cout<<"row_idx="<<row_idx<<" , col_idx="<<col_idx<<endl;
            //cout<<resized_w*col_idx<< " , " << resized_h*row_idx << " , " << resized_w -1 << " , "<<  resized_h -1<<endl;

            cv::Mat frame = task.get();
            if (frame.rows>0 && frame.cols>0)
                frame.copyTo(win_mat(cv::Rect(
                            resized_w*col_idx, resized_h*row_idx,
                            resized_w , resized_h
                            )));
            idx++;
        }
        async_tasks.clear();

        unique_lock<mutex> l(::cam_lock);
        cv::imshow("overview", win_mat);
        cv::waitKey(1);
        //cout<<"overview: post frame done" << endl;
    }

    //destroy window
    if (is_gui_present){
        for (auto& node:node_table){
            const auto& cfg = node->getCameraConfig();
            string desc = cfg.getDetailInfo();
            cv::destroyWindow(desc.c_str());
        }
        cv::destroyWindow("overview");
    }

    if (enableAsync)
        for (auto& node:node_table) node->stop();
    return 0;
}

//enableAsync = false:
//loop all cameras: open one camera, read,..., close one camera
//enableAsync = true:
//open all camera, asycn read,..., close all cameras
int test_liveview(bool enableAsync=false, int framecnt=20*30){
    vector<utils::thread_guard> liveview_threads;

    liveview_threads.reserve(node_table.size());
    if (enableAsync)
        for (auto& node:node_table) node->startSync();

    for (auto& node:node_table){
        utils::thread_guard t(thread([=, &node](){
            if (!enableAsync)
                node->startSync();

            const auto& cfg = node->getCameraConfig();
            string winName = cfg.getDetailInfo();
            LOGD("%s: handle %s", __FUNCTION__, winName.c_str());

            int count = framecnt;
            cv::Mat frame;
            if (is_gui_present){
                unique_lock<mutex> l(::cam_lock);
                cv::namedWindow(winName.c_str());
            }
            while(count-- >= 0){
                if (!node->getFrame(frame)){
                    LOGD("%s: Failed to fetch frame", winName.c_str());
                    continue;
                }

                cv::putText(frame,
                            winName.c_str(),
                            cv::Point(5,50), // Coordinates
                            cv::FONT_HERSHEY_COMPLEX_SMALL, // Font
                            2.0, // Scale. 2.0 = 2x bigger
                            cv::Scalar(0,255,0), // BGR Color
                            1, // Line Thickness
                            CV_AA); // Anti-alias

                if (is_gui_present){
                    unique_lock<mutex> l(::cam_lock);
                    cv::imshow(winName.c_str(), frame);
                    cv::waitKey(1);
                }
            }
            if (is_gui_present){
                unique_lock<mutex> l(::cam_lock);
                cv::destroyWindow(winName.c_str());
            }

            if (!enableAsync){
                //if close outside the thread, must join the thread before close
                node->stop();
            }
        }));
        if (enableAsync)
            liveview_threads.emplace_back(move(t));
    }

    liveview_threads.clear();
    if (enableAsync)
        for (auto& node:node_table) node->stop();
    return 0;
}

//enableAsync = false:
//loop all cameras: open one camera, read,..., close one camera
//enableAsync = true:
//open all camera, asycn read,..., close all cameras
int test_capture(bool enableAsync=false, int framecnt=20*30){
    vector<utils::thread_guard> capture_threads;

    capture_threads.reserve(node_table.size());
    if (enableAsync)
        for_each(node_table.begin(), node_table.end(), [](unique_ptr<CameraModule>& node){ node->startSync(); });

    char cmd[128];
    sprintf(cmd, "mkdir -p %s/last; mkdir -p %s/internal", CAMERA_TEST_DIR, CAMERA_TEST_DIR);
    system(cmd);
    for (auto& node:node_table){
        utils::thread_guard t(thread([=, &node](){
            if (!enableAsync)
                node->startSync();

            const auto& cfg = node->getCameraConfig();
            string winName = cfg.getDetailInfo();
            LOGD("%s: handle %s", __FUNCTION__, winName.c_str());

            int count = framecnt;
            auto ts = std::chrono::system_clock::now();
            cv::Mat frame;
            while(count-- >= 0){
                if (!node->getFrame(frame)){
                    LOGD("Failed to fetch frame");
                    continue;
                }

                cv::putText(frame,
                            winName.c_str(),
                            cv::Point(5,50), // Coordinates
                            cv::FONT_HERSHEY_COMPLEX_SMALL, // Font
                            2.0, // Scale. 2.0 = 2x bigger
                            cv::Scalar(0,255,0), // BGR Color
                            1, // Line Thickness
                            CV_AA); // Anti-alias

#if 1
                char fName[128];
                sprintf(fName, "%s/internal/%s_idx%d.jpeg", CAMERA_TEST_DIR, winName.c_str(), (framecnt-count));
                //cout<<"write output image to: "<<fName<<endl;
                imwrite(fName, frame);
#endif
            }
            char fName[128];
            sprintf(fName, "%s/last/%s.jpeg", CAMERA_TEST_DIR, winName.c_str());
            LOGD("write output image to: %s", fName);
            imwrite(fName, frame);
            auto te = std::chrono::system_clock::now();
            std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(te - ts);
            int64_t duration_ms = ms.count();
            int fps = static_cast<int>(framecnt * 1000 / duration_ms);

            LOGD("%s fps: %d", winName.c_str(), fps);
            if (!enableAsync)
                node->stop();

        }));
        if (enableAsync)
            capture_threads.emplace_back(move(t));
    }

    capture_threads.clear();
    if (enableAsync)
        for_each(node_table.begin(), node_table.end(), [](unique_ptr<CameraModule>& node){ node->stop(); });
    return 0;
}
bool myfunction ( unique_ptr<CameraModule> const & i, unique_ptr<CameraModule> const& j) { return true; }

int main (int argc, char* argv[])
{
    LOGD("Start SelfTest");
    char ch;
    bool useAsync = false;
    int testCycle = 10*30;
    bool enableScan = false;
    float scale = 0.5;
    bool enableLiveview = false;
    bool enableCapture = false;
    int arg_cnt = 0;

    while((ch = static_cast<char>(getopt(argc, argv, "d:n:t:s:"))) != -1){
        ++arg_cnt;
        LOGD("optind = %d, optopt = %d\n", optind, optopt);
        switch(ch){
            case 'd':
                LOGD("optidx:%d, optkey:%c, outval:%s\n", optind, ch, optarg);
                if (strcmp(optarg, "async") == 0)
                    useAsync = true;
                else if (strcmp(optarg, "sync") == 0)
                    useAsync = false;
                else{
                    LOGD("invalid parameters, support only \" -d async/sync ");
                    return -1;
                }
                break;
            case 'n':
                LOGD("optidx:%d, optkey:%c, outval:%s\n", optind, ch, optarg);
                try{
                    testCycle = stoi(string(optarg));
                }catch(exception e){
                    LOGD("Invalid -n parameter specified.");
                    return -1;
                }
                break;

            case 't':
                LOGD("optidx:%d, optkey:%c, outval:%s\n", optind, ch, optarg);
                if (strcmp(optarg, "all") == 0){
                    enableScan = true;
                    enableLiveview = true;
                    enableCapture = true;
                }
                else if (strcmp(optarg, "scan") == 0)
                    enableScan = true;
                else if (strcmp(optarg, "liveview") == 0)
                    enableLiveview = true;
                else if (strcmp(optarg, "capture") == 0)
                    enableCapture = true;
                else{
                    LOGD("invalid parameters, support only \" -t all/scan/liveview/capture");
                }
                break;

            case 's':
                LOGD("optidx:%d, optkey:%c, outval:%s\n", optind, ch, optarg);
                try{
                    scale = stof(string(optarg));
                }catch(exception& e){
                    LOGD("Invalid -s parameter specified.");
                    return -1;
                }
                break;
        }
    }

    if (arg_cnt == 0)
    {
        LOGD("usage: -d async/sync -n 300 -t all/scan/liveview/capture -s 0.5");
        return -1;
    }

    LOGD("OpenCV version: %d . %d . %d", CV_MAJOR_VERSION, CV_MINOR_VERSION, CV_SUBMINOR_VERSION);

    if (NULL == getenv("DISPLAY"))
        is_gui_present = false;

    if (is_gui_present)
        XInitThreads();

    //1.
    LOGD("create selftest output dir: %s",CAMERA_TEST_DIR);
    string cmd = string("mkdir -p ") + string(CAMERA_TEST_DIR);
    system(cmd.c_str());

    //2.
    //bool sendcfg = true;
    vector<CameraConfig> cfgs;
    loadCameraJsonConfig("./camera_cfg.json", cfgs);


    node_table.reserve(cfgs.size());
    for (auto&& cam:cfgs){
        if (cam.isUsageCapture()){
            node_table.emplace_back(unique_ptr<CameraModule>(new CameraModule(cam)));
        }
    }

//    std::sort(node_table.begin(), node_table.end(), myfunction);
    std::sort(node_table.begin(), node_table.end(), [](const unique_ptr<CameraModule>& first,const unique_ptr<CameraModule>& second){
        const auto& cfg_first = first->getCameraConfig();
        string strFirst = cfg_first.getDetailInfo();

        const auto& cfg_second = second->getCameraConfig();
        string strSecond = cfg_second.getDetailInfo();

        return strFirst.compare(strSecond)<0;
    });

    for (auto& p:node_table){
        const auto& cfg = p->getCameraConfig();
        string str = cfg.getDetailInfo();
        LOGD("pos=%s", str.c_str());
    }


    //use opencv:
    //testcase: display all camera liveview
    //testcase: parallel still capture, 600frames, 20sec for 30fps
    //testcase: serial still capture, 30 frames, 1sec
    if (enableScan){
        LOGD(">>>>>>>>>>>>>>>>>>>>>>>>>test scan<<<<<<<<<<<<<<<<<<<<<<<<<<");
        test_scan(useAsync, scale, testCycle);
    }
    if (enableLiveview){
        LOGD(">>>>>>>>>>>>>>>>>>>>>>>>>test liveview<<<<<<<<<<<<<<<<<<<<<<<<<<");
        test_liveview(useAsync, testCycle);
    }
    if (enableCapture){
        LOGD(">>>>>>>>>>>>>>>>>>>>>>>>>test capture without liveview<<<<<<<<<<<<<<<<<<<<<<<<<<");
        test_capture(useAsync, testCycle);
    }

    return 0;
}

