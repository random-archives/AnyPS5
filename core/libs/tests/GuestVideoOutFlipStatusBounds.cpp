#include "prx/libSceVideoOut/include/Output.hpp"
#include "SceTypes.hpp"
#include "tests/VideoOutTestEnvironment.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace {

constexpr int SystemUser = 255;
constexpr int MainBus = 0;

constexpr int ErrorInvalidAddress = static_cast<int>(0x80290002u);
constexpr int ErrorInvalidHandle = static_cast<int>(0x8029000Bu);

void Require(bool value) {
    if (!value) std::abort();
}

template <typename Fn>
bool Throws(Fn&& fn) {
    try {
        fn();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

void TestUnopenedHandleBounds() {
    std::array<std::uint8_t, sizeof(VideoOutFlipStatus)> untouched{};
    untouched.fill(0xAA);

    for (const int badHandle : {-1000, -100, -1, 0, 1, 2, 8, 100, 9999, 0x7FFFFFFF}) {
        Require(sceVideoOutGetFlipStatus(badHandle, reinterpret_cast<VideoOutFlipStatus*>(untouched.data())) == ErrorInvalidHandle);
        Require(std::all_of(untouched.begin(), untouched.end(), [](std::uint8_t b) { return b == 0xAA; }));

        Require(Throws([badHandle] { return sceVideoOutGetFlipStatus(badHandle, nullptr); }));

        Require(sceVideoOutIsFlipPending(badHandle) == ErrorInvalidHandle);
    }
}

void TestOpenHandleBounds(int handle) {
    Require(sceVideoOutGetFlipStatus(handle, nullptr) == ErrorInvalidAddress);

    Require(sceVideoOutIsFlipPending(handle) == 0);

    static_assert(sizeof(VideoOutFlipStatus) == 128);

    std::array<std::uint8_t, sizeof(VideoOutFlipStatus)> rawBuffer{};
    rawBuffer.fill(0x5A);

    Require(sceVideoOutGetFlipStatus(handle, reinterpret_cast<VideoOutFlipStatus*>(rawBuffer.data())) == 0);

    VideoOutFlipStatus status{};
    std::memcpy(&status, rawBuffer.data(), sizeof(status));

    Require(status.count == 0);
    Require(status.processTime == 0);
    Require(status.reserved0 == 0);
    Require(status.flipArg == -1);
    Require(status.reserved1 == 0);
    Require(status.processTimeCounter == 0);
    Require(status.gcQueueNum == 0);
    Require(status.flipPendingNum == 0);
    Require(status.currentBuffer == -1);
    Require(status.reserved2 == 0);
    Require(status.submitProcessTimeCounter == 0);
    for (std::size_t i = 0; i < 7; ++i) {
        Require(status.reserved3[i] == 0);
    }

    for (std::size_t i = 0; i < 24; ++i) {
        Require(rawBuffer[i] == 0x00);
    }
    for (std::size_t i = 24; i < 32; ++i) {
        Require(rawBuffer[i] == 0xFF);
    }
    for (std::size_t i = 32; i < 48; ++i) {
        Require(rawBuffer[i] == 0x00);
    }
    for (std::size_t i = 48; i < 52; ++i) {
        Require(rawBuffer[i] == 0xFF);
    }
    for (std::size_t i = 52; i < 128; ++i) {
        Require(rawBuffer[i] == 0x00);
    }

    Require(sceVideoOutIsFlipPending(handle) == 0);
}

void TestClosedHandleBounds(int handle) {
    std::array<std::uint8_t, sizeof(VideoOutFlipStatus)> untouched{};
    untouched.fill(0x33);

    Require(sceVideoOutGetFlipStatus(handle, reinterpret_cast<VideoOutFlipStatus*>(untouched.data())) == ErrorInvalidHandle);
    Require(std::all_of(untouched.begin(), untouched.end(), [](std::uint8_t b) { return b == 0x33; }));

    Require(Throws([handle] { return sceVideoOutGetFlipStatus(handle, nullptr); }));

    Require(sceVideoOutIsFlipPending(handle) == ErrorInvalidHandle);
}

} // namespace

int main() {
    VideoOutTestEnvironment environment;

    TestUnopenedHandleBounds();

    int handle = -1;
    try {
        handle = sceVideoOutOpen(SystemUser, MainBus, 0, nullptr);
    } catch (const std::runtime_error& error) {
        if (std::getenv("ANYPS5_REQUIRE_DISPLAY") != nullptr) throw;
        return 77;
    }

    Require(handle >= 0);

    TestOpenHandleBounds(handle);

    Require(sceVideoOutClose(handle) == 0);

    TestClosedHandleBounds(handle);

    return 0;
}
