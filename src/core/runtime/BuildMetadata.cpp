#include "BuildMetadata.h"

#ifndef C0NTROL_VERSION
#define C0NTROL_VERSION "unknown"
#endif
#ifndef C0NTROL_GIT_COMMIT
#define C0NTROL_GIT_COMMIT "unknown"
#endif
#ifndef C0NTROL_BUILD_TYPE
#define C0NTROL_BUILD_TYPE "unknown"
#endif
#ifndef C0NTROL_BUILD_PROFILE
#define C0NTROL_BUILD_PROFILE "development"
#endif
#ifndef C0NTROL_MEDIAPIPE_STATUS
#define C0NTROL_MEDIAPIPE_STATUS "disabled"
#endif
#ifndef C0NTROL_NATIVE_INPUT_CAPABILITY
#define C0NTROL_NATIVE_INPUT_CAPABILITY "unavailable"
#endif

namespace BuildMetadata {

std::string versionText() {
    return std::string{"c0ntrol "} + C0NTROL_VERSION +
        "\nGit commit: " + C0NTROL_GIT_COMMIT +
        "\nBuild type: " + C0NTROL_BUILD_TYPE +
        "\nBuild profile: " + C0NTROL_BUILD_PROFILE +
        "\nMediaPipe: " + C0NTROL_MEDIAPIPE_STATUS +
        "\nNative input: " + C0NTROL_NATIVE_INPUT_CAPABILITY + "\n";
}

std::string helpText() {
    return "Usage: c0ntrol [--help] [--version]\n"
           "\n"
           "  --help       Show this hardware-free help and exit.\n"
           "  --version    Show deterministic build metadata and exit.\n";
}

}  // namespace BuildMetadata
