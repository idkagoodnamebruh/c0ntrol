#include <cassert>
#include <string>

#include "src/core/runtime/BuildMetadata.h"

int main() {
    const std::string version = BuildMetadata::versionText();
    assert(version.find("c0ntrol 1.0") != std::string::npos);
    assert(version.find("Git commit: 0123456789abcdef") != std::string::npos);
    assert(version.find("Build profile: test") != std::string::npos);
    assert(version.find("MediaPipe: enabled") != std::string::npos);
    assert(version.find("Native input: test-backend") != std::string::npos);
    assert(version.find("20:") == std::string::npos);
    assert(version.find("C:/") == std::string::npos);
    assert(BuildMetadata::helpText().find("--version") != std::string::npos);
    return 0;
}
