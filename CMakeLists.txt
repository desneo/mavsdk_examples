cmake_minimum_required(VERSION 3.16)
project(mavsdk_test)

set(CMAKE_CXX_STANDARD 17)

# 引入boost库
# include_directories(E:/practise/3part/boost_1_83_0)
set(3RDPART_LAB /home/zsh/svn_0624/main/3rdpart_lab)
set(MAVSDK_DIR /home/zsh/svn_0624/main/3rdpart_lab/mavsdk/v2.12.9/linux_x86)

include_directories(${MAVSDK_DIR}/include)
include_directories(${MAVSDK_DIR}/include/mavsdk)

# link_directories(${MAVSDK_DIR}/lib)
link_directories(${CMAKE_CURRENT_SOURCE_DIR})
link_libraries(mavsdk)

# 更新参数的程序
add_executable(updateparam updateparam.cpp)
add_executable(updategeofence updategeofence.cpp)
add_executable(downloadfclog downloadfclog.cpp)

# 设置 log库的路径
include_directories(${3RDPART_LAB}/plog/plog-1.1.10/include)
add_definitions(-DPLOG_CHAR_IS_UTF8=1)

# json库
include_directories(${3RDPART_LAB}/nlohmann/nlohmann_v3.11.2/)

# 检查当前平台并根据条件选择性地添加额外的源文件
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32.lib)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32 mswsock)

    # 导出符号信息
    set_target_properties(${PROJECT_NAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS TRUE)
elseif(UNIX)
    link_libraries(pthread)
    link_libraries(pthread)
endif()