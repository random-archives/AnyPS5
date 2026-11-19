#include "prx/libSceAudioIn/include/AudioIn.hpp"
#include "prx/libSceAudioIn/include/Exports.hpp"
#include "SceTypes.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace {

constexpr int audioInErrorInvalidHandle = static_cast<int>(0x80260101);
constexpr int audioInErrorInvalidSize = static_cast<int>(0x80260102);
constexpr int audioInErrorInvalidFreq = static_cast<int>(0x80260103);
constexpr int audioInErrorInvalidType = static_cast<int>(0x80260104);
constexpr int audioInErrorInvalidPointer = static_cast<int>(0x80260105);
constexpr int audioInErrorInvalidParam = static_cast<int>(0x80260106);
constexpr int audioInErrorInvalidState = static_cast<int>(0x80260109);
constexpr int audioInErrorInvalidDevice = static_cast<int>(0x8026010F);
constexpr int userId = 0x10000000;

void Require(bool condition) {
    if (!condition) {
        std::abort();
    }
}

void TestMicGainAndLevelBounds() {
    Require(sceAudioInSetMicLevel(1, 0, 101, 0, 0) == audioInErrorInvalidParam);
    Require(sceAudioInSetMicLevel(1, 0, 200, 0, 0) == audioInErrorInvalidParam);
    Require(sceAudioInSetMicLevel(0, 0, 101, 0, 0) == audioInErrorInvalidParam);
    Require(sceAudioInSetMicLevel(-1, 0, 101, 0, 0) == audioInErrorInvalidParam);

    Require(sceAudioInSetMicLevel(0, 0, 50, 0, 0) == audioInErrorInvalidDevice);
    Require(sceAudioInSetMicLevel(5, 0, 50, 0, 0) == audioInErrorInvalidDevice);
    Require(sceAudioInSetMicLevel(6, 0, 50, 0, 0) == audioInErrorInvalidDevice);
    Require(sceAudioInSetMicLevel(10, 0, 50, 0, 0) == audioInErrorInvalidDevice);

    for (const int validType : {1, 2, 3, 4}) {
        bool threw = false;
        try {
            sceAudioInSetMicLevel(validType, 0, 50, 0, 0);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        Require(threw);
    }

    bool threw = false;
    try {
        sceAudioInSetMicLevel(-1, 0, 50, 0, 0);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw);

    Require(sceAudioInSetMicrophoneLevel(1, 0, 101, 0) == audioInErrorInvalidParam);
    Require(sceAudioInSetMicrophoneLevel(0, 0, 101, 0) == audioInErrorInvalidParam);
    Require(sceAudioInSetMicrophoneLevel(0, 0, 0, 0) == audioInErrorInvalidDevice);
    Require(sceAudioInSetMicrophoneLevel(5, 0, 100, 0) == audioInErrorInvalidDevice);

    for (const int validType : {1, 2, 3, 4}) {
        threw = false;
        try {
            sceAudioInSetMicrophoneLevel(validType, 0, 50, 0);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        Require(threw);
    }

    threw = false;
    try {
        sceAudioInSetMicrophoneLevel(-1, 0, 50, 0);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw);

    Require(sceAudioInGetGain(0x30000000, nullptr) == audioInErrorInvalidPointer);
    Require(sceAudioInGetGain(-1, nullptr) == audioInErrorInvalidPointer);

    float gain = 0.0f;
    Require(sceAudioInGetGain(0x30000000, &gain) == audioInErrorInvalidState && gain == 1.0f);
    gain = 0.0f;
    Require(sceAudioInGetGain(-1, &gain) == audioInErrorInvalidHandle && gain == 1.0f);
    gain = 0.0f;
    Require(sceAudioInGetGain(0, &gain) == audioInErrorInvalidHandle && gain == 1.0f);

    const int handle = sceAudioInOpen(userId, 0, 0, 128, 48000, 0);
    Require(handle > 0);
    gain = 0.0f;
    Require(sceAudioInGetGain(handle, &gain) == 0 && gain == 1.0f);
    Require(sceAudioInClose(handle) == 0);

    threw = false;
    try {
        sceAudioInSetUsbGain();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw);
}

void TestDeviceAndUserMuteToggles() {
    for (const int badType : {0, 5, 8, 9, 10, 100}) {
        Require(sceAudioInSetDeviceMuteStatus(badType, 0, 1) == audioInErrorInvalidDevice);
        Require(sceAudioInSetUserMute(badType, 0, 1) == audioInErrorInvalidDevice);
    }

    for (const int validType : {1, 2, 3, 4, 6, 7}) {
        bool threw = false;
        try {
            sceAudioInSetDeviceMuteStatus(validType, 0, 1);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        Require(threw);

        threw = false;
        try {
            sceAudioInSetUserMute(validType, 0, 1);
        } catch (const std::runtime_error&) {
            threw = true;
        }
        Require(threw);
    }

    bool threw = false;
    try {
        sceAudioInSetDeviceMuteStatus(static_cast<int>(0x80000001u), 0, 1);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw);

    threw = false;
    try {
        sceAudioInSetUserMute(static_cast<int>(0x80000002u), 0, 0);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw);
}

void TestSparkleAndNoiseSuppressionDsp() {
    Require(sceAudioInSetCompressorPreGain() == 0);
    Require(sceAudioInSetNoiseGateThreshold() == 0);

    bool threw = false;
    try {
        sceAudioInSetSparkParam();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw);

    threw = false;
    try {
        sceAudioInSetSparkSideTone();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw);

    threw = false;
    try {
        sceAudioInExtSetAecMode();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw);

    threw = false;
    try {
        sceAudioInSetDevConnection();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw);

    threw = false;
    try {
        sceAudioInSetConnections();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw);

    threw = false;
    try {
        sceAudioInSetPortConnections();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw);

    threw = false;
    try {
        sceAudioInSetPortStatuses();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw);

    threw = false;
    try {
        sceAudioInSetConnectionsForUser();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw);

    threw = false;
    try {
        sceAudioInSetFocusForUser();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw);

    threw = false;
    try {
        sceAudioInExtClose();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw);

    threw = false;
    try {
        sceAudioInExtInput();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw);

    threw = false;
    try {
        sceAudioInExtOpen();
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw);
}

void TestPortOpenVariationsAndStatus() {
    Require(sceAudioInHqOpen(userId, 0, 0, 128, 16000, 2) == audioInErrorInvalidFreq);
    Require(sceAudioInHqOpen(userId, 0, 0, 256, 48000, 2) == audioInErrorInvalidSize);
    Require(sceAudioInHqOpen(userId, 0, 0, 128, 48000, 0) == audioInErrorInvalidParam);
    Require(sceAudioInHqOpen(userId, 0, 0, 128, 48000, 1) == audioInErrorInvalidParam);
    Require(sceAudioInHqOpen(userId, 6, 0, 128, 48000, 2) == audioInErrorInvalidType);
    Require(sceAudioInHqOpen(userId, 0, 1, 128, 48000, 2) == audioInErrorInvalidParam);

    const int hqHandle = sceAudioInHqOpen(userId, 0, 0, 128, 48000, 2);
    Require(hqHandle > 0);
    Require(sceAudioInIsPadEmbeddedMic(hqHandle) == 0);
    Require(sceAudioInGetRerouteCount(hqHandle) == 0);
    Require(sceAudioInClose(hqHandle) == 0);

    Require(sceAudioInAsyncOpen(userId, 0, 0, 128, 16000, 0) == audioInErrorInvalidFreq);
    Require(sceAudioInAsyncOpen(userId, 0, 0, 256, 48000, 0) == audioInErrorInvalidSize);

    const int asyncHandle = sceAudioInAsyncOpen(userId, 0, 0, 128, 48000, 0);
    Require(asyncHandle > 0);
    Require(sceAudioInClose(asyncHandle) == 0);

    Require(sceAudioInDeviceIdOpen(userId, 6, 0, 128, 48000, 0) == audioInErrorInvalidType);
    Require(sceAudioInDeviceIdOpen(userId, 0, 0, 128, 24000, 0) == audioInErrorInvalidFreq);
    Require(sceAudioInDeviceIdOpen(userId, 0, 0, 64, 48000, 0) == audioInErrorInvalidSize);

    const int devHandle = sceAudioInDeviceIdOpen(userId, 0, 0, 128, 48000, 0);
    Require(devHandle > 0);
    Require(sceAudioInClose(devHandle) == 0);

    Require(sceAudioInDeviceHqOpen(userId, 0, 128, 24000, 0) == audioInErrorInvalidFreq);
    Require(sceAudioInDeviceHqOpen(userId, 0, 64, 48000, 0) == audioInErrorInvalidSize);

    const int devHqHandle = sceAudioInDeviceHqOpen(userId, 0, 128, 48000, 0);
    Require(devHqHandle > 0);
    Require(sceAudioInClose(devHqHandle) == 0);

    const int openExHandle = sceAudioInOpenEx(userId, 0, 0, 0, 128, 48000, 0);
    Require(openExHandle > 0);
    Require(sceAudioInClose(openExHandle) == 0);

    const int hqOpenExHandle = sceAudioInHqOpenEx(userId, 0, 0, 0, 128, 48000, 0);
    Require(hqOpenExHandle > 0);
    Require(sceAudioInClose(hqOpenExHandle) == 0);

    const int devOpenExHandle = sceAudioInDeviceOpenEx(userId, 0, 0, 0, 128, 48000, 0);
    Require(devOpenExHandle > 0);
    Require(sceAudioInClose(devOpenExHandle) == 0);

    const int devIdHqHandle = sceAudioInDeviceIdHqOpen(userId, 0, 0, 128, 48000, 0);
    Require(devIdHqHandle > 0);
    Require(sceAudioInClose(devIdHqHandle) == 0);

    const int devIdAsyncHandle = sceAudioInDeviceIdAsyncOpen(userId, 0, 0, 128, 48000, 0);
    Require(devIdAsyncHandle > 0);
    Require(sceAudioInClose(devIdAsyncHandle) == 0);

    bool threw = false;
    try {
        sceAudioInCountPorts(0, 0, 0, 0);
    } catch (const std::runtime_error&) {
        threw = true;
    }
    Require(threw);

    Require(sceAudioInIsPadEmbeddedMic(0) == audioInErrorInvalidHandle);
    Require(sceAudioInIsPadEmbeddedMic(-1) == audioInErrorInvalidHandle);
    Require(sceAudioInIsPadEmbeddedMic(0x30000000) == audioInErrorInvalidState);

    Require(sceAudioInGetRerouteCount(0) == audioInErrorInvalidHandle);
    Require(sceAudioInGetRerouteCount(-1) == audioInErrorInvalidHandle);
    Require(sceAudioInGetRerouteCount(0x30000000) == audioInErrorInvalidState);
}

} // namespace

int main() {
    Require(sceAudioInInit() == 0);

    TestMicGainAndLevelBounds();
    TestDeviceAndUserMuteToggles();
    TestSparkleAndNoiseSuppressionDsp();
    TestPortOpenVariationsAndStatus();

    LibcRunShutdown_nid_postfix();
    return 0;
}
