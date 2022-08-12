
### LIBAPC All Source Files ###
FILE(GLOB DSL_SOURCES_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)


### Make STATIC library (.so) ###
ADD_LIBRARY(dsl SHARED ${DSL_SOURCES_FILES})


### INCLUDE DIRECTORY ###
TARGET_INCLUDE_DIRECTORIES(dsl PUBLIC .)


### Find Package ###
FIND_PACKAGE(PkgConfig REQUIRED)

PKG_CHECK_MODULES(GSTREAMER-VIDEO REQUIRED gstreamer-video-1.0)
PKG_CHECK_MODULES(GSTREAMER-RTSP REQUIRED gstreamer-rtsp-server-1.0)
PKG_CHECK_MODULES(DEEPSTREAM REQUIRED gstreamer-rtsp-server-1.0)
PKG_CHECK_MODULES(OPENCV4 REQUIRED opencv4)


### Find Lib ###
FIND_LIBRARY( NVDSGST_META
	NAMES nvdsgst_meta
	PATHS ${SDK_PATH}/opt/nvidia/deepstream/deepstream-6.0/lib/
)

FIND_LIBRARY( NVDSMSGBRK
	NAMES nvds_msgbroker
	PATHS ${SDK_PATH}/opt/nvidia/deepstream/deepstream-6.0/lib/
)

FIND_LIBRARY( CUDART 
	NAMES cudart
	PATHS ${SDK_PATH}/usr/local/cuda-10.2/lib/
)

FIND_LIBRARY( NVDSGST 
	NAMES nvdsgst_smartrecord
	PATHS ${SDK_PATH}/opt/nvidia/deepstream/deepstream-6.0/lib/
)

FIND_LIBRARY( NVDSGSTDEEP
	NAMES nvdsgst_deepstream_bins
	PATHS ${SDK_PATH}/usr/lib/gstreamer-1.0/deepstream/
)
 
FIND_LIBRARY( NVDSGSTHELPER
	NAMES nvdsgst_helper
	PATHS ${SDK_PATH}/opt/nvidia/deepstream/deepstream-6.0/lib/
)

FIND_LIBRARY( OPENCVCORE
	NAMES opencv_core
	PATHS ${SDK_PATH}/usr/lib/
)


### Link Library ###
TARGET_LINK_LIBRARIES(dsl
	glib-2.0
	gstreamer-1.0
	geos_c
	geos
	cuda
	X11
	opencv_imgproc
	opencv_highgui
	curl
	nvbufsurface
	nvbufsurftransform
	gstvideo-1.0
	randomcolor
	nmp
	${NVDSGST}
	${NVDSMSGBRK}
	${NVDSGST_META}
	${CUDART}
	${NVDSGSTDEEP}
	${OPENCVCORE}
    ${NVDSGSTHELPER}
    ${GSTREAMER-VIDEO_LIBRARIES}
    ${GSTREAMER-RTSP_LIBRARIES}
    ${DEEPSTREAM_LIBRARIES}
    ${OPENCV4_LIBRARIES}
)


### INSTALL TARGET ###
INSTALL(TARGETS dsl
    DESTINATION ${CMAKE_LIB_PATH}
)
