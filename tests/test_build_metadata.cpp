#include <iostream>
#include <string>

#include "src/core/runtime/BuildMetadata.h"

int main() {
    const std::string version = BuildMetadata::versionText();
    const bool valid =
        version.find("c0ntrol 1.0") != std::string::npos &&
        version.find("Git commit: 0123456789abcdef") != std::string::npos &&
        version.find("Build profile: test") != std::string::npos &&
        version.find("MediaPipe: enabled") != std::string::npos &&
        version.find("Native input: test-backend") != std::string::npos &&
        version.find("20:") == std::string::npos &&
        version.find("C:/") == std::string::npos &&
        BuildMetadata::helpText().find("--version") != std::string::npos;
    if (!valid) {
        std::cerr << "[FAIL] invalid build metadata:\n" << version;
        return 1;
    }
    return 0;
}
