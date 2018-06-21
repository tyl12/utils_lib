#ifndef V4L2_UTIL_H
#define V4L2_UTIL_H
class CameraConfig;

#define IOCTL_RETRY 4


int _init_v4l2(const char* videodevice);
int _close_v4l2(int fd);
int _load_controls(int fd, const CameraConfig& cam);
int _check_controls(int fd, const CameraConfig& cam);

#endif
