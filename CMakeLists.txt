cmake_minimum_required(VERSION 3.16)
project(c0ntrol VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS core imgproc videoio video)

file(GLOB_RECURSE SOURCES "src/*.cpp")
file(GLOB_RECURSE HEADERS "src/*.h")

add_executable(c0ntrol ${SOURCES} ${HEADERS})

target_include_directories(c0ntrol PRIVATE ${CMAKE_SOURCE_DIR})

target_link_libraries(c0ntrol PRIVATE 
    Qt6::Core 
    Qt6::Gui 
    Qt6::Widgets 
    ${OpenCV_LIBS}
)
