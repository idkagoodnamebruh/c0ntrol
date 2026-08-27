# MediaPipe v0.10.26 OpenCV rule adapted only to the verified x64-windows
# vcpkg tree used by the production workflow.

licenses(["notice"])

exports_files(["share/opencv4/copyright"])

config_setting(
    name = "opt_build",
    values = {"compilation_mode": "opt"},
)

config_setting(
    name = "dbg_build",
    values = {"compilation_mode": "dbg"},
)

cc_library(
    name = "opencv",
    srcs = select({
        ":opt_build": glob(
            ["lib/opencv_*.lib"],
            exclude = ["lib/opencv_*d.lib"],
        ),
        ":dbg_build": glob(["debug/lib/opencv_*d.lib"]),
    }),
    hdrs = glob(["include/opencv2/**/*.h*"]),
    includes = ["include/"],
    linkstatic = 1,
    visibility = ["//visibility:public"],
)
