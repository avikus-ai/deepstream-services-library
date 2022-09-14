
### Examge All Source Files ###
FILE(GLOB SOURCES_FILES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/runNAS*.cpp
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

INSTALL(FILES run.yml option.ini
	DESTINATION ${CMAKE_INSTALL_PREFIX}
)

