cmake_minimum_required(VERSION 3.16)
project(RASimBackend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

# ========== Qt5 ==========
set(CMAKE_PREFIX_PATH "C:/Qt/Qt5.14.0/5.14.0/msvc2017_64")
find_package(Qt5 REQUIRED COMPONENTS Core Network)

# ========== AFSIM SDK ==========
set(AFSIM_SDK "${CMAKE_SOURCE_DIR}/bin/afsimsdk")

# 从原项目 RASimEngine.vcxproj 提取的 include 目录列表
set(AFSIM_INCLUDE_DIRS
    ${AFSIM_SDK}/include/mission/source
    ${AFSIM_SDK}/include/core/wsf/source
    ${AFSIM_SDK}/include/core/wsf/source/comm
    ${AFSIM_SDK}/include/core/wsf/source/event_pipe
    ${AFSIM_SDK}/include/core/wsf/source/mover
    ${AFSIM_SDK}/include/core/wsf/source/observer
    ${AFSIM_SDK}/include/core/wsf/source/processor
    ${AFSIM_SDK}/include/core/wsf/source/sensor
    ${AFSIM_SDK}/include/tools/util/source
    ${AFSIM_SDK}/include/tools/util_script/source
    ${AFSIM_SDK}/include/tools/geodata/source
    ${AFSIM_SDK}/include/tools/genio/source
    ${AFSIM_SDK}/include/tools/packetio/source
    ${AFSIM_SDK}/include/tools/tracking_filters/source
    ${AFSIM_SDK}/include/tools/dis/source
    ${AFSIM_SDK}/include/tools/profiling/source
    ${AFSIM_SDK}/include/tools/profiling/source/../common
    ${AFSIM_SDK}/include/core/wsf_util/source
    ${AFSIM_SDK}/include/core/sensor_plot_lib/source
    ${AFSIM_SDK}/include/core/wsf_mil/source
    ${AFSIM_SDK}/include/core/wsf_mil/source/comm
    ${AFSIM_SDK}/include/core/wsf_mil/source/ew
    ${AFSIM_SDK}/include/core/wsf_mil/source/mover
    ${AFSIM_SDK}/include/core/wsf_mil/source/processor
    ${AFSIM_SDK}/include/core/wsf_mil/source/observer
    ${AFSIM_SDK}/include/core/wsf_mil/source/script
    ${AFSIM_SDK}/include/core/wsf_mil/source/sensor
    ${AFSIM_SDK}/include/core/wsf_mil/source/weapon
    ${AFSIM_SDK}/include/core/wsf_cyber/source
    ${AFSIM_SDK}/include/core/wsf_grammar_check/source
    ${AFSIM_SDK}/include/core/wsf_parser/source
    ${AFSIM_SDK}/include/core/wsf_parser/source/grammar_parse
    ${AFSIM_SDK}/include/core/wsf_l16/source
    ${AFSIM_SDK}/include/core/wsf_nx/source
    ${AFSIM_SDK}/include/core/wsf_nx/source/ew
    ${AFSIM_SDK}/include/core/wsf_nx/source/sensor
    ${AFSIM_SDK}/include/core/wsf_nx/source/processor
    ${AFSIM_SDK}/include/core/wsf_mil_parser/source
    ${AFSIM_SDK}/include/core/wsf_mtt/source
    ${AFSIM_SDK}/include/core/wsf_ripr/source
    ${AFSIM_SDK}/include/core/wsf_space/source
    ${AFSIM_SDK}/include/core/wsf_weapon_server/source
    ${AFSIM_SDK}/include/wsf_plugins/wsf_simdis/source
    ${AFSIM_SDK}/include/export/win64
    ${AFSIM_SDK}/include/build/include
    ${AFSIM_SDK}/include/build/wsf/source/include
    ${AFSIM_SDK}/include/build/dis
    ${AFSIM_SDK}/include/build/genio
    ${AFSIM_SDK}/include/build/packetio
    ${AFSIM_SDK}/include/build/wsf/source
    ${AFSIM_SDK}/include/build/util/source
    ${AFSIM_SDK}/include/build/util_script/source
    ${AFSIM_SDK}/include/build/geodata/source
    ${AFSIM_SDK}/include/build/profiling/source
    # build 子目录中的额外头文件
    ${AFSIM_SDK}/include/build/Release/swdev/src/core/wsf_mil/source
)

# ========== 源文件 ==========
set(SOURCES
    src/main.cpp
    # 网络层
    src/CommunicationLayer.cpp
    src/ClientSession.cpp
    # 仿真引擎层
    src/RASimApp.cpp
    src/RASimApplication.cpp
    src/RASimFrameStepSimulation.cpp
    src/RASimServer.cpp
    # 控制层
    src/RASimServerManager.cpp
    src/SimulationController.cpp
    src/ScenarioConverter.cpp
)

set(HEADERS
    src/CommunicationLayer.h
    src/ClientSession.h
    src/communicationlayer_global.h
    src/RASimApp.h
    src/RASimApplication.h
    src/RASimFrameStepSimulation.h
    src/RASimServer.h
    src/RASimServer_global.h
    src/RASimServerManager.h
    src/SimulationController.h
    src/ScenarioConverter.h
)

add_executable(RASimBackend ${SOURCES} ${HEADERS})

# ========== VS 筛选器组织 ==========
# 源文件按模块分组
source_group("源文件\\入口"        FILES src/main.cpp)
source_group("源文件\\网络层"      FILES src/CommunicationLayer.cpp src/ClientSession.cpp)
source_group("源文件\\仿真引擎"    FILES src/RASimApp.cpp src/RASimApplication.cpp src/RASimFrameStepSimulation.cpp src/RASimServer.cpp)
source_group("源文件\\控制层"      FILES src/RASimServerManager.cpp src/SimulationController.cpp src/ScenarioConverter.cpp)

# 头文件按模块分组
source_group("头文件\\网络层"      FILES src/CommunicationLayer.h src/ClientSession.h src/communicationlayer_global.h)
source_group("头文件\\仿真引擎"    FILES src/RASimApp.h src/RASimApplication.h src/RASimFrameStepSimulation.h src/RASimServer.h src/RASimServer_global.h)
source_group("头文件\\控制层"      FILES src/RASimServerManager.h src/SimulationController.h src/ScenarioConverter.h)

# MOC 自动生成的文件放到单独的筛选器，避免污染主视图
source_group("生成文件" REGULAR_EXPRESSION ".*autogen.*")

target_include_directories(RASimBackend PRIVATE
    ${AFSIM_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
)

target_compile_options(RASimBackend PRIVATE /utf-8)

target_compile_definitions(RASimBackend PRIVATE
    WIN32
    _WINDOWS
    NDEBUG
    _CRT_SECURE_NO_WARNINGS
    _SCL_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_WARNINGS
    _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING
    NOMINMAX
    PROMOTE_HARDWARE_EXCEPTIONS
    SWDEV_ALL_USE_DLL
    QT_COMPILE
    BUILD_STATIC
)

target_link_directories(RASimBackend PRIVATE
    ${AFSIM_SDK}/lib/Release
)

target_link_libraries(RASimBackend PRIVATE
    # AFSIM SDK 核心库 (22 个)
    sensor_plot_lib
    wsf_cyber
    wsf_grammar_check
    wsf_mil_parser
    wsf_mtt
    wsf_parser
    wsf_ripr
    wsf_space
    wsf_weapon_server
    wsf_simdis
    wsf_l16
    wsf_nx
    wsf_mil
    wsf
    geodata
    packetio
    dis
    util_script
    tracking_filters
    genio
    profiling
    wsf_util
    util
    # Qt
    Qt5::Core
    Qt5::Network
)

# 输出到 bin/release
set_target_properties(RASimBackend PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin/release"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin/debug"
)
