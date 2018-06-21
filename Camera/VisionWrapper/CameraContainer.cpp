
#include <opencv2/opencv.hpp>			// C++

#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>

#include "utils.h"
#include "CameraModule.h"
#include "CameraContainer.h"


CameraContainer::~CameraContainer()
{
    LOGE("E");
}
CameraContainer::CameraContainer(){
    LOGE("E");
}
void CameraContainer::addCamera(const string& key, unique_ptr<CameraModule> cam)//TODO
{
    mCamTable.push_back(move(cam));
}
//addCameraByConfig(const string& key, const CameraConfig& cfg);
bool CameraContainer::loadAllCameras()
{

}
bool CameraContainer::loadCamerasByUsage(const string& usage){}
bool CameraContainer::loadCamerasByPos(const string& pos){}

void CameraContainer::clear(){
    mCamTable.clear();
}
void CameraContainer::startAll(){ //TODO: add restart logic for stability recovery
    for(auto& cam:mCamTable){
        cam->startAsync(30, 5);
        //cam->startSync();
    }
}
void CameraContainer::stopAll(){
    for(auto& cam:mCamTable){
        cam->stop();
    }
}
vector<cv::Mat> CameraContainer::getFramesAll(){
    vector<cv::Mat> v;
    for(auto& cam:mCamTable){
        cv::Mat frame;
        if(cam->getFrame(frame)){
            v.push_back(frame);
        }
        else{
            LOGE("Fatail error: get frame failed");
            assert(0);
        }
    }
    return v;
}

vector<int> CameraContainer::getCameraPosIndexPerLayer(){
    vector<int> output;
    for(auto& cam:mCamTable){
        auto&& cfg = cam->getCameraConfig();
        int row, col;
        cfg.getPosParsed(row, col);
        output.push_back(col);
    }
    return output;
}

//capture only, ordered
vector<CameraContainer> CameraContainer::setupCameraContainer_withPos(vector<CameraConfig> cfgs){
    vector<CameraContainer> output;
    map<int, vector<pair<int, CameraConfig>>> m;
    int rows = 0;
    for(auto& cfg:cfgs){
        if (cfg.isUsageCapture()){
            int row, col;
            cfg.getPosParsed(row, col);
            LOGD("pos: (%d, %d)",row,col);
            auto&& itor = m.find(row);
            if (itor != m.end()){
                itor->second.push_back(make_pair(col, cfg));
            }else{
                vector<pair<int, CameraConfig>> vc;
                vc.push_back(make_pair(col, cfg));
                //m.insert(row, make_pair<int, vector<int>>(row, v));
                m.insert(make_pair(row, vc));
            }
        }
    }
    for (auto i:m){
        CameraContainer container;
        LOGD("shelf: %d", i.first);
        sort(i.second.begin(), i.second.end(), [](const pair<int, CameraConfig>& p1, const pair<int, CameraConfig>& p2){
                assert(p1.first != p2.first && "duplicated camera pos");
                return (p1.first < p2.first)? true: false;
            });
        for (auto s:i.second){
            LOGD("index:     %d", s.first);
        }
        for (auto s:i.second){
            const CameraConfig& cfg = s.second;
            container.addCamera(s.second.getPos(), unique_ptr<CameraModule>(new CameraModule(cfg)));
        }
        output.push_back(move(container));
    }
    return output;
}




#ifdef DEBUG_CAMERA_CONTAINER
int main()
{
    vector<CameraConfig> cfgs;
    loadCameraJsonConfig("./camera_cfg.json", cfgs);
    CameraContainer::setupCameraContainer_withPos(cfgs);
    return 0;

    CameraContainer cnt;
    for (auto cfg: cfgs){
        cnt.addCamera("key", unique_ptr<CameraModule>(new CameraModule(cfg)));
    }
    cnt.startAll();

    for (int i =0 ; i< 20; i++){
        auto&& frames = cnt.getFramesAll();
        LOGD("frames cnt= %ld", frames.size());
        int s = 0;
        for (const auto& f: frames){
            char c_name[256];
            sprintf(c_name, "%d-%d.jpg", s, i);
            cv::imwrite(c_name, f);
            s++;
        }
    }
    cnt.stopAll();
}
#endif
