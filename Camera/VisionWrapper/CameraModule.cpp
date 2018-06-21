
#include <opencv2/opencv.hpp>			// C++

#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>

#include "CameraConfig.h"
#include "CameraModule.h"
#include "utils.h"
#include "v4l2_util.h"
#include "perf.h"


int CameraModule::_loadConfig()
{
    const char* videodevice = mConfig.getVideoDevice();
    int fd = _init_v4l2(videodevice);
    //cfg node
    _load_controls(fd, mConfig);

    //close node
    _close_v4l2(fd);
}
int CameraModule::_checkConfig()
{
    const char* videodevice = mConfig.getVideoDevice();
    int fd = _init_v4l2(videodevice);
    //cfg node
    _check_controls(fd, mConfig);

    //close node
    _close_v4l2(fd);
}

CameraModule::CameraModule(const CameraConfig& cfg)
{
    perf p("CameraModule ctor");
    mConfig = cfg;
    _loadConfig();
    _checkConfig();

}

int CameraModule::_commonInit()
{
    bExit = false;

    mCap.open(mConfig.getVideoNodeIdx());
    assert(mCap.isOpened());
    mCap.set(CV_CAP_PROP_FPS, mConfig.getFrameRate()); //TODO
    mCap.set(CV_CAP_PROP_FOURCC, CV_FOURCC('M','J','P','G'));
    mCap.set(CV_CAP_PROP_FRAME_WIDTH, mConfig.get_width());
    mCap.set(CV_CAP_PROP_FRAME_HEIGHT, mConfig.get_height());

    flush();
}

int CameraModule::startSync() //do flush internal
{
    LOGD("E");
    if (mCap.isOpened())
        LOGE("camera is already started");
    _commonInit();
    bAsyncMode = false;
}
// <=0: keep looping
// >0; interval between two fetches
int CameraModule::startAsync(long fetchIntervalms, size_t cache_len) //do flush internal
{
    LOGD("E, intervalms=%ld, queuesize=%ld", fetchIntervalms, cache_len);
    if (mCap.isOpened())
        LOGE("camera is already started");
    _commonInit();
    bAsyncMode = true;

    mIntervalms = fetchIntervalms;
    mQueue.set_capacity(cache_len);

    mThread = thread([&](){
        while(true){
            {
                unique_lock<mutex> _l(mtx);
                cv::Mat frame;

                if (fetchIntervalms > 0){
                    mCond.wait_for(_l, chrono::milliseconds(mIntervalms), [&](){ return (bExit || mQueue.isEmpty());});
                    if (bExit){
                        LOGD("thread exit");
                        return;
                    }
#ifdef DEBUG_CAMERA_MODULE
                    LOGD("mQueue.size=%ld",mQueue.size());
                    LOGD("loop thread ...");
#endif
                    //if (!mQueue.isFull()){
                        if (mCap.read(frame)){
                            mQueue.push_force(frame); //NOTES: ownership
                            mCond.notify_all();
                        }
                        else{
                            LOGE("videocapture read fail. retry...");
                        }
                        continue;
                    //}
                }
                else{
                    mCap.read(frame);
                    mQueue.push_force(frame);
                    mCond.notify_all();
                }
            }
        }
    });
}

int CameraModule::stop()
{
    LOGD("E");
    if (bAsyncMode){
        {
            unique_lock<mutex> _l(mtx);
            bExit = true;
            mCond.notify_all();
        }
        if (mThread.joinable())
            mThread.join();
    }

    mQueue.clear();
    mCap.release();
}

CameraModule::~CameraModule()
{
    LOGD("E");
    stop();
}

//0 for fail
int CameraModule::_getFrameSync(cv::Mat& frame){
#ifdef DEBUG_CAMERA_MODULE
    LOGD("E");
#endif
    return mCap.read(frame);
}
/*
int CameraModule::_getFrameAsync(cv::Mat& frame){
    unique_lock<mutex> _l(mtx);
    bRequest = true;
    mCond.notify_all();
    mCond.wait(_l, [&](){ return bRequest == false || bExit; });
    if (bExit || bRequest){
        return -1;
    }
    frame = mFrame.clone();
    return 0;
}
*/
//0 for fail
int CameraModule::_getFrameAsync(cv::Mat& frame){
#ifdef DEBUG_CAMERA_MODULE
    LOGD("E");
#endif
    unique_lock<mutex> _l(mtx);
    mCond.notify_all();
    mCond.wait(_l, [&](){ return bExit || !mQueue.isEmpty(); });
    if (bExit){
        return 0;
    }
    if (!mQueue.isEmpty()){
        //frame = mQueue.top().clone(); //NOTES: take the ownership
        frame = mQueue.top();
        mQueue.pop();
        return 1;
    }
    return 0;
}

//0 for fail
int CameraModule::getFrame(cv::Mat& frame)
{
    if ( bAsyncMode )
        return _getFrameAsync(frame);
    else
        return _getFrameSync(frame);
}


int CameraModule::reset()
{
}

//flush the buffers in circular queue
//flush the camera output queue
int CameraModule::flush()
{
    int skip = mConfig.getFrameSkip();
    perf p("flush");

    LOGD("E. skip %d frames", skip);
    unique_lock<mutex> _l(mtx);
    mQueue.clear();
    for (;skip>0;skip--){
        mCap.grab();
    }
    mCond.notify_all();
}

#ifdef DEBUG_CAMERA_CTRL
int main()
{
    vector<CameraConfig> cfgs;
    loadCameraJsonConfig("./camera_cfg.json", cfgs);
    CameraModule cam(cfgs[0]);
    cv::Mat frame;

#if 0
    for(int i =0; i<5;i++){
    RingBuffer<cv::Mat> buf(20);
    LOGD("----");
    cam.startSync();
    this_thread::sleep_for(std::chrono::seconds(1));
    cam.getFrame(frame);
    cam.flush();
    cam.getFrame(frame);
    cam.stop();
    }
#endif
    LOGD("---------------------");
#if 1
    cam.startAsync(30, 5);

    for(int i =0; i<10;i++){
        LOGD("----");
        cam.getFrame(frame); //should be 30ms per frame
        //this_thread::sleep_for(std::chrono::seconds(1));
    }
    cam.stop();

    cam.startAsync(30, 5);

    for(int i =0; i<10;i++){
        LOGD("----");
        cam.getFrame(frame); //should be 30ms per frame
        this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    cam.stop();

    cam.startAsync(90, 5);

    for(int i =0; i<10;i++){
        LOGD("----");
        cam.getFrame(frame); //should be 60ms per frame
        this_thread::sleep_for(std::chrono::milliseconds(60));
    }
    cam.stop();
#endif
    return 0;
}
#endif
