foreach(required ROOT EXECUTABLE_RELATIVE MODEL_RELATIVE BRIDGE_RELATIVE
                 MANIFEST_RELATIVE EXPECTED_MODEL_SHA)
    if(NOT DEFINED ${required})
        message(FATAL_ERROR "${required} is required")
    endif()
endforeach()

set(executable "${ROOT}/${EXECUTABLE_RELATIVE}")
set(model "${ROOT}/${MODEL_RELATIVE}")
set(bridge "${ROOT}/${BRIDGE_RELATIVE}")
set(manifest "${ROOT}/${MANIFEST_RELATIVE}")

if(NOT DEFINED README_RELATIVE)
    set(README_RELATIVE "docs/README.md")
endif()
if(NOT DEFINED NOTICES_RELATIVE)
    set(NOTICES_RELATIVE "docs/THIRD_PARTY_NOTICES.md")
endif()

foreach(required_file "${executable}" "${model}" "${bridge}" "${manifest}")
    if(NOT EXISTS "${required_file}")
        message(FATAL_ERROR "Required package file is missing: ${required_file}")
    endif()
endforeach()
foreach(document "${ROOT}/${README_RELATIVE}" "${ROOT}/${NOTICES_RELATIVE}")
    if(NOT EXISTS "${document}")
        message(FATAL_ERROR "Required package document is missing: ${document}")
    endif()
endforeach()

file(SHA256 "${model}" actual_model_sha)
if(NOT actual_model_sha STREQUAL EXPECTED_MODEL_SHA)
    message(FATAL_ERROR
        "Packaged model SHA-256 mismatch: ${actual_model_sha}")
endif()

file(READ "${manifest}" manifest_contents)
foreach(required_text "Build profile: production" "MediaPipe: enabled"
                      "Hand model SHA-256: ${EXPECTED_MODEL_SHA}")
    string(FIND "${manifest_contents}" "${required_text}" found_at)
    if(found_at EQUAL -1)
        message(FATAL_ERROR
            "Release manifest lacks required text: ${required_text}")
    endif()
endforeach()
if(manifest_contents MATCHES "(/home/runner|[A-Za-z]:[/\\\\].*runner|_work[/\\\\])")
    message(FATAL_ERROR "Release manifest leaks a runner path")
endif()

if(EXPECT_WINDOWS_RUNTIME)
    file(GLOB qt_core "${ROOT}/Qt6Core*.dll")
    file(GLOB opencv_runtime "${ROOT}/opencv_*.dll")
    if(NOT qt_core OR NOT EXISTS "${ROOT}/platforms/qwindows.dll")
        message(FATAL_ERROR "Qt Windows runtime or qwindows plugin is missing")
    endif()
    if(NOT opencv_runtime)
        message(FATAL_ERROR "OpenCV Windows runtime DLLs are missing")
    endif()
endif()

if(RUN_SMOKE)
    execute_process(
        COMMAND "${executable}" --version
        WORKING_DIRECTORY "${ROOT}"
        RESULT_VARIABLE smoke_result
        OUTPUT_VARIABLE smoke_stdout
        ERROR_VARIABLE smoke_stderr
    )
    if(NOT smoke_result EQUAL 0)
        message(FATAL_ERROR
            "Packaged --version failed (${smoke_result}):\n${smoke_stdout}\n${smoke_stderr}")
    endif()
    if(NOT smoke_stdout MATCHES "Build profile: production" OR
       NOT smoke_stdout MATCHES "MediaPipe: enabled")
        message(FATAL_ERROR
            "Packaged --version returned incomplete metadata:\n${smoke_stdout}")
    endif()
endif()

message(STATUS "Package integrity and hardware-free smoke: PASS")
