#include <cassert>
#include <filesystem>

#include "src/core/runtime/RuntimePaths.h"

int main() {
    const auto windows = RuntimePaths::resolve(
        std::filesystem::path{"C:/Program Files/c0ntrol/c0ntrol.exe"},
        "models");
    assert(windows.model.generic_string() ==
           "C:/Program Files/c0ntrol/models/hand_landmarker.task");

    const auto linux = RuntimePaths::resolve(
        std::filesystem::path{"/opt/c0ntrol/bin/c0ntrol"},
        "../share/c0ntrol/models");
    assert(linux.model.generic_string() ==
           "/opt/c0ntrol/share/c0ntrol/models/hand_landmarker.task");

    const auto current = RuntimePaths::forCurrentPackage(
        std::filesystem::path{"/tmp/package/bin/c0ntrol"});
    assert(current.model.filename() == "hand_landmarker.task");
    return 0;
}
