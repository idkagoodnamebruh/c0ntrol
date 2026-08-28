#include <filesystem>
#include <iostream>

#include "src/core/runtime/RuntimePaths.h"

int main() {
    const auto windows = RuntimePaths::resolve(
        std::filesystem::path{"C:/Program Files/c0ntrol/c0ntrol.exe"},
        "models");
    if (windows.model.generic_string() !=
        "C:/Program Files/c0ntrol/models/hand_landmarker.task") {
        std::cerr << "[FAIL] Windows runtime model path: "
                  << windows.model.generic_string() << '\n';
        return 1;
    }

    const auto linuxLayout = RuntimePaths::resolve(
        std::filesystem::path{"/opt/c0ntrol/bin/c0ntrol"},
        "../share/c0ntrol/models");
    if (linuxLayout.model.generic_string() !=
        "/opt/c0ntrol/share/c0ntrol/models/hand_landmarker.task") {
        std::cerr << "[FAIL] Linux runtime model path: "
                  << linuxLayout.model.generic_string() << '\n';
        return 1;
    }

    const auto current = RuntimePaths::forCurrentPackage(
        std::filesystem::path{"/tmp/package/bin/c0ntrol"});
    if (current.model.filename() != "hand_landmarker.task") {
        std::cerr << "[FAIL] current package model filename\n";
        return 1;
    }
    return 0;
}
