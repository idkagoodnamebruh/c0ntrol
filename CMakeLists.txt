cmake_minimum_required(VERSION 3.16)
project(c0ntrol VERSION 1.0 LANGUAGES CXX)

option(BUILD_APP "Build c0ntrol desktop application" ON)
option(ENABLE_MEDIAPIPE "Build the MediaPipe Hand Landmarker backend" OFF)
option(C0NTROL_PRODUCTION_BUILD
       "Require the complete real production runtime and enable packaging" OFF)
option(ENABLE_NATIVE_INPUT_SMOKE
       "Build the manual opt-in Windows native input smoke tool" OFF)
option(ENABLE_LINUX_EIS_INPUT
       "Enable Linux Wayland input through libei/EIS" ON)
option(REQUIRE_LINUX_EIS_INPUT
       "Fail configuration when the Linux libei/EIS backend is unavailable" OFF)
set(MEDIAPIPE_BRIDGE_LIBRARY "" CACHE FILEPATH "Pinned c0ntrol MediaPipe bridge shared library")
set(MEDIAPIPE_BRIDGE_RUNTIME_LIBRARY "" CACHE FILEPATH
    "Windows runtime DLL for the pinned c0ntrol MediaPipe bridge")
set(C0NTROL_OPENCV_RUNTIME_DIR "" CACHE PATH
    "Windows directory containing the OpenCV runtime DLLs")
set(C0NTROL_WINDEPLOYQT_EXECUTABLE "" CACHE FILEPATH
    "Path to the official Qt windeployqt tool")
set(C0NTROL_GIT_COMMIT "" CACHE STRING
    "Git commit embedded in deterministic release metadata")
include(CTest)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Threads REQUIRED)

set(C0NTROL_MODEL_FILE
    "${CMAKE_SOURCE_DIR}/models/hand_landmarker.task")
set(C0NTROL_MODEL_SHA256
    "fbc2a30080c3c557093b5ddfc334698132eb341044ccee322ccf8bcf3607cde1")
set(C0NTROL_MEDIAPIPE_VERSION "v0.10.26")
set(C0NTROL_MEDIAPIPE_COMMIT
    "80ae8afbd03465b0d6d9f9e874f8cacf093d23e9")

if(C0NTROL_PRODUCTION_BUILD)
    if(NOT BUILD_APP)
        message(FATAL_ERROR "Production build requires BUILD_APP=ON")
    endif()
    if(NOT ENABLE_MEDIAPIPE)
        message(FATAL_ERROR
            "Production build requires ENABLE_MEDIAPIPE=ON; mock tracking cannot be packaged as production")
    endif()
    if(UNIX AND NOT APPLE AND
       (NOT ENABLE_LINUX_EIS_INPUT OR NOT REQUIRE_LINUX_EIS_INPUT))
        message(FATAL_ERROR
            "Production Linux requires ENABLE_LINUX_EIS_INPUT=ON and REQUIRE_LINUX_EIS_INPUT=ON")
    endif()
    if(NOT EXISTS "${C0NTROL_MODEL_FILE}")
        message(FATAL_ERROR "Production model is missing: ${C0NTROL_MODEL_FILE}")
    endif()
    file(SHA256 "${C0NTROL_MODEL_FILE}" actual_model_sha256)
    if(NOT actual_model_sha256 STREQUAL C0NTROL_MODEL_SHA256)
        message(FATAL_ERROR
            "Production model SHA-256 mismatch: ${actual_model_sha256}")
    endif()
endif()

if(NOT C0NTROL_GIT_COMMIT)
    execute_process(
        COMMAND git rev-parse HEAD
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE git_result
        OUTPUT_VARIABLE detected_git_commit
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(git_result EQUAL 0)
        set(C0NTROL_GIT_COMMIT "${detected_git_commit}")
    else()
        set(C0NTROL_GIT_COMMIT "unknown")
    endif()
endif()

set(C0NTROL_HAS_LIBEI OFF)
if(UNIX AND NOT APPLE)
    if(REQUIRE_LINUX_EIS_INPUT AND NOT ENABLE_LINUX_EIS_INPUT)
        message(FATAL_ERROR
            "REQUIRE_LINUX_EIS_INPUT requires ENABLE_LINUX_EIS_INPUT=ON")
    endif()
    if(ENABLE_LINUX_EIS_INPUT)
        find_package(PkgConfig QUIET)
        if(PkgConfig_FOUND)
            pkg_check_modules(LIBEI QUIET IMPORTED_TARGET "libei-1.0>=1.2.1")
            pkg_check_modules(LIBOEFFIS QUIET IMPORTED_TARGET "liboeffis-1.0>=1.2.1")
        endif()
        if(LIBEI_FOUND AND LIBOEFFIS_FOUND)
            set(C0NTROL_HAS_LIBEI ON)
            message(STATUS
                "Linux EIS input enabled: libei ${LIBEI_VERSION}, liboeffis ${LIBOEFFIS_VERSION}")
        elseif(REQUIRE_LINUX_EIS_INPUT)
            message(FATAL_ERROR
                "Linux EIS input requires pkg-config modules libei-1.0>=1.2.1 and liboeffis-1.0>=1.2.1")
        else()
            message(WARNING
                "libei/liboeffis development files unavailable; using NullSystemInputBackend")
        endif()
    endif()
endif()

if(ENABLE_MEDIAPIPE)
    if(NOT MEDIAPIPE_BRIDGE_LIBRARY)
        message(FATAL_ERROR "ENABLE_MEDIAPIPE requires MEDIAPIPE_BRIDGE_LIBRARY built from the pinned bridge")
    endif()
    add_library(MediaPipe::C0ntrolBridge SHARED IMPORTED)
    if(WIN32)
        if(NOT MEDIAPIPE_BRIDGE_RUNTIME_LIBRARY)
            message(FATAL_ERROR
                "Windows MediaPipe requires MEDIAPIPE_BRIDGE_RUNTIME_LIBRARY (the bridge DLL)")
        endif()
        set_target_properties(MediaPipe::C0ntrolBridge PROPERTIES
            IMPORTED_IMPLIB "${MEDIAPIPE_BRIDGE_LIBRARY}"
            IMPORTED_LOCATION "${MEDIAPIPE_BRIDGE_RUNTIME_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}"
        )
    else()
        set_target_properties(MediaPipe::C0ntrolBridge PROPERTIES
            IMPORTED_LOCATION "${MEDIAPIPE_BRIDGE_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}"
        )
    endif()
endif()

if(BUILD_APP)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)

    find_package(Qt6 COMPONENTS Core Gui Widgets REQUIRED)
    find_package(OpenCV REQUIRED COMPONENTS core imgproc videoio video)

    set(C0NTROL_APP_SOURCES
        src/main.cpp
        src/core/actions/ActionDispatcher.cpp
        src/core/actions/PointerMapper.cpp
        src/core/calibration/PointerCalibration.cpp
        src/core/config/RuntimeConfig.cpp
        src/core/config/RuntimeConfigController.cpp
        src/core/config/SettingsCodec.cpp
        src/core/filters/LandmarkFilterBank.cpp
        src/core/gestures/DynamicGestureRecognizer.cpp
        src/core/gestures/GestureEngine.cpp
        src/core/gestures/GesturePipeline.cpp
        src/core/gestures/GestureStateMachine.cpp
        src/core/gestures/HandFeatureExtractor.cpp
        src/core/input/NativeInputRuntime.cpp
        src/core/runtime/BuildMetadata.cpp
        src/core/runtime/RuntimePaths.cpp
        src/core/vision/VisionWorker.cpp
        src/gui/DeveloperModeWidget.cpp
        src/gui/MainWindow.cpp
        src/gui/PointerCalibrationDialog.cpp
        src/gui/SettingsDialog.cpp
        src/platform/NullSystemInputBackend.cpp
        src/platform/SystemInputBackendFactory.cpp
        src/platform/opencv/OpenCVCameraSource.cpp
        src/platform/qt/QtSettingsStore.cpp
        src/core/vision/VisionWorker.h
        src/gui/DeveloperModeWidget.h
        src/gui/MainWindow.h
        src/gui/PointerCalibrationDialog.h
        src/gui/SettingsDialog.h
    )
    if(ENABLE_MEDIAPIPE)
        list(APPEND C0NTROL_APP_SOURCES
             src/core/tracking/MediaPipeHandTrackingBackend.cpp)
    endif()
    if(WIN32)
        list(APPEND C0NTROL_APP_SOURCES
            src/platform/windows/WindowsPointerMath.cpp
            src/platform/windows/WindowsSystemInputBackend.cpp
        )
    endif()
    if(C0NTROL_HAS_LIBEI)
        list(APPEND C0NTROL_APP_SOURCES
            src/platform/linux/EisRegion.cpp
            src/platform/linux/LibeiPortalSession.cpp
            src/platform/linux/LinuxEisSystemInputBackend.cpp
        )
    endif()

    add_executable(c0ntrol ${C0NTROL_APP_SOURCES})
    target_include_directories(c0ntrol PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(c0ntrol PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Threads::Threads
        ${OpenCV_LIBS}
    )
    if(ENABLE_MEDIAPIPE)
        target_compile_definitions(c0ntrol PRIVATE C0NTROL_ENABLE_MEDIAPIPE=1)
        target_link_libraries(c0ntrol PRIVATE MediaPipe::C0ntrolBridge)
    endif()

    if(C0NTROL_PRODUCTION_BUILD)
        set(c0ntrol_build_profile "production")
    else()
        set(c0ntrol_build_profile "development")
    endif()
    if(ENABLE_MEDIAPIPE)
        set(c0ntrol_mediapipe_status
            "enabled (${C0NTROL_MEDIAPIPE_VERSION}, ${C0NTROL_MEDIAPIPE_COMMIT})")
    else()
        set(c0ntrol_mediapipe_status "disabled (mock tracker)")
    endif()
    if(WIN32)
        set(c0ntrol_native_input_capability "Windows SendInput")
        set(c0ntrol_model_relative_dir "models")
    elseif(C0NTROL_HAS_LIBEI)
        set(c0ntrol_native_input_capability
            "Linux EIS/libei ${LIBEI_VERSION}, liboeffis ${LIBOEFFIS_VERSION}")
        set(c0ntrol_model_relative_dir "../share/c0ntrol/models")
    else()
        set(c0ntrol_native_input_capability "unavailable (Null backend)")
        set(c0ntrol_model_relative_dir "../share/c0ntrol/models")
    endif()
    target_compile_definitions(c0ntrol PRIVATE
        C0NTROL_VERSION="${PROJECT_VERSION}"
        C0NTROL_GIT_COMMIT="${C0NTROL_GIT_COMMIT}"
        C0NTROL_BUILD_TYPE="$<CONFIG>"
        C0NTROL_BUILD_PROFILE="${c0ntrol_build_profile}"
        C0NTROL_MEDIAPIPE_STATUS="${c0ntrol_mediapipe_status}"
        C0NTROL_NATIVE_INPUT_CAPABILITY="${c0ntrol_native_input_capability}"
        C0NTROL_MODEL_RELATIVE_DIR="${c0ntrol_model_relative_dir}"
    )
    if(WIN32)
        target_link_libraries(c0ntrol PRIVATE user32)
    endif()
    if(C0NTROL_HAS_LIBEI)
        target_compile_definitions(c0ntrol PRIVATE C0NTROL_HAS_LIBEI=1)
        target_link_libraries(c0ntrol PRIVATE
            PkgConfig::LIBEI
            PkgConfig::LIBOEFFIS
        )
    endif()

    if(WIN32)
        set(c0ntrol_app_install_dir ".")
        set(c0ntrol_model_install_dir "models")
        set(c0ntrol_doc_install_dir "docs")
    else()
        set(c0ntrol_app_install_dir "${CMAKE_INSTALL_BINDIR}")
        set(c0ntrol_model_install_dir
            "${CMAKE_INSTALL_DATADIR}/c0ntrol/models")
        set(c0ntrol_doc_install_dir
            "${CMAKE_INSTALL_DATADIR}/doc/c0ntrol")
        set_target_properties(c0ntrol PROPERTIES
            INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")
    endif()

    configure_file(
        cmake/RELEASE_MANIFEST.txt.in
        "${CMAKE_CURRENT_BINARY_DIR}/RELEASE_MANIFEST.txt"
        @ONLY
    )
    install(TARGETS c0ntrol RUNTIME DESTINATION "${c0ntrol_app_install_dir}")
    install(FILES "${C0NTROL_MODEL_FILE}"
        DESTINATION "${c0ntrol_model_install_dir}")
    install(FILES README.md THIRD_PARTY_NOTICES.md
        "${CMAKE_CURRENT_BINARY_DIR}/RELEASE_MANIFEST.txt"
        DESTINATION "${c0ntrol_doc_install_dir}")

    if(ENABLE_MEDIAPIPE)
        if(WIN32)
            install(FILES "${MEDIAPIPE_BRIDGE_RUNTIME_LIBRARY}"
                DESTINATION ".")
        else()
            install(FILES "${MEDIAPIPE_BRIDGE_LIBRARY}"
                DESTINATION "${CMAKE_INSTALL_LIBDIR}")
        endif()
    endif()

    if(C0NTROL_PRODUCTION_BUILD AND WIN32)
        if(NOT C0NTROL_WINDEPLOYQT_EXECUTABLE)
            if(TARGET Qt6::qmake)
                get_target_property(c0ntrol_qmake
                    Qt6::qmake IMPORTED_LOCATION)
            endif()
            if(c0ntrol_qmake AND NOT c0ntrol_qmake MATCHES "-NOTFOUND$")
                get_filename_component(c0ntrol_qt_bin_dir
                    "${c0ntrol_qmake}" DIRECTORY)
            endif()
            find_program(c0ntrol_detected_windeployqt
                NAMES windeployqt
                HINTS "${c0ntrol_qt_bin_dir}"
            )
            if(c0ntrol_detected_windeployqt)
                set(C0NTROL_WINDEPLOYQT_EXECUTABLE
                    "${c0ntrol_detected_windeployqt}")
            endif()
        endif()
        if(NOT C0NTROL_WINDEPLOYQT_EXECUTABLE)
            message(FATAL_ERROR
                "Production Windows packaging requires the official windeployqt executable")
        endif()
        if(NOT C0NTROL_OPENCV_RUNTIME_DIR OR
           NOT IS_DIRECTORY "${C0NTROL_OPENCV_RUNTIME_DIR}")
            message(FATAL_ERROR
                "Production Windows packaging requires C0NTROL_OPENCV_RUNTIME_DIR")
        endif()
        install(DIRECTORY "${C0NTROL_OPENCV_RUNTIME_DIR}/"
            DESTINATION "."
            FILES_MATCHING PATTERN "opencv_*.dll")
        configure_file(cmake/DeployQt.cmake.in
            "${CMAKE_CURRENT_BINARY_DIR}/DeployQt.cmake" @ONLY)
        install(SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/DeployQt.cmake")
    endif()

    if(C0NTROL_PRODUCTION_BUILD)
        string(TOLOWER "${CMAKE_SYSTEM_NAME}" c0ntrol_package_platform)
        string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" c0ntrol_package_arch)
        if(c0ntrol_package_arch MATCHES "^(amd64|x64)$")
            set(c0ntrol_package_arch "x86_64")
        endif()
        if(WIN32)
            set(CPACK_GENERATOR "ZIP")
        else()
            set(CPACK_GENERATOR "TGZ")
        endif()
        set(CPACK_PACKAGE_NAME "c0ntrol")
        set(CPACK_PACKAGE_VENDOR "c0ntrol project")
        set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
        set(CPACK_PACKAGE_FILE_NAME
            "c0ntrol-${PROJECT_VERSION}-${c0ntrol_package_platform}-${c0ntrol_package_arch}")
        set(CPACK_PACKAGE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/packages")
        set(CPACK_PACKAGE_CHECKSUM "SHA256")
        set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
        set(CPACK_VERBATIM_VARIABLES YES)
        include(CPack)
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
    function(add_core_test target source)
        add_executable(${target} ${source} ${ARGN})
        target_include_directories(${target} PRIVATE ${CMAKE_SOURCE_DIR})
        add_test(NAME ${target} COMMAND ${target})
    endfunction()

    add_core_test(test_display_transform
        tests/test_display_transform.cpp
        src/core/actions/PointerMapper.cpp
    )
    add_core_test(test_dynamic_gestures
        tests/test_dynamic_gestures.cpp
        src/core/gestures/DynamicGestureRecognizer.cpp
    )
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
        src/core/gestures/DynamicGestureRecognizer.cpp
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
        src/platform/NullSystemInputBackend.cpp
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
    add_core_test(test_native_input_lifecycle
        tests/test_native_input_lifecycle.cpp
        src/core/actions/ActionDispatcher.cpp
        src/core/actions/PointerMapper.cpp
        src/core/input/NativeInputRuntime.cpp
        src/platform/NullSystemInputBackend.cpp
    )
    target_link_libraries(test_native_input_lifecycle PRIVATE Threads::Threads)
    add_core_test(test_async_input_runtime
        tests/test_async_input_runtime.cpp
        src/core/actions/ActionDispatcher.cpp
        src/core/actions/PointerMapper.cpp
        src/core/input/NativeInputRuntime.cpp
    )
    target_link_libraries(test_async_input_runtime PRIVATE Threads::Threads)
    add_core_test(test_windows_pointer_math
        tests/test_windows_pointer_math.cpp
        src/platform/windows/WindowsPointerMath.cpp
    )
    if(UNIX AND NOT APPLE)
        add_test(NAME test_model_tooling
                 COMMAND bash
                         "${CMAKE_SOURCE_DIR}/tests/tooling/test_model_tooling.sh")
        add_core_test(test_eis_regions
            tests/test_eis_regions.cpp
            src/platform/linux/EisRegion.cpp
        )
        add_core_test(test_linux_eis_backend
            tests/test_linux_eis_backend.cpp
            src/platform/linux/EisRegion.cpp
            src/platform/linux/LinuxEisSystemInputBackend.cpp
        )
        if(C0NTROL_HAS_LIBEI)
            add_executable(test_linux_eis_backend_compile
                tests/test_linux_eis_backend_compile.cpp
                src/platform/linux/EisRegion.cpp
                src/platform/linux/LinuxEisSystemInputBackend.cpp
                src/platform/linux/LibeiPortalSession.cpp
                src/platform/SystemInputBackendFactory.cpp
            )
            target_include_directories(test_linux_eis_backend_compile
                PRIVATE ${CMAKE_SOURCE_DIR}
            )
            target_link_libraries(test_linux_eis_backend_compile PRIVATE
                PkgConfig::LIBEI
                PkgConfig::LIBOEFFIS
            )
            target_compile_definitions(test_linux_eis_backend_compile PRIVATE
                C0NTROL_HAS_LIBEI=1
            )
            add_test(NAME test_linux_eis_backend_compile
                     COMMAND test_linux_eis_backend_compile)
        endif()
    endif()
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
    add_core_test(test_runtime_paths
        tests/test_runtime_paths.cpp
        src/core/runtime/RuntimePaths.cpp
    )
    target_compile_definitions(test_runtime_paths PRIVATE
        C0NTROL_MODEL_RELATIVE_DIR="models")
    add_core_test(test_build_metadata
        tests/test_build_metadata.cpp
        src/core/runtime/BuildMetadata.cpp
    )
    target_compile_definitions(test_build_metadata PRIVATE
        C0NTROL_VERSION="1.0"
        C0NTROL_GIT_COMMIT="0123456789abcdef"
        C0NTROL_BUILD_TYPE="Test"
        C0NTROL_BUILD_PROFILE="test"
        C0NTROL_MEDIAPIPE_STATUS="enabled"
        C0NTROL_NATIVE_INPUT_CAPABILITY="test-backend"
    )
    add_test(
        NAME test_production_build_guard
        COMMAND "${CMAKE_COMMAND}"
            -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DBINARY_DIR=${CMAKE_BINARY_DIR}/production-guard-build
            -P ${CMAKE_SOURCE_DIR}/tests/tooling/test_production_guard.cmake
    )
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
