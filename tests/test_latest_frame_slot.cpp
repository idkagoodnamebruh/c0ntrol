#include <atomic>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "src/core/capture/LatestFrameSlot.h"

namespace {

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

CapturedFrame<int> frame(std::uint64_t sequence, int value) {
    return CapturedFrame<int>{value,
                              CapturedFrameMetadata{
                                  sequence,
                                  static_cast<std::int64_t>(sequence)}};
}

} // namespace

int main() {
    LatestFrameSlot<int> slot;
    expect(!slot.consumeLatest().has_value(), "empty slot must stay empty");

    slot.publish(frame(1, 10));
    auto first = slot.consumeLatest();
    expect(first.has_value() && first->value == 10,
           "published frame must be consumable");
    expect(first->metadata.captureSequence == 1,
           "sequence must be preserved");

    slot.reset();
    slot.publish(frame(1, 10));
    slot.publish(frame(2, 20));
    slot.publish(frame(3, 30));
    auto latest = slot.consumeLatest();
    expect(latest.has_value() && latest->value == 30,
           "latest frame must win");
    expect(latest->metadata.captureSequence == 3,
           "latest sequence must be preserved");
    expect(slot.stats().overwrittenFrames == 2,
           "two pending frames must be overwritten");
    expect(!slot.consumeLatest().has_value(),
           "one frame must never be consumed twice");

    slot.reset();
    constexpr std::uint64_t kFrames = 2'000;
    std::atomic<bool> producerDone{false};
    std::uint64_t lastConsumed = 0;
    std::thread producer([&] {
        for (std::uint64_t sequence = 1; sequence <= kFrames; ++sequence)
            slot.publish(frame(sequence, static_cast<int>(sequence)));
        producerDone.store(true);
    });
    std::thread consumer([&] {
        while (!producerDone.load() || slot.stats().hasPendingFrame) {
            auto value = slot.consumeLatest();
            if (!value.has_value()) {
                std::this_thread::yield();
                continue;
            }
            expect(value->metadata.captureSequence > lastConsumed,
                   "consumer sequences must increase");
            lastConsumed = value->metadata.captureSequence;
        }
    });
    producer.join();
    consumer.join();

    const auto stats = slot.stats();
    expect(lastConsumed == kFrames, "consumer must eventually see final frame");
    expect(stats.publishedFrames == kFrames,
           "all producer publications must be counted");
    expect(stats.publishedFrames ==
               stats.consumedFrames + stats.overwrittenFrames,
           "capacity-one accounting must remain bounded and exact");
    return 0;
}
