#include "prx/libSceAudioIn/include/AudioIn.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <vector>

namespace {

constexpr int ErrorInvalidHandle = static_cast<int>(0x80260101);
constexpr int ErrorInvalidPointer = static_cast<int>(0x80260105);
constexpr int ErrorInvalidState = static_cast<int>(0x80260109);

constexpr int UserId = 0x10000000;
constexpr std::uint32_t HandleBase = 0x30000000;

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

void TestUnopenedPortGates() {
    for (const int badHandle : {-100, -1, 0, 1, 7, 8, 15, static_cast<int>(0x20000000u), static_cast<int>(0x30000008u), static_cast<int>(0x300000FFu), 0x7FFFFFFF}) {
        Require(sceAudioInGetSilentState(badHandle) == ErrorInvalidHandle);
        Require(sceAudioInIsPadEmbeddedMic(badHandle) == ErrorInvalidHandle);
        Require(sceAudioInClose(badHandle) == ErrorInvalidHandle);
        Require(sceAudioInInput(badHandle, nullptr) == ErrorInvalidHandle);
        Require(sceAudioInGetRerouteCount(badHandle) == ErrorInvalidHandle);
        Require(sceAudioInGetHandleStatusInfo(badHandle, nullptr) == ErrorInvalidHandle);

        std::array<std::uint8_t, 32> buffer{};
        Require(sceAudioInGetHandleStatusInfo(badHandle, buffer.data()) == ErrorInvalidHandle);
    }

    for (std::uint32_t slot = 0; slot < 8u; ++slot) {
        const int unopenedHandle = static_cast<int>(HandleBase + slot);
        Require(sceAudioInGetSilentState(unopenedHandle) == ErrorInvalidState);
        Require(sceAudioInIsPadEmbeddedMic(unopenedHandle) == ErrorInvalidState);
        Require(sceAudioInClose(unopenedHandle) == ErrorInvalidState);
        Require(sceAudioInInput(unopenedHandle, nullptr) == ErrorInvalidState);
        Require(sceAudioInGetRerouteCount(unopenedHandle) == ErrorInvalidState);
        Require(sceAudioInGetHandleStatusInfo(unopenedHandle, nullptr) == ErrorInvalidState);

        std::array<std::uint8_t, 32> buffer{};
        Require(sceAudioInGetHandleStatusInfo(unopenedHandle, buffer.data()) == ErrorInvalidState);
    }
}

void TestModeConfiguration() {
    for (const int mode : {0, 1, 2, -1, 100}) {
        Require(Throws([&] { sceAudioInSetMode(mode, 10); }));
        Require(Throws([&] { sceAudioInSetMode2(mode, 20, nullptr); }));

        int dummy = 123;
        Require(Throws([&] { sceAudioInSetMode2(mode, 30, &dummy); }));
    }
}

void TestPortLifecycleAndSilentState() {
    Require(Throws([] { sceAudioInSetMode(0, 5); }));

    std::vector<int> handles;
    for (std::uint32_t i = 0; i < 4u; ++i) {
        const int handle = sceAudioInOpen(UserId, 0, 0, 256, 48000, 2);
        Require(handle >= static_cast<int>(HandleBase));
        handles.push_back(handle);
    }

    for (const int handle : handles) {
        const int silentState = sceAudioInGetSilentState(handle);
        Require(silentState == 0 || silentState == 1);
        Require(sceAudioInIsPadEmbeddedMic(handle) == 0);
        Require(sceAudioInInput(handle, nullptr) == 0);
        Require(Throws([&] { return sceAudioInGetHandleStatusInfo(handle, nullptr); }));
    }

    for (const int handle : handles) {
        Require(sceAudioInClose(handle) == 0);

        Require(sceAudioInGetSilentState(handle) == ErrorInvalidState);
        Require(sceAudioInIsPadEmbeddedMic(handle) == ErrorInvalidState);
        Require(sceAudioInClose(handle) == ErrorInvalidState);
        Require(sceAudioInInput(handle, nullptr) == ErrorInvalidState);
        Require(sceAudioInGetRerouteCount(handle) == ErrorInvalidState);
        Require(sceAudioInGetHandleStatusInfo(handle, nullptr) == ErrorInvalidState);
    }

    Require(Throws([] { sceAudioInSetMode(0, 0); }));
}

} // namespace

int main() {
    Require(sceAudioInInit() == 0);

    TestUnopenedPortGates();
    TestModeConfiguration();
    TestPortLifecycleAndSilentState();

    return 0;
}
