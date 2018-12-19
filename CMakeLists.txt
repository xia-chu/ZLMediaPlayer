project(MediaPlayer)
cmake_minimum_required(VERSION 2.8)
#SET(CMAKE_FIND_LIBRARY_SUFFIXES ".a")

#使能c++11
#add_compile_options(-std=c++11)
set(CMAKE_CXX_STANDARD 11)

#加载自定义模块
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")
#设置库文件路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
#设置可执行程序路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

#设置工程源码根目录
set(ToolKit_Root ${CMAKE_SOURCE_DIR}/../ZLMediaKit/ZLToolKit/src)
set(MediaKit_Root ${CMAKE_SOURCE_DIR}/../ZLMediaKit/src)
set(MediaPlayer_Root ${CMAKE_SOURCE_DIR}/src)

#设置头文件目录
INCLUDE_DIRECTORIES(${ToolKit_Root})
INCLUDE_DIRECTORIES(${MediaKit_Root})
INCLUDE_DIRECTORIES(${MediaPlayer_Root})
INCLUDE_DIRECTORIES(${MediaPlayer_Root}/libFaad)

add_definitions(-DHAVE_CONFIG_H)

#收集源代码
file(GLOB ToolKit_src_list ${ToolKit_Root}/*/*.cpp ${ToolKit_Root}/*/*.h ${ToolKit_Root}/*/*.c)
file(GLOB MediaKit_src_list ${MediaKit_Root}/*/*.cpp ${MediaKit_Root}/*/*.h ${MediaKit_Root}/*/*.c)
file(GLOB MediaPlayer_src_list ${MediaPlayer_Root}/*/*.cpp ${MediaPlayer_Root}/*/*.h ${MediaPlayer_Root}/*/*.c)

#去除win32的适配代码
if (NOT WIN32)
    list(REMOVE_ITEM ToolKit_src_list ${ToolKit_Root}/win32/getopt.c)
endif ()

set(LINK_LIB_LIST)

if (NOT WIN32)
    list(APPEND LINK_LIB_LIST pthread dl)
else ()
    list(APPEND LINK_LIB_LIST WS2_32 Iphlpapi shlwapi)
endif ()

#添加库
add_library(zltoolkit STATIC ${ToolKit_src_list})
add_library(zlmediakit STATIC ${MediaKit_src_list})
add_library(mediaplayer STATIC ${MediaPlayer_src_list})


target_link_libraries(zltoolkit ${LINK_LIB_LIST})
target_link_libraries(zlmediakit zltoolkit ${LINK_LIB_LIST})
target_link_libraries(mediaplayer zlmediakit zltoolkit ${LINK_LIB_LIST})

#查找ffmpeg/libavcodec是否安装
find_package(AVCODEC QUIET)
if (AVCODEC_FOUND)
    include_directories(${AVCODEC_INCLUDE_DIR})
    list(APPEND LINK_LIB_LIST ${AVCODEC_LIBRARIES})
    message(STATUS "找到libavcodec库:\"${AVCODEC_INCLUDE_DIR} ${AVCODEC_LIBRARIES}\"")
endif (AVCODEC_FOUND)

#查找ffmpeg/libavcodec是否安装
find_package(AVUTIL QUIET)
if (AVUTIL_FOUND)
    include_directories(${AVUTIL_INCLUDE_DIR})
    list(APPEND LINK_LIB_LIST ${AVUTIL_LIBRARIES})
    message(STATUS "找到libavutil库:\"${AVUTIL_INCLUDE_DIR} ${AVUTIL_LIBRARIES}\"")
endif (AVUTIL_FOUND)

#查找SDL2是否安装
find_package(SDL2 QUIET)
if (SDL2_FOUND)
    include_directories(${SDL2_INCLUDE_DIR})
    list(APPEND LINK_LIB_LIST ${SDL2_LIBRARY})
    message(STATUS "找到SDL2:${SDL2_INCLUDE_DIR},${SDL2_LIBRARY}")
endif (SDL2_FOUND)


add_executable(test_player src/test_player.cpp)

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    list(APPEND LINK_LIB_LIST swresample z)
endif()

target_link_libraries(test_player mediaplayer zlmediakit zltoolkit ${LINK_LIB_LIST})























