#ifndef RUNTIMEPATHS_H
#define RUNTIMEPATHS_H

#include <filesystem>

struct ResolvedRuntimePaths {
    std::filesystem::path executable;
    std::filesystem::path model;
};

namespace RuntimePaths {

ResolvedRuntimePaths resolve(
    const std::filesystem::path& executable,
    const std::filesystem::path& modelDirectoryRelativeToExecutable);
ResolvedRuntimePaths forCurrentPackage(
    const std::filesystem::path& executable);

}  // namespace RuntimePaths

#endif  // RUNTIMEPATHS_H
