#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <libv4l2.h>

#include "CameraConfig.h"

#include "v4l2_util.h"

#ifndef V4L2_CTRL_ID2CLASS
#define V4L2_CTRL_ID2CLASS(id)    ((id) & 0x0fff0000UL)
#endif

static uint8_t disable_libv4l2 = 0; /*set to 1 to disable libv4l2 calls*/


int _xioctl(int fd, int IOCTL_X, void *arg)
{
    int ret = 0;
    int tries= IOCTL_RETRY;
    do {
        if(!disable_libv4l2)
            ret = v4l2_ioctl(fd, IOCTL_X, arg);
        else
            ret = ioctl(fd, IOCTL_X, arg);
    }
    while (ret && tries-- &&
            ((errno == EINTR) || (errno == EAGAIN) || (errno == ETIMEDOUT)));

    if (ret && (tries <= 0)) fprintf(stderr, "V4L2_CORE: ioctl (%i) retried %i times - giving up: %s)\n", IOCTL_X, IOCTL_RETRY, strerror(errno));

    return (ret);
}

int _init_v4l2(const char* videodevice)
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
	ret = _xioctl(fd, VIDIOC_QUERYCAP, &cap);
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
int _close_v4l2(int fd)
{
    close(fd);
    return 0;
}

int _load_controls(int fd, const CameraConfig& cfg)
{
    const auto& v4l2_cfg = cfg.v4l2_cfg;

    usleep(cfg.open_latency*1000);

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
            int32_t class_type = V4L2_CTRL_ID2CLASS(cid);
            if (class_type == V4L2_CTRL_CLASS_USER){

                struct v4l2_control control;
                memset (&control, 0, sizeof (control));

                control.id = cid;
                control.value = val;

                /*
                   if (cid == V4L2_CID_EXPOSURE_AUTO)
                   {
                   control.id = cid;
                   control.value = 3;
                   if (ioctl(fd, VIDIOC_S_CTRL, &control)){
                   printf("ERROR set str:%s id:%d val:%d\n", str, control.id, control.value);
                   continue;
                   }
                   control.value = val;
                   }
                   */
                if (_xioctl(fd, VIDIOC_S_CTRL, &control)){
                    printf("ERROR VIDIOC_S_CTRL str:%s id:%d val:%d. (%s)\n", str, control.id, control.value, strerror(errno));
                    continue;
                }
//#ifdef DEBUG_CAMERA_CFG
                printf("OK  VIDIOC_S_CTRL str:%s id:%d val:%d\n", str, control.id, control.value);
//#endif
            }
            else{
                //printf("V4L2_CORE: using VIDIOC_S_EXT_CTRLS on single controls for class: 0x%08x\n", class_type);

                struct v4l2_ext_control control={0};
                control.id = cid;
                control.value64 = val;
                control.value = val;

                struct v4l2_ext_controls ctrls={0};
                ctrls.ctrl_class = class_type;
                ctrls.count = 1;
                ctrls.controls = &control;

                if(_xioctl(fd, VIDIOC_S_EXT_CTRLS, &ctrls)){
                    printf("ERROR VIDIOC_S_EXT_CTRLS str:%s id:%d val:%d. (%s)\n", str, control.id, control.value, strerror(errno));
                    continue;
                }
//#ifdef DEBUG_CAMERA_CFG
                printf("OK  VIDIOC_S_EXT_CTRLS str:%s id:%d val:%d\n", str, control.id, control.value);
//#endif
            }
            usleep(cfg.cmd_latency*1000);
        }
        else{
            printf("FAIL not found str:%s val:%d\n", str, val);
        }
    }
    return 0;
}

int _check_controls(int fd, const CameraConfig& cfg)
{
    const auto& v4l2_cfg = cfg.v4l2_cfg;

    usleep(cfg.open_latency*1000);

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
            if (_xioctl(fd, VIDIOC_G_CTRL, &control)){
                printf("ERROR VIDIOC_G_CTRL str:%s id:%d val:%d (%s)\n", str, control.id, control.value, strerror(errno));
                continue;
            }
//#ifdef DEBUG_CAMERA_CFG
            printf("OK  VIDIOC_G_CTRL str:%s id:%d val:%d\n", str, control.id, control.value);
//#endif
#ifndef DEBUG_NO_STRICT_MODE
            assert(control.value == val);
#endif

            usleep(cfg.cmd_latency*1000);
        }
        else{
            printf("FAIL not found str:%s , expected val:%d\n", str, val);
        }
    }
    return 0;
}
