cmake_minimum_required(VERSION 3.16)
project(c0ntrol VERSION 1.0 LANGUAGES CXX)

option(BUILD_APP "Build c0ntrol desktop application" ON)
include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(BUILD_APP)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)
    find_package(OpenCV REQUIRED COMPONENTS core imgproc videoio video)

    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
    file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "src/*.h")

    add_executable(c0ntrol ${SOURCES} ${HEADERS})
    target_include_directories(c0ntrol PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(c0ntrol PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        ${OpenCV_LIBS}
    )
endif()

if(BUILD_TESTING)
    function(add_core_test target source)
        add_executable(${target} ${source} ${ARGN})
        target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR})
        add_test(NAME ${target} COMMAND ${target})
    endfunction()

    add_core_test(test_display_transform tests/test_display_transform.cpp)
    add_core_test(test_dynamic_gestures tests/test_dynamic_gestures.cpp)
    add_core_test(test_frame_sync tests/test_frame_sync.cpp)
    add_core_test(test_hand_geometry
        tests/test_hand_geometry.cpp
        src/core/gestures/GestureClassifier.cpp
    )
    add_core_test(test_one_euro tests/test_one_euro.cpp)
endif()
