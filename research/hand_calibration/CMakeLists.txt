cmake_minimum_required(VERSION 3.16)
project(c0ntrol_hand_calibration_research LANGUAGES CXX)

# Standalone entry point. Never add_subdirectory this from the production build.
enable_testing()
get_filename_component(C0NTROL_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
add_executable(test_hand_calibration
    "${C0NTROL_ROOT}/tests/test_hand_calibration.cpp"
    "${C0NTROL_ROOT}/src/core/calibration/HandCalibrationSession.cpp"
    "${C0NTROL_ROOT}/src/core/calibration/HandCalibrationStatistics.cpp"
)
target_include_directories(test_hand_calibration PRIVATE "${C0NTROL_ROOT}")
target_compile_features(test_hand_calibration PRIVATE cxx_std_20)
set_target_properties(test_hand_calibration PROPERTIES CXX_EXTENSIONS OFF)
if(MSVC)
    target_compile_options(test_hand_calibration PRIVATE /W4 /WX /permissive-)
else()
    target_compile_options(test_hand_calibration PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror)
endif()
add_test(NAME test_hand_calibration COMMAND test_hand_calibration)
