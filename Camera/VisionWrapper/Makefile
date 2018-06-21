LDFLAGS+= `pkg-config --libs opencv`  -lpthread -lv4l2
CFLAGS+= `pkg-config --cflags opencv`

OUTDIR=install
CFLAGS += -I ../../build/install
#crypt is used by libutils.so
LDFLAGS += -L../../build/install -lutils -lcrypt

all: prepare
	g++ -std=c++11  -o $(OUTDIR)/debug_camera_pci  $(CFLAGS) ports_map_videos.cpp $(LDFLAGS) -DDEBUG_CAMERA_PCI
	g++ -std=c++11  -o $(OUTDIR)/debug_camera_cfg  $(CFLAGS) CameraConfig.cpp ports_map_videos.cpp  $(LDFLAGS) -DDEBUG_CAMERA_CFG  -DDEBUG_NO_STRICT_MODE
	g++ -std=c++11  -o $(OUTDIR)/debug_camera_ctrl $(CFLAGS) v4l2_util.cpp CameraModule.cpp CameraConfig.cpp ports_map_videos.cpp  $(LDFLAGS) -DDEBUG_CAMERA_CTRL  -DDEBUG_NO_STRICT_MODE
	g++ -std=c++11  -o $(OUTDIR)/debug_camera_cnr  $(CFLAGS) v4l2_util.cpp CameraContainer.cpp CameraModule.cpp CameraConfig.cpp ports_map_videos.cpp $(LDFLAGS) -DDEBUG_CAMERA_CONTAINER  -DDEBUG_NO_STRICT_MODE
	#-lX11 for x11 lib
	g++ -std=c++11  -o $(OUTDIR)/debug_camera_view $(CFLAGS) v4l2_util.cpp CameraDebug.cpp CameraModule.cpp CameraConfig.cpp ports_map_videos.cpp $(LDFLAGS) -lX11  -DDEBUG_NO_STRICT_MODE 
	cp camera_cfg.json $(OUTDIR)/

.PHONY: prepare
prepare:
	mkdir -p $(OUTDIR)

.PHONY: clean
clean:
	rm -rf $(OUTDIR)

.PHONY: test
test:
	export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:../../build/install && ./install/debug_camera_pci
	export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:../../build/install && ./install/debug_camera_cfg
	export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:../../build/install && ./install/debug_camera_ctrl
	export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:../../build/install && ./install/debug_camera_cnr
	export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:../../build/install && ./install/debug_camera_view
