#ifndef CAMERA_CONTAINER_H
#define CAMERA_CONTAINER_H
#include <memory>

class CameraContainer
{
    private:
        vector<unique_ptr<CameraModule>> mCamTable;

    public:
        virtual ~CameraContainer();
        CameraContainer();
        CameraContainer(CameraContainer&& container){
            this->mCamTable.swap(container.mCamTable);
        };
        void addCamera(const string& key, unique_ptr<CameraModule> cam);
        //addCameraByConfig(const string& key, const CameraConfig& cfg);
        bool loadAllCameras();
        bool loadCamerasByUsage(const string& usage);
        bool loadCamerasByPos(const string& pos);

        void clear();
        void startAll();
        void stopAll();
        vector<cv::Mat> getFramesAll();
        vector<int> getCameraPosIndexPerLayer();

       static vector<CameraContainer> setupCameraContainer_withPos(vector<CameraConfig> cfgs);
};

#endif
