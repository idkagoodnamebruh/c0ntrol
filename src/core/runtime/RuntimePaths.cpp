#include "RuntimePaths.h"

#ifndef C0NTROL_MODEL_RELATIVE_DIR
#define C0NTROL_MODEL_RELATIVE_DIR "models"
#endif

namespace RuntimePaths {

ResolvedRuntimePaths resolve(
    const std::filesystem::path& executable,
    const std::filesystem::path& modelDirectoryRelativeToExecutable) {
    const auto normalizedExecutable = executable.lexically_normal();
    const auto model = (normalizedExecutable.parent_path() /
                        modelDirectoryRelativeToExecutable /
                        "hand_landmarker.task").lexically_normal();
    return {normalizedExecutable, model};
}

ResolvedRuntimePaths forCurrentPackage(
    const std::filesystem::path& executable) {
    return resolve(executable, C0NTROL_MODEL_RELATIVE_DIR);
}

}  // namespace RuntimePaths
