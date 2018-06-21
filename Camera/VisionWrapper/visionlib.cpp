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
#include <regex>
#include <future>
#include <cstdlib>
#include <unordered_map>
#include <set>

#ifdef _WIN32
#define OPENCV
#endif

#include "perf.h"
#include "yolo_v2_class.hpp"	// imported functions from DLL
#include "Tracking.h"
#include "Fusion.h"
#include "ecc.h"
#include "semaphore.h"

#define SAVE_PRE
#ifdef OPENCV
#include <opencv2/opencv.hpp>			// C++

#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>

#include "opencv2/core/version.hpp"
#ifndef CV_VERSION_EPOCH
#include "opencv2/videoio/videoio.hpp"
#define OPENCV_VERSION CVAUX_STR(CV_VERSION_MAJOR)""CVAUX_STR(CV_VERSION_MINOR)""CVAUX_STR(CV_VERSION_REVISION)
#pragma comment(lib, "opencv_world" OPENCV_VERSION ".lib")
#else
#define OPENCV_VERSION CVAUX_STR(CV_VERSION_EPOCH)""CVAUX_STR(CV_VERSION_MAJOR)""CVAUX_STR(CV_VERSION_MINOR)
#pragma comment(lib, "opencv_core" OPENCV_VERSION ".lib")
#pragma comment(lib, "opencv_imgproc" OPENCV_VERSION ".lib")
#pragma comment(lib, "opencv_highgui" OPENCV_VERSION ".lib")
#endif

#define DEBUG_INFO 0
std::mutex mtx_imshow;
#endif	// OPENCV

//the API below is exposed to JNI caller in form of .so lib
#include <memory>
#include <type_traits>
#include "vision.h"
#include "ecc.h"
#include "CameraModule.h"
#include "ports_map_videos.h"
#ifndef DEBUG_NO_JNI_CALLBACK
#include "../jni/jniapi.h"
#endif

//NOTES: the f**king x11 head file define Bool->int, which will cause rapidjson compile failure.
//WA: include x11 after all others.
#ifdef DEBUG_NO_JNI_CALLBACK
#include <X11/Xlib.h>
#endif


using std::cout;
using std::cerr;
using std::endl;

//#include "JOut.h"
#define MAX_DETECTOR 2

class VisionLib;
static std::vector<std::unique_ptr<VisionLib>> vision_table;
static std::vector<CamCtrl> stream_table;
static std::vector<CamCtrl> capture_table;

static std::thread t_compress;

static std::mutex detectResultLock;
static std::mutex detectLock;
static Semaphore detector_sem(MAX_DETECTOR);
static Semaphore ss_sem(1);

struct ShelfInfo
{
    int layerCount;
    std::vector<int> layerCamCount;
};

struct DetectionImage
{
    int layerIndex; // starts with 0
    int camIndex; // starts with 0
    int type;
    cv::Mat frame;
};

namespace yolo {

    cv::Mat crop_frame_func(cv::Mat src, std::vector<int>& crop_rect) {
        auto dst = cv::Mat(src, cv::Rect_<int>(crop_rect[0], crop_rect[1], crop_rect[2], crop_rect[3])).clone();
        return dst;
    }

    cv::Mat rotate_frame_func(cv::Mat src, int angle) {
#if 1
        int cvang=0;
        switch (angle){
            case 90:
                cvang=cv::ROTATE_90_CLOCKWISE;
                break;
            case 180:
                cvang=cv::ROTATE_180;
                break;
            case 270:
                cvang=cv::ROTATE_90_COUNTERCLOCKWISE;
                break;
            default:
                break;
        }
        //could support inplace convertion. with better perf.
        cv::rotate(src, src, cvang);
        return src;
#else
        using namespace cv;
        Mat dst;
        if(angle != 0 || angle != 360)
        {
            int center_y = src.rows / 2;
            int center_x = src.cols / 2;

            Point_<int> center = { center_x, center_y };
            Mat rotation = getRotationMatrix2D(center, angle, 1);

            Rect bbox = RotatedRect(center, src.size(), angle).boundingRect();
            rotation.at<double>(0, 2) += bbox.width / 2.0 - center_x;
            rotation.at<double>(1, 2) += bbox.height / 2.0 - center_y;
            warpAffine(src, dst, rotation, bbox.size());
        }
        return dst;
#endif
    }

    namespace data {
        using namespace std;

        bool gIsGuiPresent=false;
        string gTimeStamp; //updated once startvision() called
        string gDumpFolder; //used to save dumped video/ts/fusion output
        //string gStaticPicFolder; //used to save door open/close jpeg
        int gSampleInterval=0;
        bool gEnableDump = true;

        /* detection probability threshold, which means if `det_prob < thres`, we would discard this detection */
        int thres = 0.24;

        /* use to output `pfusion` debug info */
        ofstream fusion_dbg_fp;

        /* all stream and capture cam threads share the only `Fusion` (so, there is a need to access by mutex) */
        unique_ptr<Fusion> pfusion;

        /* assign a constant number to the specified camera, that is
         * {
         *     "left top": 0,
         *     "right top": 1,
         *     "left bottom": 2,
         *     "right bottom": 3
         * } */
        vector<int> tracking_map;
        auto init_tracking_map = [&tracking_map]() {
            tracking_map.resize(64);
            tracking_map[2] = 0;
            tracking_map[3] = 1;
            tracking_map[1] = 2;
            tracking_map[4] = 3;
            tracking_map[12] = 0;
            tracking_map[6]  = 1;
            tracking_map[13] = 2;
            tracking_map[5]  = 3;
        };

        /* a map maintains the relationship between physical port and specified camera */
        vector<pair<string, string>> ports_map_videos;
        auto init_ports_map_videos = [&ports_map_videos]() {
            ports_map_videos = wrapper_pciport_videonode();
        };
        auto get_port_id = [&ports_map_videos](int cam_id) {
            string video_name = string("/dev/video") + to_string(cam_id);
            for (auto&& item: ports_map_videos)
                if (item.second.compare(video_name) == 0)
                    return item.first;
            return string("-1");
        };


        /* there is one good-names list among this program,
         * which is dedicated used by algorithm lib
         */
        vector<string> obj_names;
        auto init_obj_names = [&obj_names]() {
            ifstream ifs("data_xm/voc.names");
            if (!ifs.is_open()) {
                cerr << "file `./data_xm/voc.names` do not exist...\n";
                exit(1);
            }
            string line;
            while ( getline(ifs, line) )
                obj_names.push_back(line);
        };



        /* map good name to corresponding QR code */
        map<string, vector<string> > goodsmap;
        auto init_names = [&goodsmap](){

            ifstream ifs("data_xm/all.names");
            if (!ifs.is_open()) {
                cerr << "file `./data_xm/all.names` do not exist...\n";
                exit(1);
            }

            /* for each line, names distinguished by comma (essential), tab (optional), or blankspace (optional) */
            regex delim(",\\s*\\t*");
            sregex_token_iterator end;
            string line;
            cout<<"initialize goodsMap:"<<endl;
            while ( getline(ifs, line) ) {
                string name;
                sregex_token_iterator it(line.begin(), line.end(), delim, -1);
                string key;
                string code69;
                string china;
                int cnt = 0;
                bool valid = true;
                while (it != end) {
                    if (cnt == 0)
                        key = *it;
                    else if (cnt == 1)
                        code69 = *it;
                    else if (cnt == 2)
                        china = *it;
                    else{
                        cerr<<"Invalid cnt num, pls check all.names, skip current line: "<< line <<endl;
                        valid = false;
                        break;
                    }
                    cnt++;
                    it++;
                }
                if (valid){
                    cout<<key<<" : " <<code69 << " : " << china <<endl;
                    vector<string> val;
                    val.push_back(code69);
                    val.push_back(china);
                    goodsmap.insert(pair<string, vector<string>>(key, val));
                    //goodsmap.insert(std::pair<string, string>(name, code));
                }
            }

        };

        //get 69 code from goodsmap
        auto find_good_code_withkey = [&goodsmap](string key) {
            auto it = goodsmap.find(key);
            string code69("");
            if (it != goodsmap.end()){
                code69 = it->second[0];
            }
            else{
                cerr<<"ERROR: goods name not found in goodsmap: " << key<<endl;
                exit(1);
            }
            return code69;
        };

        auto find_good_code = [&goodsmap](int index) {
            string mark_name = obj_names[index];
            return find_good_code_withkey(mark_name);
        };

        auto find_ch_name = [&](int index) {
            string mark_name = obj_names[index];
            auto res = goodsmap.find(mark_name);
            return res->second[1];
        };
        namespace static_cam
        {
	        queue<std::shared_ptr<Detector>> gsDetector;
	        //queue<unique_ptr<Detector>> gsDetector;
	        //std::stack<unique_ptr<Detector>> gsDetector;
            /* all stream and capture cam threads share the only `Fusion` (so, there is a need to access by mutex) */
            unique_ptr<Fusion> gsFusion;
            unique_ptr<Fusion> halfFusion;
            std::once_flag onceFlag;
        }

	    std::mutex camLock;
        std::vector<std::mutex> locks;
        std::vector<std::condition_variable> capBusy;
        std::vector<std::shared_ptr<cv::VideoCapture>> caps; 
        std::vector<std::future<std::shared_ptr<cv::VideoCapture>>> futureCaptures; 

        std::vector<cv::Mat> openImages;
        std::vector<cv::Mat> closeImages; 

        std::vector<int> openFusionResult;
        std::vector<int> closeFusionResult;
        bool reuseImages = true;

        std::unordered_map<std::string, cv::Mat> closeImagesMap;
        std::vector<std::future<bool>> futureWriteImages;

        ShelfInfo shelfInfo;
    }

    /* callback func for darknet v2, use `yolo::native_callback` to invoke it */
    inline namespace callback_v2 {
        auto native_callback = [&](std::vector<std::vector<float>> results, float thres) {
            for (auto&& probs: results) {
                int i = 0;
                char code_ret[64] = { '\0' };
                for (auto&& prob: probs) {
#ifndef DEBUG_NO_JNI_CALLBACK
                    if (std::abs(prob) > thres) {
                        int dir = [&prob](){ return prob > 0? 1: -1; }();
                        cout << "**** DEBUG_INFO (hit) from `native_callback(..)` ****, detected good ( name: "
                            << yolo::data::obj_names[i] << ", direction: " << dir << " )\n";
                        auto code = yolo::data::find_good_code(i);
                        assert(code.size() != 0 && "goods map may be corrupted...\n");

                        std::copy(code.begin(), code.end(), code_ret);
                        callback(code_ret, 1, dir);
                    }
                    else {
                        cout << "**** DEBUG_INFO (discard) from `native_callback(..)` ****, detected good ( name: "
                            << yolo::data::obj_names[i] << " ), without direction info\n";
                    }
#endif
                    ++i;
                }
            }
#ifndef DEBUG_NO_JNI_CALLBACK
            /* end signal to tell icelocker-main-process that it time to settle */
            char end[64] = { "terminate" };
            callback(end, 0, 0);
#endif
        }; 
    }

    namespace utils {
        auto WriteImage = [&]( std::string name, cv::Mat frame )
        {
            // `gDumpFolder` is updated at each shopping.
            auto absolutePath = yolo::data::gDumpFolder + name;
            return cv::imwrite( absolutePath.c_str(), frame );
        };
        // std::shared_ptr<cv::VideoCapture> OpenCapture( CamCtrl& cam )
        auto OpenCapture = [&]( CamCtrl cam )
        {
            auto idx        = cam.getVideoNodeIdx();
            auto trackingID = cam.getTrackId();
            auto w          = cam.get_width();
            auto h          = cam.get_height();

            {
                std::lock_guard<std::mutex> l(yolo::data::camLock);
                auto cap = std::make_shared<cv::VideoCapture>( idx );

                if (!cap->isOpened()) 
                {
                    cerr << "Fail to open camera with tracking ID " << trackingID << ", exit..." << endl;
                    exit(-1);
                }

                cap->set(CV_CAP_PROP_FPS, 10);
                cap->set(CV_CAP_PROP_FOURCC, CV_FOURCC('M','J','P','G'));
                cap->set(CV_CAP_PROP_FRAME_WIDTH, w);
                cap->set(CV_CAP_PROP_FRAME_HEIGHT, h);
                return cap;
            }
            // for ( auto i = 0; i < 5; ++i ) ( void ) cap->grab();
        };

        auto GetImages = [&](std::shared_ptr<cv::VideoCapture> cap, int idx = 0) {
            perf p("take picture");
            // std::unique_lock<std::mutex> lock(yolo::data::locks[idx]);
            // std::cout << "##@@## `VideoCapture "  << idx << "` shared_ptr count is " << cap.use_count() << std::endl;
            // yolo::data::capBusy[idx].wait(lock, [&cap](){ return cap.use_count() < 3; });
            // 
            // for (auto i = 0; i < 3; ++i) { (void)cap->grab(); }
            // while (!cap->read(frame)) {
            //     if (++count > 8) {
            //         std::cout << "##@@## fail to retrieve frame...\n";
            //         exit(1);
            //     }
            // }
            // lock.unlock();
            // cap.reset();
            // yolo::data::capBusy[idx].notify_one(); 
            
            cv::Mat frame;
            auto count = 0; 
            for (auto i = 0; i < 10; ++i) ( void ) cap->grab(); // inevitable progress, at least 5 times, as the inner buffer of cam
            while (!cap->read(frame)) {
                if (++count > 8) {
                    std::cout << "##@@## fail to retrieve frame...\n";
                    exit(1);
                }
            }

            return frame;
        };

        static bool enable_dump(int& interval){
            interval = 0;
            ifstream fin("/tmp/enable_dump");
            if (!fin) {
                cout << "/tmp/enable_dump not exist. disable save video." << endl;
                return false;
            }
            if (!fin.eof())
                fin>>interval;
            cout << "/tmp/enable_dump exist. enable save video. sampleinterval=" <<interval<< endl;
            return true;
        }

        /* get milliseconds from now to epoch */
        int64_t get_time() {
            auto now = std::chrono::system_clock::now();
            auto now_millis = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
            auto value = now_millis.time_since_epoch();

            return value.count();
        }

        /* almost useless */
        void show_fusion_result(vector<vector<float>> fusion_result) {
            for (auto&& probs: fusion_result) {
                cout << "*** fusion_result as follows:\n";
                //std::copy(probs.begin(), probs.end(), std::ostream_iterator<float>(cout, ", "));
                for (auto p:probs){
                    cout<<p<<",";
                }
                cout << "\n";
            }
        }

        void show_detect_result(vector<Df> &res, std::string &path) {
    	    std::ofstream f(path, std::ios::app);
    	    if (f.is_open()) {
                for (auto &&df: res) {
                    f << df.b.x << " " << df.b.y << " " << df.b.w << " " << df.b.h;
                    for (auto &&p: df.pc)
                        f << " " << p;
                    f << "\n";
                }
            }
	        f.close();
        }

        /* same as `show_fusion_result`, as there many meaningless information */
        void show_detect_result(vector<Df> res) {
            cout << "*** detect sth: \n";

            for (auto&& df: res) {
                cout << "\ttimestamp, " << df.ts << "\n";
                cout << "\tprobability, " << df.p_obj << "\n";
                cout << "\tbox info, " << "( " << df.b.x << ", " << df.b.y << ", " << df.b.w << ", " << df.b.h << ")\n";
            }
        }

        void draw_boxes(cv::Mat mat_img,
                        std::vector<Df > &result_vec,
                        bool save = false,
                        int track_id = 0,
                        std::string direct = "result",
                        unsigned int wait_msec = 0,
                        int current_det_fps = -1,
                        int current_cap_fps = -1) {

            detectResultLock.lock();
            int const colors[6][3] = { { 1,0,1 },{ 0,0,1 },{ 0,1,1 },{ 0,1,0 },{ 1,1,0 },{ 1,0,0 } };
            for (auto &i : result_vec) {
                auto max_p = std::max_element(i.pc.begin(), i.pc.end());
                if (*max_p<= Tracking::strong_detction_thr)
                    continue;

                int obj_id = std::distance(i.pc.begin(), max_p);
                int const offset = obj_id * 123457 % 6;
                int const color_scale = 150 + (obj_id * 123457) % 100;
                cv::Scalar color(colors[offset][0], colors[offset][1], colors[offset][2]);
                color *= color_scale;
                int x = (int)(mat_img.cols * std::fmax(0.0, (i.b.x - i.b.w / 2.)));
                int y = (int)(mat_img.rows * std::fmax(0.0, (i.b.y - i.b.h / 2.)));
                int w = (int)(mat_img.cols * i.b.w);
                int h = (int)(mat_img.rows * i.b.h);

                cv::rectangle(mat_img, cv::Rect(x, y, w, h), color, 5);
                if (yolo::data::obj_names.size() > obj_id) {
                    char c_name[64];
                    std::string obj_name = yolo::data::obj_names[obj_id];
                    sprintf(c_name, "%s  %.2f", obj_name.c_str(), *max_p);
                    cv::Size const text_size = getTextSize(c_name, cv::FONT_HERSHEY_COMPLEX_SMALL, 1.2, 2, 0);
                    int const max_width = (text_size.width > w + 2) ? text_size.width : (w + 2);
                    cv::rectangle(mat_img, cv::Point2f(std::max((int)x - 3, 0), std::max((int)y - 30, 0)),
                                  cv::Point2f(std::min((int)x + max_width, mat_img.cols-1), std::min((int)y, mat_img.rows-1)),
                    	          color, CV_FILLED, 8, 0);
                    putText(mat_img, c_name, cv::Point2f(x, y - 10), cv::FONT_HERSHEY_COMPLEX_SMALL, 1.2, cv::Scalar(0, 0, 0), 2);
                }
            }
            if (save) {
                char c_name[256];
                sprintf(c_name, "%s/result-%s-tkid%d.jpg", yolo::data::gDumpFolder.c_str(), direct.c_str(), track_id);
                cv::imwrite(c_name, mat_img);
#ifdef SAVE_PRE
                if ( direct.compare( "close" ) == 0 )
                {
                    sprintf( c_name, "/result-open-tkid%d.jpg", track_id );
                    std::cout << "##@@## " << c_name << std::endl;  
                    yolo::data::closeImagesMap.insert( std::make_pair<std::string, cv::Mat>( std::string( c_name ), mat_img.clone() ) ); 
                }
#endif
            }
            detectResultLock.unlock();
#ifdef DEBUG_NO_JNI_CALLBACK
            if (!save) {
                std::ostringstream oss;
                oss<<std::this_thread::get_id();
                std::string winName = oss.str();
                //cout<<winName<<endl;

                std::unique_lock<std::mutex> lock(mtx_imshow);
                cv::namedWindow(winName.c_str());
                cv::imshow(winName.c_str(), mat_img);
                cv::waitKey(1);
            }
#endif
        }

        /*namespace debug_tracking_v1 {
            void debug_info_tracking(std::vector<Tracklet_all> &tracking_result, std::ofstream &FP, std::vector<int64_t > &ts_vec, int frame_cnt) {

	        for (int tracklet_id = 0; tracklet_id < tracking_result.size(); tracklet_id++) {
	    	    FP << "Frame " << frame_cnt << " Tracklet_all " << tracklet_id;
	    	    auto it = find(ts_vec.begin(), ts_vec.end(), tracking_result[tracklet_id].ts_start);
	    	    int frame_start = it - ts_vec.begin();
	    	    it = find(ts_vec.begin(), ts_vec.end(), tracking_result[tracklet_id].ts_end);
	    	    int frame_end = it - ts_vec.begin();
	    	    FP << " Start " << frame_start << " End " << frame_end << endl;
	    	    for (int c = 0; c < yolo::data::obj_names.size(); c++) {
	    	        float pc = tracking_result[tracklet_id].prob[c];
	    	        if (fabs(pc) > 0.0001) {
	                    FP << yolo::data::obj_names[c] << " :" << pc << endl;
	    	        }
	    	    }

	    	    FP << endl;
	        }
            }
        }*/

        inline namespace debug_tracking_v2 {
            void debug_info_tracking(std::vector<Tracklet_all > &tracking_result, std::ofstream &FP, std::vector<int64_t > &ts_vec, int frame_cnt) {
                for (int tracklet_id = 0; tracklet_id < tracking_result.size(); tracklet_id++) {
                    Tracklet_ele & t_start = tracking_result[tracklet_id].ele.front();
                    Tracklet_ele & t_end = tracking_result[tracklet_id].ele.back();
                    int class_id = tracking_result[tracklet_id].class_id;
                    FP << "Frame " << frame_cnt << " Tracklet_id " << tracking_result[tracklet_id].id;
                    auto it = find(ts_vec.begin(), ts_vec.end(), t_start.ts);
                    int frame_start = it - ts_vec.begin();
                    it = find(ts_vec.begin(), ts_vec.end(), t_end.ts);
                    int frame_end = it - ts_vec.begin();
                    FP << " Start " << frame_start << " End " << frame_end << endl;
                    FP << t_start.ts << ", " << t_end.ts << endl;
                    //double depth_start = Distance3D(t_start.bbox, class_id, t_start.cam_id);
                    //double depth_end = Distance3D(t_end.bbox, class_id, t_end.cam_id);
                    FP << "depth_start " << t_start.depth << " depth_end " << t_end.depth << " prob: " << endl;
                    FP << yolo::data::obj_names[class_id] << " :" << tracking_result[tracklet_id].pc << endl;
                    FP << endl;
                }
            }
        }


        void show_tracking_result(vector<Tracklet_all>& tracking_res, int tracking_id) {
            for (auto&& tracklet: tracking_res) {
                if (fabs(tracklet.pc) > yolo::data::thres)
                    cout << "\n*** cam_" << tracking_id << " detecting " << "\033[1;33m" << yolo::data::find_ch_name(tracklet.class_id) << "\033[0m" << ", with probability " << tracklet.pc << "\n";
            }
        }

        /* get current date with format `%Y_%m_%d_%H_%M_%S` */
        std::string get_date() {
            char tp[64] = {'\0'};
            auto now = std::chrono::system_clock::now();
            std::time_t t = std::chrono::system_clock::to_time_t(now);
            std::tm tm = *std::localtime(&t);
            strftime(tp, 64, "%Y_%m_%d_%H_%M_%S", &tm);
            return std::string(tp);
        }
    }
}

#ifdef VISIONLIB
class VisionLib{
private:
    CamCtrl     mCamCtrl;
    int         mCamIdx;
    std::string mCfg;
    std::string mWeights;
    std::string mVocname;
    bool        bSaveOutput;//drop, not used
    std::string mOutfile;//drop, not used
    double      mThres;

    bool        bExit;
    std::mutex  mtx;
    std::condition_variable cv;
    std::condition_variable new_consumed_cond;

    std::thread tThread;
    Detector* pDetector;
    Tracking* ptracking;

    static std::mutex camLock;
    static std::mutex fusion_lock;

    /* assign a constant number to the specified camera, that is
     * {
     *     "left top": 0,
     *     "right top": 1,
     *     "right bottom": 2,
     *     "left bottom": 3
     * } */
    int tracking_id;
    string port_id;

    // save debug info to local file
    std::ofstream fusion_dbg_info;

    std::ofstream ts_file;

    std::vector<int64_t> ts_vec;

public:
    VisionLib(CamCtrl camctrl,
              std::string cfg="data_xm/yolo-voc.cfg",
              std::string weights="data_xm/yolo-voc.weights",
              std::string vocname="data_xm/voc.names",
              double thres=0.24,
              bool save_output=false,
              std::string outfile=""): mCamCtrl(camctrl),
                                       mCfg(cfg),
                                       mWeights(weights),
                                       mVocname(vocname),
                                       mThres(thres),
                                       bExit(false),
                                       pDetector(nullptr),
                                       ptracking(nullptr) {
        cout<<__FUNCTION__<<endl;
        mCamIdx = mCamCtrl.getVideoNodeIdx();
        //mOutfile = std::to_string(mCamIdx) + outfile;

        cout<<"    camidx="    <<mCamIdx<<endl;
        cout<<"    cfg="       <<mCfg<<endl;
        cout<<"    weights="   <<mWeights<<endl;
        cout<<"    vocname="   <<mVocname<<endl;
        cout<<"    thres="     <<mThres<<endl;
        //cout<<"    saveoutput="<<bSaveOutput<<endl;
        //cout<<"    outputfile="<<mOutfile<<endl;
    }

    virtual ~VisionLib() {
        cout<<__FUNCTION__<<" E. cam "<<mCamIdx<<endl;
        // delete pDetector;
        delete ptracking;
    }

    int visionPrepare(){
        cout<<__FUNCTION__<<" E. cam "<<mCamIdx<<endl;

        perf p(std::string("Detector ctor ") + std::to_string(mCamIdx));
        if (pDetector != nullptr){
            cerr<< "ERROR: multi visionPrepare called! construct Detector" <<endl;
            delete pDetector;
        }
        pDetector = new Detector(mCfg, mWeights);
#if 0
        /* initialize Tracking */
        //port_id = mCamCtrl.getPciId();
        port_id = yolo::data::get_port_id(mCamIdx);
        if (port_id.compare("-1") == 0 || port_id.compare("") == 0) {
            cerr << "<camera id> matches <port id> failures...\n";
            exit(1);
        }
        tracking_id = yolo::data::tracking_map[port_id];
        cout <<"map port_id -> tracking_id : " << port_id << " -> " << tracking_id << endl;
#endif
        /* initialize Tracking */
        port_id = mCamCtrl.getPciId();
        if (port_id.compare("-1") == 0 || port_id.compare("") == 0) {
            cerr << "<camera id> matches <port id> failures...\n";
            exit(1);
        }
        tracking_id = stoi(mCamCtrl.getTrackId());
        cout <<"map port_id -> tracking_id : " << port_id << " -> " << tracking_id << endl;


        if (ptracking != nullptr){
            cerr<< "ERROR: multi visionPrepare called! construct Tracking" <<endl;
            delete ptracking;
        }
        ptracking = new Tracking(yolo::data::obj_names.size(), tracking_id);
        /*
        system("mkdir -p ./output/videos");
        system("mkdir -p ./output/logs");
        */

        return 0;
    }

    int visionStart(){
#if 1
        std::string suffix = std::string("pci") + port_id +
                             std::string("-") +
                             std::string("tkid") + std::to_string(tracking_id) +
                             std::string("-") +
                             yolo::data::gTimeStamp;
        mOutfile = yolo::data::gDumpFolder +
                   std::string("/video-") +
                   suffix +
                   std::string(".avi");

        std::string ts_path = yolo::data::gDumpFolder +
                              std::string("/ts-") +
                              suffix +
                              std::string(".txt");
#else
        std::string suffix = std::to_string(tracking_id) +
                             std::string("_bak");
        mOutfile = std::string("./output/videos/test/cam_") +
                   suffix +
                   std::string(".avi");

        std::string ts_path = std::string("./output/videos/test/ts_") +
                              suffix +
                              std::string(".txt");
#endif

        if (yolo::data::gEnableDump)
            ts_file.open(ts_path.c_str());

        /* initialize fusion_dbg_info (ofstream) */
        std::string debug_per_track_path = yolo::data::gDumpFolder +
                                 std::string("/fusion_dbg-") +
                                 suffix +
                                 std::string(".txt");
        fusion_dbg_info.open(debug_per_track_path.c_str());

        cout<<__FUNCTION__<<" E. cam "<<mCamIdx<<endl;
        tThread = std::thread([&]() { _processThread();});
        return 0;
    }

    int _processThread(){
        Detector& detector = *pDetector;

        std::string out_videofile = mOutfile;
        bool const save_output_videofile = yolo::data::gEnableDump;

        bExit = false;
        int frame_count = 0;  // counts how many frames readed from file
        int sample_count = 0;    // indicates sampling frequency, save one frame while three frames passing

        auto crop_info = std::move(mCamCtrl.get_crop());
        auto rotate_angle = mCamCtrl.get_rotate();
        auto width = mCamCtrl.get_width();
        auto height = mCamCtrl.get_height();

        /*
        std::string sam_dir = std::string("./output/sample_images/") + yolo::data::sam_dir + std::string("/") + std::string("pci") + std::to_string(port_id);
        std::string cmd = std::string("mkdir -p ") + sam_dir;
        system(cmd.c_str());
        */

        while (!bExit) {
            try {
                cout<<"external loop, /dev/video"<< mCamIdx <<endl;
#ifdef OPENCV
                int64_t ts;
                int64_t cur_ts;

                cv::Mat cap_frame, cur_frame, det_frame, write_frame;

                std::shared_ptr<image_t> det_image;

                std::vector<Df> result_vec, thread_result_vec;

                std::atomic<bool> consumed, videowrite_ready;
                consumed =         true;
                videowrite_ready = true;

                std::atomic<int> fps_det_counter, fps_cap_counter;
                fps_det_counter = 0;
                fps_cap_counter = 0;

                int current_det_fps = 0, current_cap_fps = 0;

                std::thread t_detect, t_cap, t_videowrite;

#ifdef DEBUG_WITH_FAKE_FRAME
                std::string ts_file_name = std::string("./debug_video/cam") + std::to_string(tracking_id) + std::string(".txt");
                std::ifstream ts_file_ifs(ts_file_name.c_str());
#endif

                std::chrono::steady_clock::time_point steady_start, steady_end;
                cv::VideoCapture cap;
                {
                    std::unique_lock<std::mutex> lock(camLock);
                    {
#ifndef DEBUG_WITH_FAKE_FRAME
                        //TODO: here we open/close camera once, as some unknown error in opencv when set CV_PROP if camera is not closed correctly last time.
                        cv::VideoCapture tmp(mCamIdx);
#endif

                    }
#ifdef DEBUG_WITH_FAKE_FRAME
                    std::string video_file = std::string("./debug_video/cam") + std::to_string(tracking_id) + std::string(".avi");
                    cap.open(video_file.c_str());
#else
                    cap.open(mCamIdx);
#endif
                    static int failcnt=0;
                    if (!cap.isOpened()){
                        if (failcnt++>5){
                            cerr<<"Fail to open camera "<<mCamIdx<<". Exit"<<endl;
                            return -1;
                        }
                        cerr<<"Fail to open camera "<<mCamIdx<<endl;
                        continue; //try again
                    }
                    failcnt = 0;

                    cap.set(CV_CAP_PROP_FOURCC, CV_FOURCC('M','J','P','G'));
                    cap.set(CV_CAP_PROP_FRAME_WIDTH, width);
                    cap.set(CV_CAP_PROP_FRAME_HEIGHT, height);
                }

                int const video_fps = cap.get(CV_CAP_PROP_FPS);

                cap >> cap_frame;
#ifdef DEBUG_WITH_FAKE_FRAME
                ts_file_ifs >> ts;
                ++frame_count;
#else
                ts = yolo::utils::get_time();
#endif
                ts_vec.push_back(ts);

                if (cap_frame.empty()) {
                    cerr<<"Empty frame. camera "<<mCamIdx<<endl;
                }

                /*
                //for debug only, hardcoding crop/resize/rotate
                cv::Mat crop_frame = cap_frame(cv::Range(0,480), cv::Range(80,80+480)); //as resize below, we could skip clone cropped frame
                cv::resize(crop_frame, cur_frame, cv::Size(416,416));
                */
                //cur_frame = cap_frame(cv::Range(0,480), cv::Range(80,80+480)).clone();
#if 0
                //skip crop/rotate, for latest training file
                //FIXME: the crop/rotate perf could be optimized by removing some redundant clone.
                cur_frame = crop_frame_func(cap_frame, crop_info);
                cur_frame = rotate_frame_func(cur_frame, rotate_angle);
#else
                cur_frame = cap_frame.clone();
                cur_ts = ts;
#endif
                cv::Size const frame_size = cur_frame.size();
                cv::VideoWriter output_video;

                if (save_output_videofile)
                    output_video.open(out_videofile, CV_FOURCC('X', '2', '6', '4'), std::max(30, video_fps), frame_size, true);
                    //output_video.open(out_videofile, CV_FOURCC('D', 'I', 'V', 'X'), std::max(30, video_fps), frame_size, true);

                while (!cur_frame.empty() && !bExit) {
                    if (t_cap.joinable()) { //##@@## check whether a new frame is ready, if yes, clone the new frame cap_frame to cur_frame
                        t_cap.join();
                        ++fps_cap_counter;

#if 0
                        cur_frame = crop_frame_func(cap_frame, crop_info);
                        cur_frame = rotate_frame_func(cur_frame, rotate_angle);
#else
                        cur_frame = cap_frame.clone();
#endif
                        cur_ts = ts;

                        if (DEBUG_INFO){
                            cout<<"size="<<cur_frame.cols<<"x"<<cur_frame.rows<<endl;
                            cout<<"cam "<<mCamIdx<<" fetch"<<endl;
                        }
                    }

                    /* capture a new frame through a new thread */
                    t_cap = std::thread([&]() {
#ifdef DEBUG_WITH_FAKE_FRAME
                        ts_file_ifs >> ts;
                        ++frame_count;
#else
                        ts = yolo::utils::get_time();
#endif

#ifdef DEBUG_NO_JNI_CALLBACK
                        /* there is a potential risk while using ts_vec (with inner data always growing), such as someone keeps the door opening */
                        ts_vec.push_back(ts);
#endif
                        cap >> cap_frame;

#ifdef DEBUG_WITH_FAKE_FRAME
                        if (ts_file_ifs.eof() || cur_frame.empty()){
                            cerr<<"reach debug file EOF. exit"<<endl;
                            bExit = true; //request out looper exit
                            return;
                        }
#endif
                        if ((sample_count++ >= yolo::data::gSampleInterval) && output_video.isOpened() && videowrite_ready) {
                            if (t_videowrite.joinable())
                                t_videowrite.join();

                            write_frame = cur_frame.clone();
                            videowrite_ready = false;
                            t_videowrite = std::thread([&, ts]() {
                                    output_video << write_frame;
                                    ts_file << ts << "\n";
                                    videowrite_ready = true;
                                    });
                            sample_count = 0;
                        }


                        /*
                        if (++sample_count % 3 == 0) {
                            auto static_frame = cap_frame.clone();
                            auto future = std::async(std::launch::async, [&]() {
                                auto sam_name = sam_dir +
                                std::string("/sample_pci") +
                                std::to_string(port_id) +
                                std::string("_") +
                                std::to_string(yolo::utils::get_time()) +
                                std::string(".jpg");
                                cv::imwrite(sam_name.c_str(), static_frame);
                            });
                            sample_count = 0;
                        }
                        */
                    });

                    // wait detection result for video-file only (not for net-cam)
                    //if (protocol != "rtsp://" && protocol != "http://" && protocol != "https:/")  //##@@## wait detection if videofile/usbcamera is used.
                    {
                        {
                            std::unique_lock<std::mutex> lock(mtx);
                            cv.wait(lock, [&](){ return bExit || consumed; });
                            if (bExit) continue;
                        }

                        /* `consumed` means the previous frame is ready to use */
                        if (consumed){
                            //do s/w pre-process
                            det_image = detector.mat_to_image_resize(cur_frame);

                            result_vec = thread_result_vec;
                            consumed = false;
                            new_consumed_cond.notify_all();
                        }
                    }


                    /* launch t_detect once, then run until `bExit == true` */
                    if (!t_detect.joinable()) {
                        t_detect = std::thread([&]() {
                                auto image_in = det_image;
                                auto ts_in = cur_ts;


                                while (image_in.use_count() > 0 && !bExit) {
                                    auto result = detector.detect_resized(*image_in, frame_size, mThres, false);

                                    /* solving detection results */
                                    auto tracking_result = ptracking->add_detctions(result, ts_in);

                                    yolo::utils::debug_info_tracking(tracking_result, fusion_dbg_info, ts_vec, frame_count);
                                    yolo::utils::show_tracking_result(tracking_result, tracking_id);
                                    fusion_lock.lock();
                                    yolo::data::pfusion->add_video_info(tracking_id, tracking_result);
                                    fusion_lock.unlock();

                                    ++fps_det_counter;
                                    thread_result_vec = result; // output

                                    std::unique_lock<std::mutex> lock(mtx);
                                    consumed = true;
                                    cv.notify_all();

                                    new_consumed_cond.wait(lock, [&](){ return bExit || !consumed; });

                                    image_in = det_image;  // input
                                    ts_in = cur_ts;
                                }
                                });
                    }

#ifdef DEBUG_NO_JNI_CALLBACK
                    if (yolo::data::gIsGuiPresent)
                        yolo::utils::draw_boxes(cur_frame.clone(), result_vec);
#endif

                    if (!cur_frame.empty()) {
                        steady_end = std::chrono::steady_clock::now();
                        if (std::chrono::duration<double>(steady_end - steady_start).count() >= 1) {
                            current_det_fps = fps_det_counter;
                            current_cap_fps = fps_cap_counter;
                            if (DEBUG_INFO)
                                cout << "*** FPS thread-"
                                    << std::hash<std::thread::id>{}(std::this_thread::get_id())
                                    //<< std::this_thread::get_id()
                                    << ": (det_fps " << current_det_fps << ", "
                                    << "cap_fps " << current_cap_fps << ")\n";
                            steady_start = steady_end;
                            fps_det_counter = 0;
                            fps_cap_counter = 0;
                        }

                    }

#if 0 //def DEBUG_WITH_FAKE_FRAME
                    if (frame_count > 999) {
                        std::unique_lock<std::mutex> lock(mtx);
                        cv.wait(lock, [&](){ return bExit; });
                    }
#endif
                }
                if (t_cap.joinable()) t_cap.join();
                if (t_detect.joinable()) t_detect.join();
                if (t_videowrite.joinable()) t_videowrite.join();
                cout << "Video ended \n";
#else
                auto img = detector.load_image(filename);
                std::vector<bbox_t> result_vec = detector.detect(img);
                detector.free_image(img);
#endif
            }
            catch (std::exception &e) { cerr << "exception: " << e.what() << "\n"; getchar(); }
            catch (...) { cerr << "unknown exception \n"; getchar(); }
        }
        auto tracking_res = ptracking->add_detctions();
        yolo::utils::debug_info_tracking(tracking_res, fusion_dbg_info, ts_vec, frame_count);
        fusion_lock.lock();
        yolo::data::pfusion->add_video_info(tracking_id, tracking_res);
        fusion_lock.unlock();

        return 0;
    }//_processThread

        int visionStop(){
            if (ts_file.is_open())
                ts_file.close();

            if (fusion_dbg_info.is_open())
                fusion_dbg_info.close();

            cout<<__FUNCTION__<<" E. cam "<<mCamIdx<<endl;
            {
                std::unique_lock<std::mutex> lock(mtx);
                bExit=true;
                cv.notify_all();
                new_consumed_cond.notify_all();
            }
            if (tThread.joinable()) {
                cout<<__FUNCTION__<<" cam " << mCamIdx<<" start to join process thread..."<<endl;
                tThread.join();
                cout<<__FUNCTION__<<" cam " << mCamIdx<<" join process thread done"<<endl;
            }
	    cout<<__FUNCTION__<<" X. cam "<<mCamIdx<<endl;
            return 0;
        }


/* snippets of capture camera */
private:
    static std::vector<std::thread> open_thread_table;
    static std::vector<std::thread> close_thread_table;
    static std::mutex mtx_capture;
    static std::condition_variable cond;
    static bool closed;


    static void take_picture(const std::string &open_close, const CamCtrl &cam, cv::Mat &frame_open) {
        perf p("Take picture");
        auto output_dir = yolo::data::gDumpFolder;
#ifndef DEBUG_WITH_FAKE_FRAME
            int idx = cam.getVideoNodeIdx();
            std::string tracking_id = cam.getTrackId();
            auto width = cam.get_width();
            auto height = cam.get_height();

            cv::VideoCapture cap;
            {
                std::unique_lock<std::mutex> lock(camLock);
                cap.open(idx);
            }
            if (!cap.isOpened()) {
                cerr<<"Fail to open camera "<<idx<<", exit..."<<endl;
                exit(-1);
            }

            cout<<__FUNCTION__<<": open camera " << idx <<  ", width = " << width << ", height = " << height << ", success"<<endl;
            cap.set(CV_CAP_PROP_FOURCC, CV_FOURCC('M','J','P','G'));
            cap.set(CV_CAP_PROP_FRAME_WIDTH, width);
            cap.set(CV_CAP_PROP_FRAME_HEIGHT, height);

            std::string suffix = std::string("tkid") + tracking_id;
            // cv::Mat frame_open;

            /* prefetch some frame to avoid corrupted frame (read(..) is a blocked
            method (at least 30 milliseconds for each frame), `8` is bigger) */
            for (int i = 0; i < 3; ++i) {
		//auto future = std::async(std::launch::async, [&](){ cap.read(frame_open); });
                if(!cap.read(frame_open))
                    cout << "!!!!!!!!!!!  skip frame failed !!!!!!!! cam info:  " << suffix << endl;
            }
            //std::this_thread::sleep_for(std::chrono::seconds(1));

            int retry = 0;
            while(!cap.read(frame_open)) {
                retry++;
		        if(retry > 8)
                    cout << "!!!!!!!!!!!  skip " << open_close << " frame failed !!!!!!!! cam info:  " << suffix << endl;
            }

            string name_open = output_dir +
                               "/" +
				               open_close +
				               "-" +
				               suffix +
				               std::string(".jpg");
            cv::imwrite(name_open.c_str(), frame_open);
            cout << name_open << " generated." << endl;
            // frame = frame_open; //.clone();
            cap.release();
        p.done();
#else
        cout<<"copy fake image to output folder " << endl;
        string cmd = std::string("cp -a precap/* ") + yolo::data::gDumpFolder;
        system(cmd.c_str());
#endif
    }

    static void do_detect(const int open_close, const double &mThres, cv::Mat &cur_frame, const std::string &tracking_id) {
        using namespace yolo::data::static_cam;

        perf p("Do detect");
// #ifdef DEBUG_WITH_FAKE_FRAME
//         string oc = open_close ? "close" : "open";
//         string fname = yolo::data::gDumpFolder + "/" + oc + "-tkid" + tracking_id +".jpg";
// 
//         cout<<"loading fake image: "<< fname << endl;
//         cur_frame = imread(fname);
//         if (cur_frame.empty()) {
//             cout << fname << " is not found" <<endl;
//             exit(-1);
//         }
// #endif
        int cropx0 =276;
        int cropx1 =276;
        int cropy0 =57;
        int cropy1 =57;
        cv::Size const r_size = cur_frame.size();
        cur_frame = cur_frame(cv::Range(cropy0,r_size.height-cropy1),cv::Range(cropx0,r_size.width-cropx1));
        cv::Size const frame_size = cur_frame.size();
        cout<<__FUNCTION__<<": detector image resize"<< ", width = " << frame_size.width << ", heigh = " << frame_size.height << endl;
        detector_sem.wait();

        detectLock.lock();
        auto detector = yolo::data::static_cam::gsDetector.front();
        yolo::data::static_cam::gsDetector.pop();
        detectLock.unlock();

        std::shared_ptr<image_t> det_image = detector->mat_to_image_resize(cur_frame);
        cout<<__FUNCTION__<<": detector detect ..."<<endl;
        auto result = detector->detect_resized(*det_image, frame_size, mThres, false);

        {
            auto path = yolo::data::gDumpFolder + "/" + (open_close? "close": "open") + tracking_id + ".log";
            yolo::utils::show_detect_result(result, path);
        }


        detectLock.lock();
        yolo::data::static_cam::gsDetector.push(detector);
        detectLock.unlock();

        detector_sem.notify();

        cout<<__FUNCTION__<<": add to fusion: "<< atoi(tracking_id.c_str()) << endl;

        fusion_lock.lock();
        yolo::data::static_cam::gsFusion->add_detctions(open_close, atoi(tracking_id.c_str()), result);

        /* cache close-door detection results if `yolo::data::reuseImages` sets to true */
        if ( yolo::data::reuseImages && open_close )
        {
            // std::call_once(onceFlag, [&halfFusion] { halfFusion.reset( new Fusion( yolo::data::obj_names,3 ) ); });
            if ( !halfFusion ) halfFusion.reset( new Fusion( yolo::data::obj_names, yolo::data::shelfInfo.layerCount, yolo::data::shelfInfo.layerCamCount) );

            halfFusion->add_detctions( 1 - open_close, atoi( tracking_id.c_str() ), result );
        }
        
        fusion_lock.unlock();

        yolo::utils::draw_boxes(cur_frame, result, true, atoi(tracking_id.c_str()), open_close ? "close" : "open");
        cout<<__FUNCTION__<<": detector done"<<endl;
    }

    static void do_detect(int open_close, double mThres, cv::Mat cur_frame, std::string& tracking_id, std::vector<int>& cropVec) {
        using namespace yolo::data::static_cam;
        perf p("Do detect");

        assert(cropVec.size() == 4 && "corrupted corp info");
        int cropx0 = cropVec[0];
        int cropx1 = cropVec[1];
        int cropy0 = cropVec[2];
        int cropy1 = cropVec[3];

        cv::Size const r_size = cur_frame.size();
        cur_frame = cur_frame(cv::Range(cropy0,r_size.height-cropy1),cv::Range(cropx0,r_size.width-cropx1));
        cv::Size const frame_size = cur_frame.size();
        cout<<__FUNCTION__<<": detector image resize"<< ", width = " << frame_size.width << ", heigh = " << frame_size.height << endl;
        detector_sem.wait();

        detectLock.lock();
        auto detector = yolo::data::static_cam::gsDetector.front();
        yolo::data::static_cam::gsDetector.pop();
        detectLock.unlock();

        std::shared_ptr<image_t> det_image = detector->mat_to_image_resize(cur_frame);
        cout<<__FUNCTION__<<": detector detect ..."<<endl;
        auto result = detector->detect_resized(*det_image, frame_size, mThres, false);

        {
            auto path = yolo::data::gDumpFolder + "/" + (open_close? "close": "open") + tracking_id + ".log";
            yolo::utils::show_detect_result(result, path);
        }

        detectLock.lock();
        yolo::data::static_cam::gsDetector.push(detector);
        detectLock.unlock();

        detector_sem.notify();

        cout<<__FUNCTION__<<": add to fusion: "<< atoi(tracking_id.c_str()) << endl;

        fusion_lock.lock();
        yolo::data::static_cam::gsFusion->add_detctions(open_close, atoi(tracking_id.c_str()), result);

        /* cache close-door detection results if `yolo::data::reuseImages` sets to true */
        if ( yolo::data::reuseImages && open_close )
        {
            if ( !halfFusion ) halfFusion.reset( new Fusion( yolo::data::obj_names, yolo::data::shelfInfo.layerCount, yolo::data::shelfInfo.layerCamCount) ); 
            halfFusion->add_detctions( 1 - open_close, atoi( tracking_id.c_str() ), result );
        }
        fusion_lock.unlock();

        yolo::utils::draw_boxes(cur_frame, result, true, atoi(tracking_id.c_str()), open_close ? "close" : "open");
        cout<<__FUNCTION__<<": detector done"<<endl;
    }

public:
    static void ResetOpenThreads() 
    {
        open_thread_table.clear();
    }

    static void JoinOpenThreads() 
    {
        if ( open_thread_table.empty() ) return;
        for ( auto &&t: open_thread_table ) 
        { 
            if (t.joinable()) t.join(); 
        }
    }

    static int Pre_Capture(std::vector<CamCtrl> &capture_table, std::vector<cv::Mat> &imgs, float mThres=0.24) {
        perf p("Pre_Capture");
        cout<<__FUNCTION__<<" E"<<endl;
        open_thread_table.clear();

        /* open images order is chaos, as we can not make sure the order of threads(bad example) */
        // size_t i = 0;
        // for (const auto& cam: capture_table) {
        //     open_thread_table.emplace_back(std::thread([&]() {
        //         perf p("Open door -> take picture -> detect");
        //         std::string tracking_id = cam.getTrackId();

        //         cv::Mat frame = yolo::data::futureOpenImages[i++].get();

        //         /* save opening frame */
		//         char *name = new char[128];
        //         sprintf(name, "%s/open-tkid%s.jpg", yolo::data::gDumpFolder.c_str(), tracking_id.c_str());
		//         cv::imwrite(name, frame);
		//         delete name;

		//         do_detect(0, mThres, frame, tracking_id);
        //     }));
        // }

        auto i = 0;
        for (const auto& cam: capture_table) {
            open_thread_table.emplace_back(std::thread([&, i]() {
                perf p("Open door -> take picture -> detect");
                std::string tracking_id = cam.getTrackId();

                // direct assignment would affect the origin images vector
                cv::Mat frame = imgs[i];

                /* save opening frame */
		        auto tmpFuture = std::async(std::launch::async, [&, frame]() {
                    char *name = new char[128];
                    sprintf(name, "%s/open-tkid%s.jpg", yolo::data::gDumpFolder.c_str(), tracking_id.c_str());
                    cv::imwrite(name, frame);
                    delete name;
                });
                // do_detect(0, mThres, frame, tracking_id);
                auto cropVec = cam.get_crop();
                do_detect(0, mThres, frame, tracking_id, cropVec);
                (void) tmpFuture.get();
            }));
            ++i;
        }

        for (auto& t: open_thread_table)
            if (t.joinable())
                t.join();


        open_thread_table.clear();
        cout << __FUNCTION__ << " X" << endl;

        return 0;
    }

    static int OpenDetect(std::vector<CamCtrl> &capture_table, std::vector<cv::Mat> &imgs, const size_t i, float mThres=0.24) {
        perf p("Pre_Capture");
        cout<<__FUNCTION__<<" E"<<endl;

        /* open images order is chaos, as we can not make sure the order of threads(bad example) */
        // size_t i = 0;
        // for (const auto& cam: capture_table) {
        //     open_thread_table.emplace_back(std::thread([&]() {
        //         perf p("Open door -> take picture -> detect");
        //         std::string tracking_id = cam.getTrackId();

        //         cv::Mat frame = yolo::data::futureOpenImages[i++].get();

        //         /* save opening frame */
		//         char *name = new char[128];
        //         sprintf(name, "%s/open-tkid%s.jpg", yolo::data::gDumpFolder.c_str(), tracking_id.c_str());
		//         cv::imwrite(name, frame);
		//         delete name;

		//         do_detect(0, mThres, frame, tracking_id);
        //     }));
        // }

        open_thread_table.emplace_back(std::thread([&, i]() {
            perf p("Open door -> take picture (seperate with detection)");
            std::string tracking_id = capture_table[i].getTrackId();

            // direct assignment would affect the origin images vector
            cv::Mat frame = imgs[i].clone();

            /* save opening frame */
		    auto tmpFuture = std::async(std::launch::async, [&, frame]() {
                char *name = new char[128];
                sprintf(name, "%s/open-tkid%s.jpg", yolo::data::gDumpFolder.c_str(), tracking_id.c_str());
                cv::imwrite(name, frame);
                delete name;
            });

            // do_detect(0, mThres, frame, tracking_id);
            auto cropVec = capture_table[i].get_crop();
            do_detect(0, mThres, frame, tracking_id, cropVec);
            (void) tmpFuture.get();
        }));
        cout << __FUNCTION__ << " X" << endl;

        return 0;
    }


    static int Post_Capture(std::vector<CamCtrl> &capture_table, std::vector<cv::Mat> &imgs, float mThres=0.24) {
        perf p("Post_Capture");
        cout<<__FUNCTION__<<" E"<<endl;
        close_thread_table.clear();

        // size_t i = 0; 
        // for (const auto& cam: capture_table) {
        //     close_thread_table.emplace_back(std::thread([&]() {
        //         perf p("Close door -> take picture -> detect");
        //         std::string tracking_id = cam.getTrackId();

        //         // if we capture `i` by vlaue, `i` would be a read-only variable
        //         // static_assert(!std::is_const<decltype(i)>::value, "type error");
        //         cv::Mat frame = yolo::data::futureCloseImages[i++].get();

        //         /* save closing frame */
		//         char *name = new char[128];
        //         sprintf(name, "%s/close-tkid%s.jpg", yolo::data::gDumpFolder.c_str(), tracking_id.c_str());
		//         cv::imwrite(name, frame);
		//         delete name;

        //         do_detect(1, mThres, frame, tracking_id);
        //     }));
        // }
        auto i = 0; 
        for (const auto& cam: capture_table) {
            close_thread_table.emplace_back(std::thread([&, i]() {
                perf p("Close door -> take picture -> detect");
                std::string tracking_id = cam.getTrackId();

                // if we capture `i` by vlaue, `i` would be a read-only variable
                // static_assert(!std::is_const<decltype(i)>::value, "type error");
                cv::Mat frame = imgs[i].clone();

                /* save closing frame */
                auto saveImageFuture = std::async( std::launch::async, [&, frame] {
		            char *name = new char[128];
                    sprintf(name, "%s/close-tkid%s.jpg", yolo::data::gDumpFolder.c_str(), tracking_id.c_str());
		            cv::imwrite(name, frame);
#ifdef SAVE_PRE
                    sprintf( name, "/open-tkid%s.jpg", tracking_id.c_str() );
                    std::cout << "##@@## " << name << std::endl;  
                    yolo::data::closeImagesMap.insert( std::make_pair<std::string, cv::Mat>( std::string( name ), frame.clone() ) );
#endif
		            delete name;
                } );

                // do_detect(1, mThres, frame, tracking_id);
                auto cropVec = cam.get_crop();
                do_detect(1, mThres, frame, tracking_id, cropVec);
                ( void ) saveImageFuture.get();
            }));
            ++i;
        }

        for (auto& t: close_thread_table) {
            if (t.joinable()) {
                t.join();
            }
        }

        close_thread_table.clear();
        cout << __FUNCTION__ << " X" << endl;
        return 0;
    }
};

/* initialize static variable within `VisionLib` */
std::mutex VisionLib::camLock;
std::mutex VisionLib::fusion_lock;
std::vector<std::thread> VisionLib::open_thread_table;
std::vector<std::thread> VisionLib::close_thread_table;
std::mutex VisionLib::mtx_capture;
vector<CameraContainer> gContainers;
//=======================================================================================


class DetectWrapper{
    static std::vector<std::thread> open_thread_table;

    static int open(vector<CameraContainer>& container, float mThres=0.24) {
        perf p("DetectWrapper:E");

        vector<cv::Mat> frames = container.getFramesAll();



        open_thread_table.emplace_back(std::thread([&, i]() {
            perf p("Open door -> take picture (seperate with detection)");
            std::string tracking_id = capture_table[i].getTrackId();

            // direct assignment would affect the origin images vector
            cv::Mat frame = imgs[i].clone();

            /* save opening frame */
		    auto tmpFuture = std::async(std::launch::async, [&, frame]() {
                char *name = new char[128];
                sprintf(name, "%s/open-tkid%s.jpg", yolo::data::gDumpFolder.c_str(), tracking_id.c_str());
                cv::imwrite(name, frame);
                delete name;
            });

            // do_detect(0, mThres, frame, tracking_id);
            auto cropVec = capture_table[i].get_crop();
            do_detect(0, mThres, frame, tracking_id, cropVec);
            (void) tmpFuture.get();
        }));
        cout << __FUNCTION__ << " X" << endl;

        return 0;
    }

}

static bool gIsGuiPresent = false;



extern "C" int VisionPrepare_Wrap()
{
    LOGD("E");
    perf p(__FUNCTION__);
    cout<< "OpenCV version: " << CV_MAJOR_VERSION << "."
        << CV_MINOR_VERSION << "." << CV_SUBMINOR_VERSION << endl;

    bool is_gui_present = true;
    if (NULL == getenv("DISPLAY"))
        is_gui_present = false;
    cout<<"gIsGuiPresent = "<<is_gui_present << (is_gui_present?" Enable liveview":" Disable liveview")<<endl;
    gIsGuiPresent = is_gui_present;

#ifdef DEBUG_NO_JNI_CALLBACK
    if (gIsGuiPresent)
        XInitThreads();
#endif

    perf cameraLoad("load_camera_config");
    vector<CameraConfig> cfgs;
    loadCameraJsonConfig("./camera_cfg.json", cfgs);
    cameraLoad.done();

    perf cameraStart("startAllCamera");
    gContainers = CameraContainer::setupCameraContainer_withPos(cfgs);//capture only, ordered
    gContainers.startAll();
    cameraStart.done();

#if 0

    stream_table.clear();
    capture_table.clear();
    vision_table.clear();

    /* count time consumption on `initAllCameras()` */
    perf cam_perf("initAllCamera");
    bool sendcfg = true;
#ifdef DEBUG_WITH_FAKE_FRAME
    sendcfg = false;
#endif
    auto resVect = std::move(initAllCameras(sendcfg)); // move value automatically
    cam_perf.done();

    if (sendcfg && resVect.size() == 0){
        cerr<<"FATAL: fail to init cameras"<<endl;
        //TODO
        exit(-1);
    }

    /* initialize global varibles */
    yolo::data::init_names(); //init all-goods map used by native wrapper
    yolo::data::init_obj_names(); //init goods list used by algorithm
    yolo::data::init_ports_map_videos(); //init pci-videox table

    for(int i = 0; i < MAX_DETECTOR; i++)
        yolo::data::static_cam::gsDetector.emplace(std::make_shared<Detector>("data_xm/yolo-voc.cfg", "data_xm/yolo-voc.weights"));

    std::vector<std::string> descVec;
    for (auto cam:resVect){
        if (cam.isUsageStream()){
            int i = cam.getVideoNodeIdx();

            //VisionLib* visionPtr = new VisionLib(i);
            std::unique_ptr<VisionLib> visionPtr(new VisionLib(cam));
            if(visionPtr->visionPrepare()){
                cerr<<"FATAL: visionPreapre Failed!"<<endl;
                //TODO: add exception handler;
            }
            else{
                vision_table.push_back(std::move(visionPtr));
                stream_table.push_back(cam);
            }
        }
        else if (cam.isUsageCapture()){
            capture_table.push_back(cam);

            // assemble all cam descriptions
            descVec.emplace_back(cam.getDescription());

#ifndef DEBUG_WITH_FAKE_FRAME
            auto idx        = cam.getVideoNodeIdx();
            auto trackingID = cam.getTrackId();
            auto w          = cam.get_width();
            auto h          = cam.get_height();
    
            auto cap = yolo::utils::OpenCapture( cam );
    
            for (size_t i = 0; i < 5; ++i)
                if (!cap->grab())
                    std::cout << "##@@## grab frame failed with tracking ID " << trackingID << std::endl;
#endif
        }
    }

    /* get shelf information, including shelf layers and total cameras in each layer*/
    std::vector<std::set<int>> layerCamIndexVec;
    const char* delim = "-";

    for (auto&& desc: descVec)
    {
        char* cdesc = strdup(desc.c_str());
        printf("parsing description %s\n", desc.c_str());
        auto i1 = strtok(cdesc, delim);
        auto i2 = strtok(NULL, delim);
        std::cout << i1 << " " << i2 << std::endl;
        assert(i1 && i2 && "bad camera description");

        auto layerIndex = atoi(i1);
        auto camIndex = atoi(i2);

        free(cdesc);

        if (layerCamIndexVec.size() < layerIndex + 1)
            layerCamIndexVec.resize(layerIndex + 1);

        auto res = layerCamIndexVec[layerIndex].insert(camIndex);
        assert(res.second && "duplicate camera description");
    }

    for (auto&& s: layerCamIndexVec)
    {
        auto sum = std::accumulate(s.begin(), s.end(), 0);\
        assert(sum == s.size() * (s.size() - 1) / 2 && "bad camera description");
    }

    yolo::data::shelfInfo.layerCount = layerCamIndexVec.size();
    for (auto&& s: layerCamIndexVec)
    {
        yolo::data::shelfInfo.layerCamCount.push_back(s.size());
    }

    std::cout << "layer count: " << yolo::data::shelfInfo.layerCount << std::endl;
    std::copy(yolo::data::shelfInfo.layerCamCount.begin(), yolo::data::shelfInfo.layerCamCount.end(), std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;

#ifdef DEBUG_WITH_FAKE_FRAME
    auto i = 0;
    for (auto&& s: layerCamIndexVec)
    {
        std::cout << "##@@## layer :" << i++ << std::endl;
        std::copy(s.begin(), s.end(), std::ostream_iterator<int>(std::cout, " "));
        std::cout << std::endl;
    }
#endif

#ifdef SINGLE_CAM
    yolo::data::shelfInfo.layerCount = 4;
    std::vector<int> tmp(4, 1);
    yolo::data::shelfInfo.layerCamCount.swap(tmp);
    std::cout << "4 layer count: " << yolo::data::shelfInfo.layerCount << std::endl;
    std::copy(yolo::data::shelfInfo.layerCamCount.begin(), yolo::data::shelfInfo.layerCamCount.end(), std::ostream_iterator<int>(std::cout, " "));
    std::cout << std::endl;
#endif

#endif

    cout<<__FUNCTION__<<" X"<<endl;
    return 0;
}

extern "C" int VisionStart_Wrap(const char* id){
    cout<<__FUNCTION__<<" E"<<endl;
    perf p(__FUNCTION__);
#if 0
#ifndef DEBUG_WITH_FAKE_FRAME
    std::vector<std::future<cv::Mat>> futureOpenImages;

    
    if ( !yolo::data::static_cam::halfFusion )
    {
        using namespace yolo::data;
        
        for ( auto&& cam: capture_table ){
            yolo::data::futureCaptures.emplace_back( std::async( std::launch::async, yolo::utils::OpenCapture, cam ) );
        }
        for ( auto&& fc: futureCaptures ){
            fc.wait();
        }
        
        for ( auto&& fc: futureCaptures )
        {
            auto cap = fc.get();
            futureOpenImages.emplace_back( std::async( std::launch::async, yolo::utils::GetImages, cap ) );
        } /* capture images synchronously (with std::async) */
    }
#endif

    ss_sem.wait();
#endif
#ifdef DEBUG_WITH_FAKE_FRAME
    yolo::data::gEnableDump = true;
#else
    yolo::data::gEnableDump = yolo::utils::enable_dump(yolo::data::gSampleInterval);
    cout<<"enable dump: " << yolo::data::gEnableDump << "  SampleInterval: " << yolo::data::gSampleInterval<<endl;
#endif
    int layerCount = gContainers.size();
    vector<int> layerCamCount = gContainers[0].getCameraPosIndexPerLayer();
   
    yolo::data::static_cam::gsFusion.reset(new Fusion(yolo::data::obj_names, layerCount, layerCamCount));

    /* two purposes: 
     * 1. resets `halfFusion`
     * 2. inherits close-door detection results */
    if ( yolo::data::static_cam::halfFusion )
        yolo::data::static_cam::halfFusion.swap( yolo::data::static_cam::gsFusion );

#if 0
    int dsize = yolo::data::static_cam::gsDetector.size();
    cout << "Detector size: " << dsize << " (MAX = " << MAX_DETECTOR << ")" << endl;
    cout << "Detector semaphore size: " << detector_sem.get() << endl;
    if(detector_sem.get() != MAX_DETECTOR) {
        detector_sem.reset(MAX_DETECTOR);
    }
#endif
    //yolo::data::static_cam::gsDetector.reset(new Detector("data_xm/yolo-voc.cfg", "data_xm/yolo-voc.weights"));
    yolo::data::gTimeStamp = yolo::utils::get_date();
    yolo::data::gDumpFolder = string("./output/dump_native-") + yolo::data::gTimeStamp;
    //yolo::data::gStaticPicFolder = string("./output/dump_before_after-") + yolo::data::gTimeStamp;

    if (id){
        yolo::data::gDumpFolder += string("-id") + string(id);
    }

    if (yolo::data::gEnableDump){
        string cmd = std::string("mkdir -p ") + yolo::data::gDumpFolder;
        system(cmd.c_str());
        //cmd = std::string("mkdir -p ") + yolo::data::gStaticPicFolder;
        //system(cmd.c_str());
    }

    string fusion_dbg_file = yolo::data::gDumpFolder + std::string("/fusion_dbg_all.txt");
    yolo::data::fusion_dbg_fp.open(fusion_dbg_file.c_str());
    if ( !yolo::data::fusion_dbg_fp.is_open() )
    {
        std::cout << "*** open fusion_dbg_all.txt failure ***";
        exit( 1 );
    }

#ifndef DEBUG_WITH_FAKE_FRAME
    if ( !yolo::data::static_cam::halfFusion )
    {
        int i = 0;
        for (auto& cont: gContainers){
            //vector<cv::Mat> layerFrames = cont.getFramesAll();
            DetectWrapper.open(cont);
        }


        VisionLib::ResetOpenThreads();

        auto index = 0; 
        
        for ( auto&& future: futureOpenImages ) 
        {
            yolo::data::openImages.emplace_back( future.get() );
            VisionLib::OpenDetect( capture_table, yolo::data::openImages, index++ );
        }
    }
#else

    auto name = new char[256];
    cv::Mat frame;
    std::cout << "*** capture_table.size = " << capture_table.size() << " ***\n";
    for ( auto&& cam: capture_table )
    {
        sprintf( name, "./precap/open-tkid%s.jpg", cam.getTrackId().c_str());
        std::cout << "*** read frame from " << name << " ***\n";
        frame = imread( name );
        if ( frame.empty() )
        {
            std::cout << name << " not found\n";
            exit( 1 );
        }
        yolo::data::openImages.emplace_back( frame );
        
        sprintf( name, "./precap/close-tkid%s.jpg", cam.getTrackId().c_str() );
        std::cout << "*** read frame from " << name << " ***\n";
        frame = imread( name );
        if ( frame.empty() )
        {
            std::cout << name << " not found\n";
            exit( 1 );
        }
        yolo::data::closeImages.emplace_back( frame );
    }
    delete name;

    auto index = 0;
    for ( auto&& tmp: yolo::data::openImages )
        VisionLib::OpenDetect( capture_table, yolo::data::openImages, index++ );
    std::cout << "*** you have reached here ( " << __LINE__ <<" ) ***\n";
#endif

    yolo::data::futureCaptures.clear();
#ifdef SAVE_PRE
    std::cout << "##@@## you have defined SAVE_PRE\n";
    if ( !yolo::data::closeImagesMap.empty() )
    {
        using namespace yolo::data;
        // for ( auto&& item: yolo::data::closeImagesMap )
        //     cv::imwrite( item.first.c_str(), item.second );
        for ( auto&& item: closeImagesMap )
        {
            futureWriteImages.emplace_back( std::async( std::launch::async, yolo::utils::WriteImage, item.first, item.second ) );
        }
    }
#endif
    // yolo::data::caps.clear();
    cout<<__FUNCTION__<<" X"<<endl;
    ss_sem.notify();
    return 0;
}

extern "C" int VisionStop_Wrap(){
    cout<<__FUNCTION__<<" E"<<endl;
    perf p(__FUNCTION__);
    using namespace yolo::data;
#ifdef SAVE_PRE
    for ( auto&& future: futureWriteImages )
        ( void ) future.get();
    closeImagesMap.clear();
    futureWriteImages.clear();
#endif

#ifndef DEBUG_WITH_FAKE_FRAME
    std::vector<std::future<cv::Mat>> futureCloseImages; 
    
    for ( auto&& cam: capture_table )
        futureCaptures.emplace_back( std::async( std::launch::async, yolo::utils::OpenCapture, cam ) );

    for ( auto&& fc: futureCaptures ){
        fc.wait();
    }
    
    for ( auto&& fc: futureCaptures )
    {
        auto cap = fc.get();
        futureCloseImages.emplace_back( std::async( std::launch::async, yolo::utils::GetImages, cap ) );
    } 
    
    for (auto &&future: futureCloseImages)
        yolo::data::closeImages.emplace_back(future.get()); 
#endif
    /* two functions below is safe */
    VisionLib::JoinOpenThreads();
    VisionLib::ResetOpenThreads();

    using dvector = std::vector<std::vector<int>>;

    auto printVector = [&](std::vector<int> vec, std::string head) {
        std::cout << head << ": ";
        std::copy(vec.begin(), vec.end(), std::ostream_iterator<int>(std::cout, " "));
        std::cout << "\n";
    };

    auto printSpace = [&](int count) {
        for (auto i = 0; i < count; ++i)
            std::cout << "-";
    };
    auto printTab = [&](int count) {
        for (auto i = 0; i < count; ++i)
            std::cout << "\t";
    };

    auto placeholdLen = [](std::string str) {
        auto length = 0;
        auto utf8Len = 0;
        for (auto&& c: str)
        {
            if (int(c) > 0)
                ++length;
            else
                ++utf8Len;
        }

        return length + utf8Len / 3 * 2;
    };

    auto printName = [&](std::vector<int> vec) {
        std::cout << "good list:\n";
        auto i = 0;
        for (auto&& v: vec)
        {
            if (v != 0)
            {
                auto name = find_ch_name(i);
                name.erase(std::remove(name.begin(), name.end(), '\t'), name.end());
                // std::cout << std::setw(24) <<  name << ": " << v << "\n";
                std::cout << name;
                printSpace(20 - placeholdLen(name));
                // if (name.size() > 6) printTab(4);
                // else                 printTab(3);
                std::cout << ">" << v << "\n";
            }
            ++i;
        }
    };

    auto combineResult = [&](dvector dv) {
        auto i = 0;
        for (auto&& v: dv)
        {
            printVector(v, std::string("layer ") + std::to_string(++i));
            printName(v);
        }
        std::vector<int> res;
        res.swap(dv[0]);
        for (auto i = 1; i < dv.size(); ++i)
            std::transform(dv[i].begin(), dv[i].end(), res.begin(), res.begin(), std::plus<int>());

        return res;
    };

    // get open-door goods counting after detection is over
    auto openFusionLayerResult = std::move(static_cam::gsFusion->output_final_count(0));
    openFusionResult = combineResult(openFusionLayerResult);
    printVector(openFusionResult, std::string("open result"));
    // printName(openFusionResult);

    ss_sem.wait();
#if 0
    for (auto& visionPtr: vision_table){
        if (visionPtr->visionStop()){
            cerr<<"FATAL: visionStop Failed!"<<endl;
            //TODO: add exception handler;
        }
    }
#endif
    VisionLib::Post_Capture(capture_table, yolo::data::closeImages);

    futureCaptures.clear();
    openImages.clear();
    closeImages.clear();

    // get close-door goods counting after detection is over
    auto closeFusionLayerResult = std::move(static_cam::gsFusion->output_final_count(1));
    closeFusionResult = combineResult(closeFusionLayerResult);
    printVector(closeFusionResult, std::string("close result"));
    // printName(closeFusionResult);
    assert(openFusionResult.size() == closeFusionResult.size() && "FATAL: length of fusion results do not equal");

    std::ofstream ofsForCounting;
    ofsForCounting.open(gDumpFolder + "/count.log");
    ofsForCounting << "#################### before ####################\n";
    for (auto i = 0; i < shelfInfo.layerCount; ++i)
    {
        ofsForCounting << "layer" << i << ":\n";
        for (auto j = 0; j < openFusionLayerResult.at(i).size(); ++j)
        {
            auto count = openFusionLayerResult[i].at(j);
            if (count > 0)
            {
                auto name = find_good_code(j);
                ofsForCounting << name << "\t" << count << "\n";
            }
        }
        ofsForCounting << "\n";
    }
    ofsForCounting << "\n";
    ofsForCounting << "##################### after ####################\n";
    for (auto i = 0; i < shelfInfo.layerCount; ++i)
    {
        ofsForCounting << "layer" << i << ":\n";
        for (auto j = 0; j < closeFusionLayerResult.at(i).size(); ++j)
        {
            auto count = closeFusionLayerResult[i].at(j);
            if (count > 0)
            {
                auto name = find_good_code(j);
                ofsForCounting << name << "\t" << count << "\n";
            }
        }
        ofsForCounting << "\n";
    }
    ofsForCounting.close();


    auto f_result = std::vector<int>(openFusionResult.size());
    std::transform(openFusionResult.begin(), openFusionResult.end(), closeFusionResult.begin(), f_result.begin(), std::minus<int>());
    printVector(f_result, std::string("combined result"));
    printName(f_result);

    // construct the fusion result to meet java callback
    // auto f_result = yolo::data::static_cam::gsFusion->output_final_count(yolo::data::gDumpFolder);

    std::vector<std::vector<float>> fusion_result;
    int len = 0;
    for (auto&& p: f_result) {
//        cout << "fusion result: " << p << endl;
        if (p > 0) {
            for (int j = 0; j < p; j++) {
                std::vector<float> res(f_result.size(), 0.1);
                res[len] = 0.99;
                fusion_result.push_back(std::move(res));
            }
        }
        len++;
     }

    //auto fusion_result = yolo::data::pfusion->output_video_prob(INT64_MAX);

    for (int fusion_id = 0; fusion_id < fusion_result.size(); fusion_id++) {
        yolo::data::fusion_dbg_fp << " Fusion " << fusion_id << " prob: " << endl;

        for (int c = 0; c < yolo::data::obj_names.size(); c++) {
            float pc = fusion_result[fusion_id][c];
            if (fabs(pc) > 0.0001)
                yolo::data::fusion_dbg_fp << yolo::data::obj_names[c] << " :" << pc << endl;
        }
        yolo::data::fusion_dbg_fp << endl << endl;
    }

    yolo::native_callback(fusion_result, 0.24);
    yolo::data::fusion_dbg_fp.close();

    ss_sem.notify(); 
    //compress the current dump_native folder
    if (t_compress.joinable()){
        t_compress.join();
    }
#ifndef DEBUG_WITH_FAKE_FRAME
    std::string zipFolder = yolo::data::gDumpFolder;
    t_compress = std::thread([=](){
            char cmd[512];
            perf p("compress dump folder");
            cout<<"compress dump folder: "<<zipFolder<<endl;
            const char* pDump= zipFolder.c_str();
            sprintf(cmd, "test -d %s && tar czf %s.tgz.tmp %s && mv %s.tgz.tmp %s.tgz && rm -rf %s", pDump, pDump, pDump, pDump, pDump, pDump);
            system(cmd);
    });
#endif
    cout<<__FUNCTION__<<" X"<<endl;
    return 0;
}
extern "C" int VisionExit_Wrap(){
    cout<<__FUNCTION__<<" E"<<endl;

    vision_table.clear();

    if (t_compress.joinable()){
        t_compress.join();
    }

    cout<<__FUNCTION__<<" X"<<endl;

    return 0;
}
extern "C" int DoorClosed_Wrap()
{
    cout<<__FUNCTION__<<" E"<<endl;
    perf p(__FUNCTION__);
    //notify_door_closed();

    return 0;
}

#endif  //#ifdef VISIONLIB
