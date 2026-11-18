#include "prx/libSceAudioOut/include/AudioOut.hpp"
#include "SceTypes.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace {

constexpr int errorInvalidPointer = -2144993276;
constexpr int errorInvalidHandle = -2144993277;
constexpr int errorInvalidSize = -2144993274;
constexpr int errorInvalidType = -2144993268;

static void Require(bool condition) {
    if (!condition) {
        std::abort();
    }
}

template <typename F>
static bool ThrowsRuntimeError(F&& func) {
    try {
        func();
        return false;
    } catch (const std::runtime_error&) {
        return true;
    } catch (...) {
        return false;
    }
}

static void TestSysOpenModes() {
    for (std::uint32_t mode : {0u, 1u, 2u, 3u, 4u, 6u, 7u, 8u, 9u, 10u, 0xFFu, 0x1000u}) {
        Require(ThrowsRuntimeError([mode] {
            static_cast<void>(sceAudioOutSysOpen(255, mode));
        }));
    }

    const int sysHandle = sceAudioOutSysOpen(255, 5);
    Require(sysHandle > 0 && sysHandle <= 32);

    AudioOutPortState state{};
    Require(ThrowsRuntimeError([sysHandle, &state] {
        static_cast<void>(sceAudioOutGetPortState(sysHandle, &state));
    }));

    std::vector<std::int16_t> sysBuf(1024 * 8, 0);
    Require(sceAudioOutOutput(sysHandle, sysBuf.data()) == 1024);
    Require(sceAudioOutOutput(sysHandle, nullptr) == 1024);

    Require(sceAudioOutSysClose(sysHandle) == 0);

    Require(ThrowsRuntimeError([sysHandle] {
        static_cast<void>(sceAudioOutSysClose(sysHandle));
    }));
}

static void TestSysConfigureOutputParameters() {
    Require(sceAudioOutSysConfigureOutput(1, 0, 5, 1, 0) == 0);

    for (int type : {0, -1, 2, 3, 10}) {
        Require(ThrowsRuntimeError([type] {
            static_cast<void>(sceAudioOutSysConfigureOutput(type, 0, 5, 1, 0));
        }));
    }

    for (std::uint64_t flags : {1u, 2u, 0x80000000ULL, 0xFFFFFFFF00000000ULL}) {
        Require(ThrowsRuntimeError([flags] {
            static_cast<void>(sceAudioOutSysConfigureOutput(1, flags, 5, 1, 0));
        }));
    }

    for (std::uint64_t option : {1u, 5u, 0x1000ULL}) {
        Require(ThrowsRuntimeError([option] {
            static_cast<void>(sceAudioOutSysConfigureOutput(1, 0, 5, 1, option));
        }));
    }

    for (std::uint32_t mode : {0u, 1u, 2u, 3u, 4u, 6u, 7u, 10u, 0xFFu}) {
        Require(ThrowsRuntimeError([mode] {
            static_cast<void>(sceAudioOutSysConfigureOutput(1, 0, mode, 1, 0));
        }));
    }

    for (std::uint32_t target : {0u, 2u, 3u, 4u, 0xFFu}) {
        Require(ThrowsRuntimeError([target] {
            static_cast<void>(sceAudioOutSysConfigureOutput(1, 0, 5, target, 0));
        }));
    }
}

static void TestSysCloseHandleRejection() {
    for (int handle : {-100, -1, 0}) {
        Require(ThrowsRuntimeError([handle] {
            static_cast<void>(sceAudioOutSysClose(handle));
        }));
    }

    for (int handle : {33, 50, 100, 0x7FFF}) {
        Require(ThrowsRuntimeError([handle] {
            static_cast<void>(sceAudioOutSysClose(handle));
        }));
    }

    for (int handle : {1, 2, 16, 32}) {
        Require(ThrowsRuntimeError([handle] {
            static_cast<void>(sceAudioOutSysClose(handle));
        }));
    }
}

static void TestSysCloseNonSysPortRejection() {
    const int standardPort = sceAudioOutOpen(255, 0, 0, 256, 48000, 1);
    Require(standardPort > 0);
    Require(ThrowsRuntimeError([standardPort] {
        static_cast<void>(sceAudioOutSysClose(standardPort));
    }));
    Require(sceAudioOutClose(standardPort) == 0);

    const int exPort = sceAudioOutExOpen(255, 0);
    Require(exPort > 0);
    Require(ThrowsRuntimeError([exPort] {
        static_cast<void>(sceAudioOutSysClose(exPort));
    }));
    Require(sceAudioOutExClose(exPort) == 0);

    const int bgmPort = sceAudioOutOpen(255, 1, 0, 256, 48000, 1);
    Require(bgmPort > 0);
    Require(ThrowsRuntimeError([bgmPort] {
        static_cast<void>(sceAudioOutSysClose(bgmPort));
    }));
    Require(sceAudioOutClose(bgmPort) == 0);
}

static void TestSysPortLifecycleAndCloseByRegularClose() {
    const int sysHandle = sceAudioOutSysOpen(255, 5);
    Require(sysHandle > 0);

    Require(sceAudioOutClose(sysHandle) == 0);

    Require(ThrowsRuntimeError([sysHandle] {
        static_cast<void>(sceAudioOutSysClose(sysHandle));
    }));

    Require(sceAudioOutClose(sysHandle) == errorInvalidHandle);
}

static void TestSysHdmiMonitorInfo() {
    constexpr std::uint32_t hdmiMonitorInfoSize = 0x180;
    std::array<std::uint8_t, hdmiMonitorInfoSize> infoBuffer{};

    Require(sceAudioOutSysGetHdmiMonitorInfo(1, nullptr, hdmiMonitorInfoSize) == errorInvalidPointer);

    Require(sceAudioOutSysGetHdmiMonitorInfo(1, infoBuffer.data(), 0) == errorInvalidSize);
    Require(sceAudioOutSysGetHdmiMonitorInfo(1, infoBuffer.data(), 100) == errorInvalidSize);
    Require(sceAudioOutSysGetHdmiMonitorInfo(1, infoBuffer.data(), 1024) == errorInvalidSize);

    for (int type : {0, -1, 2, 3, 10}) {
        Require(sceAudioOutSysGetHdmiMonitorInfo(type, infoBuffer.data(), hdmiMonitorInfoSize) == errorInvalidType);
    }

    Require(ThrowsRuntimeError([&infoBuffer] {
        static_cast<void>(sceAudioOutSysGetHdmiMonitorInfo(0x10, infoBuffer.data(), hdmiMonitorInfoSize));
    }));

    infoBuffer.fill(0xAA);
    Require(sceAudioOutSysGetHdmiMonitorInfo(1, infoBuffer.data(), hdmiMonitorInfoSize) == 0);
    for (std::uint8_t byte : infoBuffer) {
        Require(byte == 0);
    }
}

} // namespace

int main() {
    Require(sceAudioOutInit() == 0);

    TestSysOpenModes();
    TestSysConfigureOutputParameters();
    TestSysCloseHandleRejection();
    TestSysCloseNonSysPortRejection();
    TestSysPortLifecycleAndCloseByRegularClose();
    TestSysHdmiMonitorInfo();

    LibcRunShutdown_nid_postfix();
    return 0;
}
