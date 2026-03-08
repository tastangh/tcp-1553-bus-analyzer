cmake_minimum_required(VERSION 3.21)
project(tcp_1553_bus_analyzer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pin system library paths into the binary's RPATH so the dynamic linker
# always uses the correct system glibc/libpthread, not snap's shadowing version.
# NOTE: We explicitly use RPATH (not RUNPATH) via --disable-new-dtags because
# RPATH has higher precedence than LD_LIBRARY_PATH, whereas RUNPATH does not.
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "/lib/x86_64-linux-gnu:/usr/lib/x86_64-linux-gnu")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--disable-new-dtags")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/)

find_package(Boost 1.74.0 COMPONENTS filesystem REQUIRED)
find_package(spdlog REQUIRED)

add_definitions(-DBOOST_BIND_GLOBAL_PLACEHOLDERS)

# Find wxWidgets
find_package(wxWidgets COMPONENTS core base net REQUIRED)
include(${wxWidgets_USE_FILE})

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(tcp_1553_bus_analyzer ${SOURCES})

target_compile_definitions(tcp_1553_bus_analyzer PUBLIC
    _FILE_OFFSET_BITS=64 WXUSINGDLL __WXGTK__ _AIM_LINUX
)

target_include_directories(tcp_1553_bus_analyzer PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

target_link_libraries(tcp_1553_bus_analyzer PRIVATE 
    Boost::filesystem
    spdlog::spdlog    
    ${wxWidgets_LIBRARIES}
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/libpcap.so"
    pthread
)
