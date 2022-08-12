
### Examge All Source Files ###
FILE(GLOB SOURCES_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

LIST(REMOVE_ITEM SOURCES_FILES "${CMAKE_CURRENT_SOURCE_DIR}/1file_yolov5_preproc_pgie_iou_osd_window_nmm_xtensor.cpp")

LIST(REMOVE_ITEM SOURCES_FILES "${CMAKE_CURRENT_SOURCE_DIR}/2file_yolov5_preproc_nmm_compare.cpp")

### MAKE TARGET FILE ###
#ADD_EXECUTABLE(multiple_pipelines.out multiple_pipelines.cpp)

### Find Lib ###
FIND_LIBRARY( NVDS_META
	NAMES nvds_meta
    PATHS ${NVDS_LIB}
)

### LINK LIBRARY ###
#TARGET_LINK_LIBRARIES(multiple_pipelines.out
#	dsl
#) 

### ALL FILES BUILD ###
foreach(loopVar ${SOURCES_FILES})
    STRING(REPLACE .cpp .out output ${loopVar})
    STRING(REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/ "" bin ${output})
    ADD_EXECUTABLE(${bin} ${loopVar})
   
    TARGET_LINK_LIBRARIES(${bin} 
        dsl
        glib-2.0
		yaml
        ${NVDS_META}
    )
	### INSTALL TARGETS ###
	INSTALL(TARGETS ${bin}
		RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
	)
endforeach()

INSTALL(FILES run.yml
	DESTINATION ${CMAKE_INSTALL_PREFIX}
)

