#include "SettingsCodec.h"

#include <charconv>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string_view>

namespace {

template <typename Integer>
bool parseInteger(const std::string& text, Integer& value) {
    const char* begin = text.data();
    const char* end = begin + text.size();
    const auto result = std::from_chars(begin, end, value);
    return result.ec == std::errc{} && result.ptr == end;
}

bool parseDouble(const std::string& text, double& value) {
    std::istringstream stream(text);
    stream.imbue(std::locale::classic());
    stream >> std::noskipws >> value;
    return stream.eof() && !stream.fail() && std::isfinite(value);
}

bool parseBool(const std::string& text, bool& value) {
    if (text == "true" || text == "1") {
        value = true;
        return true;
    }
    if (text == "false" || text == "0") {
        value = false;
        return true;
    }
    return false;
}

std::string encodeDouble(double value) {
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << std::setprecision(std::numeric_limits<double>::max_digits10)
           << value;
    return stream.str();
}

template <typename Value, typename Parser>
void readValue(const SettingsMap& values, const char* key, Value& target,
               Parser parser, std::size_t& missing, std::size_t& invalid) {
    const auto item = values.find(key);
    if (item == values.end()) {
        ++missing;
        return;
    }
    Value parsed{};
    if (!parser(item->second, parsed)) {
        ++invalid;
        return;
    }
    target = parsed;
}

void readOneEuro(const SettingsMap& values, const std::string& prefix,
                 OneEuroConfig& config, std::size_t& missing,
                 std::size_t& invalid) {
    readValue(values, (prefix + "minCutoff").c_str(), config.minCutoff,
              parseDouble, missing, invalid);
    readValue(values, (prefix + "beta").c_str(), config.beta,
              parseDouble, missing, invalid);
    readValue(values, (prefix + "derivativeCutoff").c_str(),
              config.derivativeCutoff, parseDouble, missing, invalid);
    readValue(values, (prefix + "maxDeltaSeconds").c_str(),
              config.maxDeltaSeconds, parseDouble, missing, invalid);
}

void writeOneEuro(SettingsMap& values, const std::string& prefix,
                  const OneEuroConfig& config) {
    values[prefix + "minCutoff"] = encodeDouble(config.minCutoff);
    values[prefix + "beta"] = encodeDouble(config.beta);
    values[prefix + "derivativeCutoff"] =
        encodeDouble(config.derivativeCutoff);
    values[prefix + "maxDeltaSeconds"] =
        encodeDouble(config.maxDeltaSeconds);
}

} // namespace

SettingsMap encodeRuntimeConfig(const RuntimeConfig& rawConfig) {
    const RuntimeConfig config = sanitizeRuntimeConfig(rawConfig);
    SettingsMap values;
    values["configVersion"] = std::to_string(kRuntimeConfigVersion);
    values["camera/index"] = std::to_string(config.camera.index);
    values["camera/width"] = std::to_string(config.camera.requestedWidth);
    values["camera/height"] = std::to_string(config.camera.requestedHeight);
    values["camera/fps"] = encodeDouble(config.camera.requestedFps);
    values["camera/bufferSize"] =
        std::to_string(config.camera.requestedBufferSize);

    values["pointer/mirrorX"] = config.pointer.mirrorX ? "true" : "false";
    values["pointer/mirrorY"] = config.pointer.mirrorY ? "true" : "false";
    values["pointer/leftMargin"] = encodeDouble(config.pointer.leftMargin);
    values["pointer/rightMargin"] = encodeDouble(config.pointer.rightMargin);
    values["pointer/topMargin"] = encodeDouble(config.pointer.topMargin);
    values["pointer/bottomMargin"] = encodeDouble(config.pointer.bottomMargin);

    values["filter/enabled"] = config.filtering.enabled ? "true" : "false";
    writeOneEuro(values, "filter/normalized/", config.filtering.normalized);
    writeOneEuro(values, "filter/world/", config.filtering.world);
    values["filter/handResetTimeoutUs"] =
        std::to_string(config.filtering.handResetTimeoutUs);
    values["filter/teleportThreshold"] =
        encodeDouble(config.filtering.teleportThreshold);

    values["gesture/pinchEnterRatio"] =
        encodeDouble(config.gestures.pinchEnterRatio);
    values["gesture/pinchExitRatio"] =
        encodeDouble(config.gestures.pinchExitRatio);
    values["gesture/pinchEnterHoldUs"] =
        std::to_string(config.gestures.pinchEnterHoldUs);
    values["gesture/pinchExitHoldUs"] =
        std::to_string(config.gestures.pinchExitHoldUs);
    values["gesture/trackingLostTimeoutUs"] =
        std::to_string(config.gestures.trackingLostTimeoutUs);
    values["gesture/fingerExtendedMaxCurl"] =
        encodeDouble(config.gestures.fingerExtendedMaxCurl);
    values["gesture/fingerCurledMinCurl"] =
        encodeDouble(config.gestures.fingerCurledMinCurl);
    values["gesture/thumbExtendedMaxCurl"] =
        encodeDouble(config.gestures.thumbExtendedMaxCurl);
    values["gesture/thumbMinSpreadRatio"] =
        encodeDouble(config.gestures.thumbMinSpreadRatio);
    values["gesture/handScaleEpsilon"] =
        encodeDouble(config.gestures.handScaleEpsilon);

    values["input/enabled"] = config.input.enabled ? "true" : "false";
    values["input/preferredHand"] =
        config.input.preferredHand == Handedness::LEFT ? "LEFT" : "RIGHT";
    return values;
}

ConfigDecodeResult decodeRuntimeConfig(const SettingsMap& values) {
    ConfigDecodeResult result;
    if (values.empty()) {
        result.usedDefaults = true;
        return result;
    }

    std::size_t missing = 0;
    std::size_t invalid = 0;
    int version = 0;
    const auto versionItem = values.find("configVersion");
    if (versionItem == values.end()) {
        result.migrated = true;
    } else if (!parseInteger(versionItem->second, version) || version < 0) {
        result.usedDefaults = true;
        result.warning = "invalid config schema version; safe defaults used";
        return result;
    } else if (version > kRuntimeConfigVersion) {
        result.usedDefaults = true;
        result.warning = "unsupported future config schema; safe defaults used";
        return result;
    } else if (version < kRuntimeConfigVersion) {
        result.migrated = true;
    }

    RuntimeConfig config;
    readValue(values, "camera/index", config.camera.index,
              parseInteger<int>, missing, invalid);
    readValue(values, "camera/width", config.camera.requestedWidth,
              parseInteger<int>, missing, invalid);
    readValue(values, "camera/height", config.camera.requestedHeight,
              parseInteger<int>, missing, invalid);
    readValue(values, "camera/fps", config.camera.requestedFps,
              parseDouble, missing, invalid);
    readValue(values, "camera/bufferSize", config.camera.requestedBufferSize,
              parseInteger<int>, missing, invalid);

    readValue(values, "pointer/mirrorX", config.pointer.mirrorX,
              parseBool, missing, invalid);
    readValue(values, "pointer/mirrorY", config.pointer.mirrorY,
              parseBool, missing, invalid);
    readValue(values, "pointer/leftMargin", config.pointer.leftMargin,
              parseDouble, missing, invalid);
    readValue(values, "pointer/rightMargin", config.pointer.rightMargin,
              parseDouble, missing, invalid);
    readValue(values, "pointer/topMargin", config.pointer.topMargin,
              parseDouble, missing, invalid);
    readValue(values, "pointer/bottomMargin", config.pointer.bottomMargin,
              parseDouble, missing, invalid);

    readValue(values, "filter/enabled", config.filtering.enabled,
              parseBool, missing, invalid);
    readOneEuro(values, "filter/normalized/", config.filtering.normalized,
                missing, invalid);
    readOneEuro(values, "filter/world/", config.filtering.world,
                missing, invalid);
    readValue(values, "filter/handResetTimeoutUs",
              config.filtering.handResetTimeoutUs,
              parseInteger<std::int64_t>, missing, invalid);
    readValue(values, "filter/teleportThreshold",
              config.filtering.teleportThreshold, parseDouble, missing, invalid);

    readValue(values, "gesture/pinchEnterRatio",
              config.gestures.pinchEnterRatio, parseDouble, missing, invalid);
    readValue(values, "gesture/pinchExitRatio",
              config.gestures.pinchExitRatio, parseDouble, missing, invalid);
    readValue(values, "gesture/pinchEnterHoldUs",
              config.gestures.pinchEnterHoldUs,
              parseInteger<std::int64_t>, missing, invalid);
    readValue(values, "gesture/pinchExitHoldUs",
              config.gestures.pinchExitHoldUs,
              parseInteger<std::int64_t>, missing, invalid);
    readValue(values, "gesture/trackingLostTimeoutUs",
              config.gestures.trackingLostTimeoutUs,
              parseInteger<std::int64_t>, missing, invalid);
    readValue(values, "gesture/fingerExtendedMaxCurl",
              config.gestures.fingerExtendedMaxCurl,
              parseDouble, missing, invalid);
    readValue(values, "gesture/fingerCurledMinCurl",
              config.gestures.fingerCurledMinCurl,
              parseDouble, missing, invalid);
    readValue(values, "gesture/thumbExtendedMaxCurl",
              config.gestures.thumbExtendedMaxCurl,
              parseDouble, missing, invalid);
    readValue(values, "gesture/thumbMinSpreadRatio",
              config.gestures.thumbMinSpreadRatio,
              parseDouble, missing, invalid);
    readValue(values, "gesture/handScaleEpsilon",
              config.gestures.handScaleEpsilon,
              parseDouble, missing, invalid);

    readValue(values, "input/enabled", config.input.enabled,
              parseBool, missing, invalid);
    const auto preferred = values.find("input/preferredHand");
    if (preferred == values.end()) {
        ++missing;
    } else if (preferred->second == "LEFT") {
        config.input.preferredHand = Handedness::LEFT;
    } else if (preferred->second == "RIGHT") {
        config.input.preferredHand = Handedness::RIGHT;
    } else {
        ++invalid;
    }

    result.config = sanitizeRuntimeConfig(config);
    const bool normalized = result.config != config;
    result.usedDefaults = missing > 0 || invalid > 0 || normalized;
    if (invalid > 0) {
        result.warning = std::to_string(invalid) +
            " invalid setting value(s) were replaced with safe defaults";
    } else if (normalized) {
        result.warning =
            "out-of-range setting value(s) were replaced with safe defaults";
    }
    return result;
}
