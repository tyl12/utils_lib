#ifndef CAMERA_CONFIG_H
#define CAMERA_CONFIG_H

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <map>
#include <regex>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <string.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <signal.h>

#include "rapidjson/document.h" //for rapidjson::Value

#include "v4l2_util.h"
#include "utils.h"

using namespace rapidjson;
using namespace std;
using namespace utils;

//---------------------------------------------------------------------

static const char STR_COMMENT[] = "comment";
static const char STR_IGNORE[] = "ignore";
static const char STR_DEFAULT[] = "default";
static const char STR_OVERRIDE[] = "override";
static const char STR_STANDALONE[] = "standalone";
static const char STR_PCI[] = "pci-id";
static const char STR_TRACKID[] = "track-id";
static const char STR_DESC[] = "desc";
static const char STR_POS[] = "pos";
static const char STR_USAGE[] = "usage";
static const char STR_CROP[] = "crop";
static const char STR_FRAME_SKIP[] = "frame-skip";
static const char STR_FRAME_RATE[] = "frame-rate";
static const char STR_ROTATE[] = "rotate";
static const char STR_FORMAT[] = "format";
static const char STR_TRAINING[] = "training";
static const char STR_WIDTH[] = "width";
static const char STR_HEIGHT[] = "height";
static const char STR_CMD_LATENCY[] = "cmd-latency";
static const char STR_OPEN_LATENCY[] = "open-latency";
static const char STR_OPTIONAL[] = "optional";
static const char STR_TRUE[] = "true";
static const char STR_FALSE[] = "false";


//v4l2 related
static const char STR_V4L2_CTRL[] = "v4l2-config";
static const char STR_BRIGHTNESS[] = "brightness";
static const char STR_CONTRAST[] = "contrast";
static const char STR_SATURATION[] = "saturation";
static const char STR_HUE[] = "hue";
static const char STR_WBTA[] = "white_balance_temperature_auto";
static const char STR_WBT[] = "white_balance_temperature";
static const char STR_GAMMA[] = "gamma";
static const char STR_PLF[] = "power_line_frequency"; //"1"=50Hz
static const char STR_GAIN[] = "gain";
static const char STR_SHARP[] = "sharpness";
static const char STR_BACKLIGHT_COMP[] = "backlight_compensation";
static const char STR_EXPO_AUTO[] = "exposure_auto";
static const char STR_EXPO_AUTOPRI[] = "exposure_auto_priority";
static const char STR_EXPO_ABS[] = "exposure_absolute";

extern const vector<pair<const char* ,int>> v4l2_str_cid_table;
//---------------------------------------------------------------------

class CameraConfig;
vector<CameraConfig> parse_json(const char* jsonfile);
int parse_entry(const rapidjson::Value& v, CameraConfig& cfg);
int loadCameraJsonConfig(string path, vector<CameraConfig>& cfgs);

class CameraConfig
{
    private:
        string videodevice;

        //pci id is mandatory
        string pci;
        string trackId;
        string desc;
        string pos;
        string usage;
        int crop[4];
        int frame_skip;
        int frame_rate;
        int rotate;
        string format;
        int training;
        int width;
        int height;
        int optional;

        //ms between two commands
        int cmd_latency;
        //ms between open & 1st command
        int open_latency;

        vector<pair<const char*, int>> v4l2_cfg;

    public:
        CameraConfig():
            optional(0),
            desc(""),
            usage(""),
            rotate(0),
            frame_skip(8),
            frame_rate(10),
            training(0),
            format("jpeg"),
            width(1280),
            height(720),
            cmd_latency(100), //ms
            open_latency(100)
        {}
        ~CameraConfig(){}

        const char* getVideoDevice() const{ return videodevice.c_str(); }
        int setVideoDevice(const char* dev){
            videodevice = string(dev);
            return 0;
        }
        const std::vector<int> get_crop() const {
            std::vector<int> crop_rect;
            crop_rect.resize(4);
            std::copy(crop, crop + 4, crop_rect.begin());
            return crop_rect;
        }

        int get_rotate() const{ return rotate; }
        int get_width() const{ return width; }
        int get_height() const{ return height; }
        string getPciId() const{ return pci; }
        string getTrackId() const{ return trackId; }
        int getFrameSkip() const { return frame_skip;}
        int getFrameRate() const { return frame_rate;}

        const string getDesc() const { return desc; }
        const string getPos() const { return pos; }
        const void getPosParsed(int& row, int& col) const {
            string pos = getPos();
            vector<string>&& p = split_by_delim(utils::trim(pos), '-');
            assert(p.size() == 1 || p.size() == 2);
            if (p.size() == 2){
                row = stoi(p[0]); col = stoi(p[1]);
            }
            else{
                row = stoi(p[0]); col = 0;
            }
        }

        string getDetailInfo()const {
            auto idx = getVideoNodeIdx();
            return string("video")+to_string(idx)+"_pos"+pos+"_tkid"+trackId+"_"+to_string(width)+"x"+to_string(height);
        }

        const bool isOptional(){ return (optional != 0); }
        bool isUsageStream() const { return (strcmp(usage.c_str(), "stream") == 0 ? true:false); }
        bool isUsageCapture()const { return (strcmp(usage.c_str(), "capture") == 0 ? true:false); }
        const bool isUsageMonitor() const { return (strcmp(usage.c_str(), "monitor") == 0? true : false); }
        void show() const;

        int getVideoNodeIdx() const{
            int nodeid = -1;
            if (videodevice.empty()){
                cerr<<"videodevice node is still not updated. return -1 by default."<<endl;
                return nodeid;
            }

            std::regex devpat("/dev/video([0-9]+)");
            std::smatch s;
            bool found = std::regex_match(videodevice, s, devpat);
            if (found){
                nodeid = std::stoi(s[1].str());
                //std::cout<<s[1].str()<<std::endl;
            }
            return nodeid;
        }

    friend vector<CameraConfig> parse_json(const char* jsonfile);
    friend int parse_entry(const rapidjson::Value& v, CameraConfig& cfg);
    friend int loadCameraJsonConfig(string path, vector<CameraConfig>& cfgs);

    friend int _init_v4l2(const char* videodevice);
    friend int _load_controls(int fd, const CameraConfig& cfg);
    friend int _check_controls(int fd, const CameraConfig& cfg);
    friend int _close_v4l2(int fd);

    /*
    private:
        static vector<CameraConfig> mAllCfgs;
    public:
    static loadCameraJsonConfig
    */
};

#if 0
//not take it as member func, so that we could seperate v4l2 ops and camera settings.
friend int load_controls(int fd, const CamCtrl& cam);
#endif

#endif
