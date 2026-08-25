#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "src/platform/linux/LinuxEisSystemInputBackend.h"

namespace {

void require(bool condition, const char* message) {
    if (condition) return;
    std::cerr << "[FAIL] " << message << '\n';
    std::exit(1);
}

class FakeEisInputSession final : public IEisInputSession {
public:
    bool initialize() override {
        ++initializeCount;
        initialized = !failInitialize;
        if (!initialized) error = "fake portal permission denied";
        return initialized;
    }
    bool refresh() override {
        ++refreshCount;
        if (!initialized || disconnected) {
            error = "fake EIS disconnected";
            return false;
        }
        return true;
    }
    EisCapabilities capabilities() const override { return caps; }
    std::vector<EisRegion> regions() const override { return regionList; }
    bool moveAbsolute(const DesktopPoint& point) override {
        if (failOperation) {
            error = "fake move failure";
            return false;
        }
        moves.push_back(point);
        return true;
    }
    bool button(bool pressed) override {
        if (failOperation) {
            error = "fake button failure";
            return false;
        }
        buttons.push_back(pressed);
        return true;
    }
    bool scrollVertical(int logicalNotches) override {
        if (failOperation) {
            error = "fake scroll failure";
            return false;
        }
        scrolls.push_back(logicalNotches);
        return true;
    }
    void shutdown() override {
        ++shutdownCount;
        initialized = false;
    }
    std::string lastError() const override { return error; }

    EisCapabilities caps{true, true, true};
    std::vector<EisRegion> regionList{{0, 0, 1920, 1080}};
    std::vector<DesktopPoint> moves;
    std::vector<bool> buttons;
    std::vector<int> scrolls;
    int initializeCount{0};
    int refreshCount{0};
    int shutdownCount{0};
    bool initialized{false};
    bool failInitialize{false};
    bool failOperation{false};
    bool disconnected{false};
    std::string error;
};

struct Harness {
    Harness() : owned(std::make_unique<FakeEisInputSession>()),
                fake(owned.get()), backend(std::move(owned)) {}
    std::unique_ptr<FakeEisInputSession> owned;
    FakeEisInputSession* fake;
    LinuxEisSystemInputBackend backend;
};

void testCapabilitiesAndGeometry() {
    Harness ready;
    ready.fake->regionList = {{-100, 0, 100, 100}, {0, 0, 200, 120}};
    require(ready.backend.initialize(), "complete capabilities become READY");
    const DesktopGeometry geometry = ready.backend.desktopGeometry();
    require(geometry.originX == -100 && geometry.width == 300 &&
                geometry.height == 120,
            "backend exposes the EIS region bounding geometry");

    for (int missing = 0; missing < 3; ++missing) {
        Harness incomplete;
        if (missing == 0) incomplete.fake->caps.absolutePointer = false;
        if (missing == 1) incomplete.fake->caps.button = false;
        if (missing == 2) incomplete.fake->caps.scroll = false;
        require(!incomplete.backend.initialize() &&
                    !incomplete.backend.lastError().empty(),
                "each required EIS capability is enforced");
    }
}

void testMotionButtonAndScroll() {
    Harness harness;
    harness.fake->regionList = {{0, 0, 100, 100}, {200, 0, 100, 100}};
    require(harness.backend.initialize(), "backend initializes for operations");
    require(harness.backend.movePointer({25, 50}) &&
                harness.fake->moves.size() == 1,
            "absolute move inside a region passes");
    require(!harness.backend.movePointer({150, 50}) &&
                harness.backend.lastError().find("outside") != std::string::npos,
            "absolute move in a monitor gap fails clearly");
    require(harness.backend.primaryButtonDown() &&
                harness.backend.primaryButtonDown() &&
                harness.backend.primaryButtonUp() &&
                harness.fake->buttons == std::vector<bool>({true, false}),
            "button ownership suppresses duplicate DOWN and emits UP");
    require(harness.backend.scrollVertical(3) &&
                harness.backend.scrollVertical(-2) &&
                harness.fake->scrolls == std::vector<int>({3, -2}),
            "positive and negative logical scroll values are preserved");
    require(!harness.backend.scrollVertical(0),
            "zero logical scroll is rejected");
}

void testPauseRemoveRecoveryAndFailure() {
    Harness harness;
    require(harness.backend.initialize(), "lifecycle backend initializes");
    harness.fake->caps.absolutePointer = false;
    require(!harness.backend.movePointer({10, 10}),
            "paused or removed absolute device fails cleanly");
    harness.fake->caps.absolutePointer = true;
    require(harness.backend.movePointer({10, 10}),
            "resumed absolute device recovers without reinitialization");
    harness.fake->disconnected = true;
    require(!harness.backend.primaryButtonDown() &&
                harness.backend.lastError().find("disconnected") !=
                    std::string::npos,
            "disconnect propagates a stable error");
}

void testDefensiveShutdown() {
    Harness harness;
    require(harness.backend.initialize() &&
                harness.backend.primaryButtonDown(),
            "shutdown test owns BTN_LEFT");
    harness.backend.shutdown();
    harness.backend.shutdown();
    require(harness.fake->buttons == std::vector<bool>({true, false}) &&
                harness.fake->shutdownCount == 1,
            "shutdown emits one defensive UP and is idempotent");
}

} // namespace

int main() {
    testCapabilitiesAndGeometry();
    testMotionButtonAndScroll();
    testPauseRemoveRecoveryAndFailure();
    testDefensiveShutdown();
    std::cout << "[PASS] test_linux_eis_backend\n";
    return 0;
}
