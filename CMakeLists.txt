cmake_minimum_required(VERSION 3.5.1)

### EXTERNAL VARIABLE DEFINE ###
ADD_COMPILE_DEFINITIONS(DSL_VERSION=L"v0.25.alpha")
ADD_COMPILE_DEFINITIONS(GSTREAMER_SUB_VERSION=14)
ADD_COMPILE_DEFINITIONS(NVDS_DCF_LIB="/opt/nvidia/deepstream/deepstream-6.0/lib/libnvds_nvdcf.so")
ADD_COMPILE_DEFINITIONS(NVDS_KLT_LIB="/opt/nvidia/deepstream/deepstream-6.0/lib/libnvds_mot_klt.so")
ADD_COMPILE_DEFINITIONS(NVDS_IOU_LIB="/opt/nvidia/deepstream/deepstream-6.0/lib/libnvds_mot_iou.so")
ADD_COMPILE_DEFINITIONS(NVDS_MOT_LIB="/opt/nvidia/deepstream/deepstream-6.0/lib/libnvds_nvmultiobjecttracker.so")
ADD_COMPILE_DEFINITIONS(NVDS_AMQP_PROTO_LIB="/opt/nvidia/deepstream/deepstream-6.0/lib/libnvds_amqp_proto.so")
ADD_COMPILE_DEFINITIONS(NVDS_AZURE_PROTO_LIB="/opt/nvidia/deepstream/deepstream-6.0/lib/libnvds_azure_proto.so")
ADD_COMPILE_DEFINITIONS(NVDS_AZURE_EDGE_PROTO_LIB="/opt/nvidia/deepstream/deepstream-6.0/lib/libnvds_azure_edge_proto")
ADD_COMPILE_DEFINITIONS(NVDS_KAFKA_PROTO_LIB="/opt/nvidia/deepstream/deepstream-6.0/lib/libnvds_kafka_proto.so")
ADD_COMPILE_DEFINITIONS(NVDS_REDIS_PROTO_LIB="/opt/nvidia/deepstream/deepstream-6.0/lib/libnvds_redis_proto.so")
ADD_DEFINITIONS(-DBUILD_NMP_PPH=true)
ADD_DEFINITIONS(-DBUILD_INTER_PIPE=false)

### INCLUDE DIRECTORIES ###
INCLUDE_DIRECTORIES(
    ${INCLUDE_PATH}
    ${INCLUDE_PATH}/glib-2.0
    ${INCLUDE_PATH}/glib-2.0/include
    ${INCLUDE_PATH}/gstreamer-1.0
    ${INCLUDE_PATH}/gstreamer-1.0/include
    ${INCLUDE_PATH}/opencv4
    ${LIB_PATH}/glib-2.0/include
    ${CUDA_PATH}/include
    ${GST_NVDS}/include
    src
    src/thirdparty
	src/nmp
	examples/cpp/yaml
)


### ADD BUILD SUBDIRECTORIES ###
ADD_SUBDIRECTORY(src/thirdparty)
ADD_SUBDIRECTORY(src)
ADD_SUBDIRECTORY(src/nmp)
ADD_SUBDIRECTORY(examples/cpp/yaml)
ADD_SUBDIRECTORY(examples/cpp)

