#ifndef CAMERA_CTRL_H
#define CAMERA_CTRL_H

class CameraCtrl
{
    public:
        CameraCtrl(){};
        virtual ~CameraCtrl() {stop();};
        virtual int startAsync(long fetchIntervalms, size_t cache_len) = 0;
        virtual int startSync() = 0;
        virtual int stop(){};
        virtual int reset() = 0;
        virtual int flush() = 0;
        virtual int getFrame(cv::Mat& frame ) = 0;
        /*
        virtual int getWidth() = 0;
        virtual int getHeight() = 0;
        */

};

#endif
