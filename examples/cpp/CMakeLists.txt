### Examge All Source Files ###
FILE(GLOB SOURCES_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/nas*.cpp
)

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
    ADD_EXECUTABLE(${bin} ${loopVar} cmd.cpp)
   
    TARGET_LINK_LIBRARIES(${bin}
        ami
        dsl
        glib-2.0
		yaml
        pthread
        rt
        ${NVDS_META}
    )
	### INSTALL TARGETS ###
	INSTALL(TARGETS ${bin}
		RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
	)
endforeach()

#ADD_EXECUTABLE(nas ${SOURCES_FILES} cmd.cpp)
#
#TARGET_LINK_LIBRARIES(nas
#     ami
#     dsl
#     glib-2.0
#     pthread
#     rt
#     ${NVDS_META}
#)
#
##SET_TARGET_PROPERTIES (nas
##	PROPERTIES OUTPUT_NAME nassssssssssssssss
##)
#
#### INSTALL TARGETS ###
#INSTALL(TARGETS nas
#	RUNTIME DESTINATION ${CMAKE_INSTALL_PREFIX}
#)

INSTALL(FILES option.ini run.yml labels.txt
	DESTINATION ${CMAKE_INSTALL_PREFIX}
)

