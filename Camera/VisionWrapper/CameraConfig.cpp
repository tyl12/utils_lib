#include <iostream>
#include <fstream>
#include <stdio.h>
#include <vector>
#include <map>

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

// rapidjson/example/simpledom/simpledom.cpp`
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"


#include "utils.h"
#include "CameraConfig.h"

#define BUF_LEN_JSON (4096)

const vector<pair<const char* ,int>> v4l2_str_cid_table = {
    make_pair(STR_BRIGHTNESS, V4L2_CID_BRIGHTNESS),
    make_pair(STR_CONTRAST, V4L2_CID_CONTRAST),
    make_pair(STR_SATURATION,V4L2_CID_SATURATION),
    make_pair(STR_HUE,V4L2_CID_HUE),
    make_pair(STR_WBTA,V4L2_CID_AUTO_WHITE_BALANCE),
    make_pair(STR_WBT,V4L2_CID_WHITE_BALANCE_TEMPERATURE),
    make_pair(STR_GAMMA,V4L2_CID_GAMMA),
    make_pair(STR_GAIN,V4L2_CID_GAIN),
    make_pair(STR_SHARP,V4L2_CID_SHARPNESS),
    make_pair(STR_BACKLIGHT_COMP,V4L2_CID_BACKLIGHT_COMPENSATION),
    make_pair(STR_EXPO_AUTO,V4L2_CID_EXPOSURE_AUTO),
    make_pair(STR_EXPO_AUTOPRI,V4L2_CID_EXPOSURE_AUTO_PRIORITY),
    make_pair(STR_EXPO_ABS,V4L2_CID_EXPOSURE_ABSOLUTE)
};

static int check_member(const Value& vs, const char* STR){
    if (!vs.HasMember(STR)){
#ifdef DEBUG_CAMERA_CFG
        LOGD("No member '%s' defined in cfg file, skip", STR);
#endif
        return -1;
    }
    return 0;
}

static int parse_string(string& out, const Value& vs, const char* STR){
    if (check_member(vs, STR))
        return -1;
    out = vs[STR].GetString();
    return 0;
}
static int parse_int(int& out, const Value& vs, const char* STR){
    if (check_member(vs, STR))
        return -1;
    out = vs[STR].GetInt();
    return 0;
}

static int parse_v4l2(vector<pair<const char*, int>>& v4l2_cfg, const Value& vs, const char* STR){
    if (check_member(vs, STR))
        return -1;
    /*
    if (strcmp(STR,"exposure_auto")==0){
        cout<<"#@##"<<vs[STR][0].GetInt()<<endl;
        cout<<"#@##"<<vs[STR][1].GetInt()<<endl;
    }
    */
    v4l2_cfg.push_back(make_pair(STR, vs[STR].GetInt()));
    return 0;
}

static int parse_v4l2_group(vector<pair<const char*, int>>& v4l2_cfg, const Value& vg, const char* STR){
    if (check_member(vg, STR))
        return -1;

    const Value& vs = vg[STR];

    for (const auto& str_cid:v4l2_str_cid_table){
        const char* str = str_cid.first;
#ifdef DEBUG_CAMERA_CFG
        LOGD("parse v4l2 entry: %s", str);
#endif
        parse_v4l2(v4l2_cfg, vs, str);
    }
    return 0;
}

//return 0 for success, otherwise fail
int parse_entry(const Value& v, CameraConfig& cfg)
{
    if (v.HasMember(STR_IGNORE)){ //notes: if IGNORE found, skip this one.
        LOGE("Ignore current entry: %s", v[STR_IGNORE].GetString());
        return -1;
    }

    if (!v.HasMember(STR_PCI)){
        LOGE("No node: %s", STR_PCI);//notes: keep handling next entry if no valid pci-id found.
        //return -1;
    }
    else{
        cfg.pci = v[STR_PCI].GetString();
    }

    if (v.HasMember(STR_CROP)){
        const Value & croplist = v[STR_CROP];
        assert(croplist.IsArray());
        for (auto i = 0; i < croplist.Size(); ++i) {
            const Value & vi = croplist[i];
            assert(vi.IsInt());
            cfg.crop[i] = vi.GetInt();
        }
    }
    parse_int(cfg.rotate, v, STR_ROTATE);
    //notes: the range of rotate is in [0, 90, 180, 270, 360]
    if(0 <= cfg.rotate <= 360) {
        cfg.rotate = cfg.rotate / 90 * 90;
    }else {
        cfg.rotate = 0;
    }
    parse_string(cfg.trackId, v, STR_TRACKID);
    parse_string(cfg.format, v, STR_FORMAT);
    parse_string(cfg.desc, v, STR_DESC);
    parse_string(cfg.pos, v, STR_POS);
    parse_string(cfg.usage, v, STR_USAGE);
    parse_int(cfg.training, v, STR_TRAINING);
    parse_int(cfg.frame_skip, v, STR_FRAME_SKIP);
    parse_int(cfg.frame_rate, v, STR_FRAME_RATE);
    parse_int(cfg.width, v, STR_WIDTH);
    parse_int(cfg.height, v, STR_HEIGHT);
    parse_int(cfg.cmd_latency, v, STR_CMD_LATENCY);
    parse_int(cfg.open_latency, v, STR_OPEN_LATENCY);

    //parse_v4l2_group(cfg.v4l2_cfg, v, STR_V4L2_CTRL);

    vector<pair<const char*, int>> v4l2_cfg_update;
    parse_v4l2_group(v4l2_cfg_update, v, STR_V4L2_CTRL);

    for (const auto& cfg_update:v4l2_cfg_update){
        string key = cfg_update.first;
        int val = cfg_update.second;
        bool found = false;
        for (auto& cfg_def:cfg.v4l2_cfg){
            string key_def = cfg_def.first;
            if (key_def == key){
                cfg_def.second = val;
                found = true;
                break;
            }
        }
        if (!found){
            cfg.v4l2_cfg.push_back(cfg_update);
        }
    }

    return 0;
}

void CameraConfig::show() const{
    const char TAB1[]="    ";
    const char TAB2[]="        ";
    cout<<">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"<<endl;
    cout<<"CamInfo:"<<endl
        <<TAB1<<"VIDEODEVICE"<<" = "<<videodevice<<endl
        <<TAB1<<STR_PCI<<" = "<<pci<<endl
        <<TAB1<<STR_TRACKID<<" = "<<trackId<<endl
        <<TAB1<<STR_DESC<<" = "<<desc<<endl
        <<TAB1<<STR_POS<<" = "<<pos<<endl
        <<TAB1<<STR_OPTIONAL<<" = "<<optional<<endl
        <<TAB1<<STR_FRAME_SKIP<<" = "<<frame_skip<<endl
        <<TAB1<<STR_FRAME_RATE<<" = "<<frame_rate<<endl
        <<TAB1<<STR_USAGE<<" = "<<usage<<endl
        <<TAB1<<STR_CROP<<" = "<<crop[0]<<","<<crop[1]<<","<<crop[2]<<","<<crop[3]<<endl
        <<TAB1<<STR_ROTATE<<" = "<<rotate<<endl
        <<TAB1<<STR_FORMAT<<" = "<<format<<endl
        <<TAB1<<STR_CMD_LATENCY<<" = "<<cmd_latency<<endl
        <<TAB1<<STR_OPEN_LATENCY<<" = "<<open_latency<<endl
        <<TAB1<<STR_TRAINING<<" = "<<training<<endl
        <<TAB1<<STR_WIDTH<<" = "<<width<<endl
        <<TAB1<<STR_HEIGHT<<" = "<<height<<endl
        <<TAB1<<STR_V4L2_CTRL<<endl;

    for (const auto& str_val:v4l2_cfg){
        const string& str = str_val.first;
        const int val = str_val.second;
        cout<<TAB2<<str<<" = "<<val<<endl;
    }
    cout<<"<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<"<<endl;
    cout<<endl;
}

vector<CameraConfig> parse_json(const char* jsonfile){
    vector<CameraConfig> camVect;

    FILE *fp = fopen(jsonfile , "rb");
    if (fp == nullptr){
        LOGE("Fail to find the json config file: %s", jsonfile);
        exit( 1 );
    }
    char readbuffer[BUF_LEN_JSON];
    FileReadStream frs(fp , readbuffer , sizeof(readbuffer));
    Document doc;
    doc.ParseStream(frs);
    fclose(fp);

    assert(!doc.HasParseError());

    //STR_COMMENT is ignored
    if (doc.HasMember(STR_COMMENT)){
        cout<<"Found comment entry, was ignored directly"<<endl;
    }


    CameraConfig defaultCfg;
    if (doc.HasMember(STR_DEFAULT)){
        LOGD("found default entry");

        //提取数组元素（声明的变量必须为引用）
        Value &defv = doc[STR_DEFAULT];

        assert(defv.IsObject());
        if (parse_entry(defv, defaultCfg)){
            LOGE("Fail to parse default entry");
        }
        else{
            LOGD("\n\nDefault camera config:");
            defaultCfg.show();
        }
        cout<<endl;
    }

    //parse override entry
    if (doc.HasMember(STR_OVERRIDE)){
        Value &vs = doc[STR_OVERRIDE];
        assert(vs.IsArray());

        for (auto i = 0; i<vs.Size(); i++)
        {
            CameraConfig cfg(defaultCfg);

            //逐个提取数组元素（声明的变量必须为引用）
            Value &v = vs[i];

            if (parse_entry(v, cfg)){
                LOGE("Fail to parse entry, skip");
                continue;
            }

            cfg.show();
            camVect.push_back(cfg);
        }
    }

    CameraConfig standaloneCam;
    if (doc.HasMember(STR_STANDALONE)){
        printf("found standalone entry\n");

        //提取数组元素（声明的变量必须为引用）
        Value &stdav = doc[STR_STANDALONE];

        if (!stdav.IsObject()) throw std::runtime_error("bad standalone config");

        if (parse_entry(stdav, standaloneCam))
        {
            printf("Fail to parse standalone entry\n");
        }
        // cam.show(); /* redundant */
        camVect.push_back(standaloneCam);
    }
    return camVect;
}

/****************************************************************************/
#if 0
//return fd
static int init_v4l2(const char* videodevice)
{
	int i;
	int ret = 0;

    int fd;
	if ((fd = open(videodevice, O_RDWR)) == -1) {
		perror("ERROR opening V4L interface");
		exit(1);
	}

    struct v4l2_capability cap;
	memset(&cap, 0, sizeof(struct v4l2_capability));
	ret = ioctl(fd, VIDIOC_QUERYCAP, &cap);
	if (ret < 0) {
		printf("Error opening device %s: unable to query device.\n",
				videodevice);
		goto fatal;
	}

	if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
#ifdef DEBUG_CAMERA_CFG
		printf("Error opening device %s: video capture not supported.\n", videodevice);
#endif
	}
	if (!(cap.capabilities & V4L2_CAP_STREAMING)) {
#ifdef DEBUG_CAMERA_CFG
        printf("%s does not support streaming i/o\n", videodevice);
#endif
    }
	if (!(cap.capabilities & V4L2_CAP_READWRITE)) {
#ifdef DEBUG_CAMERA_CFG
			printf("%s does not support read i/o\n", videodevice);
#endif
	}
fatal:
    return fd;

}


static int close_v4l2(int fd)
{
    close(fd);
    return 0;
}

int load_controls(int fd, const CamCtrl& cam) //struct vdIn *vd)
{
    const auto& v4l2_cfg = cam.v4l2_cfg;

    //TODO:add latency
    usleep(cam.open_latency*1000);

    for (const auto& str_val:v4l2_cfg){
        const char* str = str_val.first;
        int val = str_val.second;

        int cid = -1;
        bool found = false;
        for (const auto& str_cid:v4l2_str_cid_table){
            const char* str_search = str_cid.first;
            if (strcmp(str_search, str)==0){
                cid = str_cid.second;
                found = true;
                break;
            }
        }
        if (found){
            struct v4l2_control control;
            memset (&control, 0, sizeof (control));

            control.id = cid;
            control.value = val;

            if (ioctl(fd, VIDIOC_S_CTRL, &control)){
                printf("ERROR str:%s id:%d val:%d\n", str, control.id, control.value);
                continue;
            }
//#ifdef DEBUG_CAMERA_CFG
            printf("OK  str:%s id:%d val:%d\n", str, control.id, control.value);
//#endif
            //TODO:add latency
            usleep(cam.cmd_latency*1000);
        }
        else{
            printf("FAIL not found str:%s val:%d\n", str, val);
        }
    }
    return 0;
}

int setCfg(const CamCtrl& cam){
    //find videonode via pci bus id.
    //init node
    const char* videodevice = cam.getVideoDevice();
    int fd = init_v4l2(videodevice);
    //cfg node
    load_controls(fd, cam);

    //close node
    close_v4l2(fd);

    return 0;
}
#endif

#include "ports_map_videos.h"


//load camera config from json file, return 0 if success, fail otherwise.
int loadCameraJsonConfig(string path, vector<CameraConfig>& cfgs)
{
    //parse pci-id & videonode
    auto resultvect = std::move(wrapper_pciport_videonode());
    for (const auto& p:resultvect){
        string portid = p.first;
        std::string videonode = p.second;
        LOGD("pci-port: %s,  video-node: %s", portid.c_str() , videonode.c_str());
    }
    //parse camera_cfg.json file
    auto jsonCfgs = parse_json(path.c_str());

#ifdef DEBUG_WITH_FAKE_CAMERA
    cfgs = jsonCfgs;
    return 0;
#endif

    //NOTES: we'll only handle the node exists on both json file & dev node
    for(auto& cfg:jsonCfgs){
        string videonode;
        bool found = false;
        for (auto p:resultvect){
            string portid = p.first;
            if (cfg.getPciId() == portid){
                videonode = p.second;
                found = true;
                break;
            }
        }
        //update camctrl::videodevice info based on pci id
        if (found){
            LOGD("Found pci-id %s, videonode %s", cfg.getPciId().c_str(), videonode.c_str());
            cfg.setVideoDevice(videonode.c_str());
            cfgs.push_back(cfg);
        }
        else{
            LOGE("Fail to find pci-id %s, which is specified in %s", cfg.getPciId().c_str(), path.c_str());
#ifdef DEBUG_NO_STRICT_MODE
            LOGE("DEBUG_NO_STRICT_MODE defined. force accept the config");
            cfg.setVideoDevice("/dev/video_debug");
            cfgs.push_back(cfg);
#endif
            if (!cfg.isOptional())
                throw std::runtime_error("camera connection check failure");
        }
    }

    LOGD(" Final valid video nodes:");
    for(const auto& cfg:cfgs){
        cfg.show();
    }
    if (cfgs.size() != cfgs.size())
        return -1;

    /* TODO
    if (sendCfg){
        for(const auto& cfg:cfgs){
            cout<<__FUNCTION__<<" send v4l2 cfg to camera " << cfg.getPciId().c_str()<<" " <<cfg.getVideoDevice()<<endl;
            setCfg(cfg);
        }
    }
    else{
        cout<<__FUNCTION__<<" sendCfg set false, skiip send v4l2 cfg to camera"<<endl;
    }
    */

    return 0;
}




#ifdef DEBUG_CAMERA_CFG
int main(){
    vector<CameraConfig> cfgs;
    loadCameraJsonConfig("./camera_cfg.json", cfgs);
}
#endif

