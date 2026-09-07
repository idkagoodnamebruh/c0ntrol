if(NOT DEFINED SOURCE_DIR OR NOT DEFINED BINARY_DIR)
    message(FATAL_ERROR "SOURCE_DIR and BINARY_DIR are required")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}" -B "${BINARY_DIR}"
            -DC0NTROL_PRODUCTION_BUILD=ON
            -DENABLE_MEDIAPIPE=OFF
            -DBUILD_TESTING=OFF
    RESULT_VARIABLE configure_result
    OUTPUT_VARIABLE configure_stdout
    ERROR_VARIABLE configure_stderr
)

set(configure_output "${configure_stdout}\n${configure_stderr}")
if(configure_result EQUAL 0)
    message(FATAL_ERROR
        "Production configuration incorrectly accepted ENABLE_MEDIAPIPE=OFF")
endif()
if(NOT configure_output MATCHES
   "Production build requires ENABLE_MEDIAPIPE=ON")
    message(FATAL_ERROR
        "Production guard failed for an unrelated reason:\n${configure_output}")
endif()

message(STATUS "Production mock-backend guard: PASS")
