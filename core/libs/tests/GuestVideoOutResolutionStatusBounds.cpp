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
    std::array<std::uint8_t, 80> vrrBuffer{};
    vrrBuffer.fill(0xAA);

    alignas(VideoOutResolutionStatus) std::array<std::uint8_t, sizeof(VideoOutResolutionStatus)> resBuffer{};
    resBuffer.fill(0xBB);

    alignas(VideoOutOutputStatus) std::array<std::uint8_t, sizeof(VideoOutOutputStatus)> outBuffer{};
    outBuffer.fill(0xCC);

    for (const int badHandle : {-1000, -100, -1, 0, 1, 2, 8, 100, 9999, 0x7FFFFFFF}) {
        Require(sceVideoOutGetOutputStatusWithVrr(badHandle, nullptr) == ErrorInvalidAddress);

        Require(sceVideoOutGetOutputStatusWithVrr(badHandle, vrrBuffer.data()) == ErrorInvalidHandle);
        Require(std::all_of(vrrBuffer.begin(), vrrBuffer.end(), [](std::uint8_t b) { return b == 0xAA; }));

        Require(Throws([badHandle] { return sceVideoOutGetResolutionStatus(badHandle, nullptr); }));
        Require(Throws([&] {
            return sceVideoOutGetResolutionStatus(badHandle, reinterpret_cast<VideoOutResolutionStatus*>(resBuffer.data()));
        }));
        Require(std::all_of(resBuffer.begin(), resBuffer.end(), [](std::uint8_t b) { return b == 0xBB; }));

        Require(Throws([badHandle] { return sceVideoOutGetOutputStatus(badHandle, nullptr); }));
        Require(Throws([&] {
            return sceVideoOutGetOutputStatus(badHandle, reinterpret_cast<VideoOutOutputStatus*>(outBuffer.data()));
        }));
        Require(std::all_of(outBuffer.begin(), outBuffer.end(), [](std::uint8_t b) { return b == 0xCC; }));
    }
}

void TestOpenHandleBounds(int handle) {
    Require(sceVideoOutGetOutputStatusWithVrr(handle, nullptr) == ErrorInvalidAddress);
    Require(Throws([handle] { return sceVideoOutGetResolutionStatus(handle, nullptr); }));
    Require(Throws([handle] { return sceVideoOutGetOutputStatus(handle, nullptr); }));

    static_assert(sizeof(VideoOutResolutionStatus) == 48);
    static_assert(sizeof(VideoOutOutputStatus) == 48);

    std::array<std::uint8_t, 80> vrrBuffer{};
    vrrBuffer.fill(0x5A);
    Require(sceVideoOutGetOutputStatusWithVrr(handle, vrrBuffer.data()) == 0);

    VideoOutOutputStatus vrrOutput{};
    std::memcpy(&vrrOutput, vrrBuffer.data(), sizeof(vrrOutput));
    Require(vrrOutput.resolution == 1u || vrrOutput.resolution == 2u);
    Require(vrrOutput.dynamicRange == 0u || vrrOutput.dynamicRange == 1u);
    Require(vrrOutput.flags == 0);
    for (std::size_t i = 48; i < 80; ++i) {
        Require(vrrBuffer[i] == 0x00);
    }

    alignas(VideoOutOutputStatus) std::array<std::uint8_t, sizeof(VideoOutOutputStatus)> outBuffer{};
    outBuffer.fill(0x5A);
    Require(sceVideoOutGetOutputStatus(handle, reinterpret_cast<VideoOutOutputStatus*>(outBuffer.data())) == 0);

    VideoOutOutputStatus output{};
    std::memcpy(&output, outBuffer.data(), sizeof(output));
    Require(output.resolution == vrrOutput.resolution);
    Require(output.dynamicRange == vrrOutput.dynamicRange);
    Require(output.refreshRate == vrrOutput.refreshRate);
    Require(output.flags == 0);

    alignas(VideoOutResolutionStatus) std::array<std::uint8_t, sizeof(VideoOutResolutionStatus)> resBuffer{};
    resBuffer.fill(0x5A);
    Require(sceVideoOutGetResolutionStatus(handle, reinterpret_cast<VideoOutResolutionStatus*>(resBuffer.data())) == 0);

    VideoOutResolutionStatus res{};
    std::memcpy(&res, resBuffer.data(), sizeof(res));
    Require(res.fullWidth > 0);
    Require(res.fullHeight > 0);
    Require(res.paneWidth == res.fullWidth);
    Require(res.paneHeight == res.fullHeight);
    Require(res.screenSizeInInch == 0.0f);
    Require(res.flags == 0);
    Require(res.reserved0 == 0);
    for (std::size_t i = 0; i < 3; ++i) {
        Require(res.reserved1[i] == 0);
    }
}

void TestClosedHandleBounds(int handle) {
    std::array<std::uint8_t, 80> vrrBuffer{};
    vrrBuffer.fill(0x33);

    alignas(VideoOutResolutionStatus) std::array<std::uint8_t, sizeof(VideoOutResolutionStatus)> resBuffer{};
    resBuffer.fill(0x33);

    alignas(VideoOutOutputStatus) std::array<std::uint8_t, sizeof(VideoOutOutputStatus)> outBuffer{};
    outBuffer.fill(0x33);

    Require(sceVideoOutGetOutputStatusWithVrr(handle, nullptr) == ErrorInvalidAddress);
    Require(sceVideoOutGetOutputStatusWithVrr(handle, vrrBuffer.data()) == ErrorInvalidHandle);
    Require(std::all_of(vrrBuffer.begin(), vrrBuffer.end(), [](std::uint8_t b) { return b == 0x33; }));

    Require(Throws([handle] { return sceVideoOutGetResolutionStatus(handle, nullptr); }));
    Require(Throws([&] {
        return sceVideoOutGetResolutionStatus(handle, reinterpret_cast<VideoOutResolutionStatus*>(resBuffer.data()));
    }));
    Require(std::all_of(resBuffer.begin(), resBuffer.end(), [](std::uint8_t b) { return b == 0x33; }));

    Require(Throws([handle] { return sceVideoOutGetOutputStatus(handle, nullptr); }));
    Require(Throws([&] {
        return sceVideoOutGetOutputStatus(handle, reinterpret_cast<VideoOutOutputStatus*>(outBuffer.data()));
    }));
    Require(std::all_of(outBuffer.begin(), outBuffer.end(), [](std::uint8_t b) { return b == 0x33; }));
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
