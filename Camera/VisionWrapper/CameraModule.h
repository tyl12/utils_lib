#ifndef CAMERA_MODULE_H
#define CAMERA_MODULE_H
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

#include "CameraCtrl.h"
#include "CameraConfig.h"

#include "RingBuffer.h"

class CameraModule: public CameraCtrl
{
    private:
        bool bExit;
        bool bRequest;
        mutex mtx;
        condition_variable mCond;
        bool bAsyncMode;
        CameraConfig mConfig;
        long mIntervalms;
        int _loadConfig();
        int _checkConfig();
        int _commonInit();

        thread mThread;
        cv::VideoCapture mCap;

        int _getFrameAsync(cv::Mat& frame);
        int _getFrameSync(cv::Mat& frame);

        //cv::Mat mFrame;
        RingBuffer<cv::Mat> mQueue;

    public:
        virtual ~CameraModule();
        CameraModule(const CameraConfig& cfg);
        CameraModule(CameraModule&& cam) = default;
        //CameraModule(const CameraModule& cam) = default;
        CameraModule& operator=(const CameraModule & p) = delete;


        // <=0: keep looping
        // >0; interval between two fetches
        virtual int startAsync(long fetchIntervalms, size_t cache_len=2);
        virtual int startSync();
        virtual int stop();

        virtual int reset();
        virtual int flush();

        //0 for fail
        virtual int getFrame(cv::Mat& frame );

        virtual const CameraConfig& getCameraConfig() const{ return mConfig; }

        void die(const string& msg){
            auto idx    = mConfig.getVideoNodeIdx();
            cout<<"die E. camera=/dev/video/"<<idx<<endl;
            throw std::runtime_error(msg.c_str());
        }
};

#endif
