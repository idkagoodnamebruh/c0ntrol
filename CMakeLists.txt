cmake_minimum_required(VERSION 3.16)
project(c0ntrol VERSION 1.0 LANGUAGES CXX)

option(BUILD_APP "Build c0ntrol desktop application" ON)
option(ENABLE_MEDIAPIPE "Build the MediaPipe Hand Landmarker backend" OFF)
option(ENABLE_NATIVE_INPUT_SMOKE
       "Build the manual opt-in Windows native input smoke tool" OFF)
set(MEDIAPIPE_BRIDGE_LIBRARY "" CACHE FILEPATH "Pinned c0ntrol MediaPipe bridge shared library")
include(CTest)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(ENABLE_MEDIAPIPE)
    if(NOT MEDIAPIPE_BRIDGE_LIBRARY)
        message(FATAL_ERROR "ENABLE_MEDIAPIPE requires MEDIAPIPE_BRIDGE_LIBRARY built from the pinned bridge")
    endif()
    add_library(MediaPipe::C0ntrolBridge SHARED IMPORTED)
    set_target_properties(MediaPipe::C0ntrolBridge PROPERTIES
        IMPORTED_LOCATION "${MEDIAPIPE_BRIDGE_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}"
    )
endif()

if(BUILD_APP)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)
    find_package(OpenCV REQUIRED COMPONENTS core imgproc videoio video)

    file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "src/*.cpp")
    file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS "src/*.h")

    if(NOT ENABLE_MEDIAPIPE)
        list(FILTER SOURCES EXCLUDE REGEX "MediaPipeHandTrackingBackend\\.cpp$")
    endif()
    if(NOT WIN32)
        list(FILTER SOURCES EXCLUDE REGEX
             "WindowsSystemInputBackend\\.cpp$")
    endif()

    add_executable(c0ntrol ${SOURCES} ${HEADERS})
    target_include_directories(c0ntrol PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(c0ntrol PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        ${OpenCV_LIBS}
    )
    if(ENABLE_MEDIAPIPE)
        target_compile_definitions(c0ntrol PRIVATE C0NTROL_ENABLE_MEDIAPIPE=1)
        target_link_libraries(c0ntrol PRIVATE MediaPipe::C0ntrolBridge)
    endif()
    if(WIN32)
        target_link_libraries(c0ntrol PRIVATE user32)
    endif()
endif()

if(WIN32 AND ENABLE_NATIVE_INPUT_SMOKE)
    add_executable(c0ntrol_native_input_smoke
        tools/native_input_smoke.cpp
        src/platform/windows/WindowsSystemInputBackend.cpp
        src/platform/windows/WindowsPointerMath.cpp
    )
    target_include_directories(c0ntrol_native_input_smoke
        PRIVATE ${CMAKE_SOURCE_DIR}
    )
    target_link_libraries(c0ntrol_native_input_smoke PRIVATE user32)
endif()

if(BUILD_TESTING)
    find_package(Threads REQUIRED)

    function(add_core_test target source)
        add_executable(${target} ${source} ${ARGN})
        target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR})
        add_test(NAME ${target} COMMAND ${target})
    endfunction()

    add_core_test(test_display_transform
        tests/test_display_transform.cpp
        src/core/actions/PointerMapper.cpp
    )
    add_core_test(test_dynamic_gestures tests/test_dynamic_gestures.cpp)
    add_core_test(test_frame_sync tests/test_frame_sync.cpp)
    add_core_test(test_hand_geometry
        tests/test_hand_geometry.cpp
        src/core/gestures/HandFeatureExtractor.cpp
        src/core/gestures/GestureEngine.cpp
    )
    add_core_test(test_hand_features
        tests/test_hand_features.cpp
        src/core/gestures/HandFeatureExtractor.cpp
        src/core/gestures/GestureEngine.cpp
    )
    add_core_test(test_gesture_state_machine
        tests/test_gesture_state_machine.cpp
        src/core/gestures/GestureStateMachine.cpp
    )
    add_core_test(test_gesture_pipeline
        tests/test_gesture_pipeline.cpp
        src/core/gestures/HandFeatureExtractor.cpp
        src/core/gestures/GestureEngine.cpp
        src/core/gestures/GestureStateMachine.cpp
        src/core/gestures/GesturePipeline.cpp
    )
    add_core_test(test_pointer_mapper
        tests/test_pointer_mapper.cpp
        src/core/actions/PointerMapper.cpp
    )
    add_core_test(test_action_dispatcher
        tests/test_action_dispatcher.cpp
        src/core/actions/ActionDispatcher.cpp
        src/core/actions/PointerMapper.cpp
        src/core/config/RuntimeConfig.cpp
        src/core/config/RuntimeConfigController.cpp
        src/core/filters/LandmarkFilterBank.cpp
    )
    add_core_test(test_runtime_config
        tests/test_runtime_config.cpp
        src/core/config/RuntimeConfig.cpp
        src/core/actions/PointerMapper.cpp
        src/core/filters/LandmarkFilterBank.cpp
    )
    add_core_test(test_settings_serialization
        tests/test_settings_serialization.cpp
        src/core/config/SettingsCodec.cpp
        src/core/config/RuntimeConfig.cpp
        src/core/actions/PointerMapper.cpp
        src/core/filters/LandmarkFilterBank.cpp
    )
    add_core_test(test_pointer_calibration
        tests/test_pointer_calibration.cpp
        src/core/calibration/PointerCalibration.cpp
        src/core/actions/PointerMapper.cpp
    )
    add_core_test(test_latest_frame_slot tests/test_latest_frame_slot.cpp)
    target_link_libraries(test_latest_frame_slot PRIVATE Threads::Threads)
    add_core_test(test_async_capture tests/test_async_capture.cpp)
    target_link_libraries(test_async_capture PRIVATE Threads::Threads)
    add_core_test(test_pipeline_metrics tests/test_pipeline_metrics.cpp)
    add_core_test(test_windows_pointer_math
        tests/test_windows_pointer_math.cpp
        src/platform/windows/WindowsPointerMath.cpp
    )
    if(WIN32)
        add_executable(test_windows_input_backend_compile
            tests/test_windows_input_backend_compile.cpp
            src/platform/windows/WindowsSystemInputBackend.cpp
            src/platform/windows/WindowsPointerMath.cpp
        )
        target_include_directories(test_windows_input_backend_compile
            PRIVATE ${CMAKE_SOURCE_DIR}
        )
        target_link_libraries(test_windows_input_backend_compile PRIVATE user32)
        add_test(
            NAME test_windows_input_backend_compile
            COMMAND test_windows_input_backend_compile
        )
    endif()
    add_core_test(test_one_euro tests/test_one_euro.cpp)
    add_core_test(test_landmark_filter_bank
        tests/test_landmark_filter_bank.cpp
        src/core/filters/LandmarkFilterBank.cpp
    )
    add_core_test(test_hand_tracking tests/test_hand_tracking.cpp)
    if(ENABLE_MEDIAPIPE)
        add_executable(test_mediapipe_hand_tracking_backend
            tests/test_mediapipe_hand_tracking_backend.cpp
            src/core/tracking/MediaPipeHandTrackingBackend.cpp
        )
        target_include_directories(test_mediapipe_hand_tracking_backend PRIVATE ${CMAKE_SOURCE_DIR})
        target_link_libraries(test_mediapipe_hand_tracking_backend PRIVATE MediaPipe::C0ntrolBridge)
        target_compile_definitions(test_mediapipe_hand_tracking_backend PRIVATE
            C0NTROL_MODEL_PATH="${CMAKE_SOURCE_DIR}/models/hand_landmarker.task")
        add_test(NAME test_mediapipe_hand_tracking_backend
                 COMMAND test_mediapipe_hand_tracking_backend)
        add_executable(test_mediapipe_real_hand
            tests/test_mediapipe_real_hand.cpp
            src/core/tracking/MediaPipeHandTrackingBackend.cpp
        )
        target_include_directories(test_mediapipe_real_hand PRIVATE ${CMAKE_SOURCE_DIR})
        target_link_libraries(test_mediapipe_real_hand PRIVATE MediaPipe::C0ntrolBridge)
        target_compile_definitions(test_mediapipe_real_hand PRIVATE
            C0NTROL_MODEL_PATH="${CMAKE_SOURCE_DIR}/models/hand_landmarker.task"
            C0NTROL_HAND_ASSET_PATH="${CMAKE_SOURCE_DIR}/tests/assets/hand_test.ppm")
        add_test(NAME test_mediapipe_real_hand COMMAND test_mediapipe_real_hand)
    endif()
endif()
