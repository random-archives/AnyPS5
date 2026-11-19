#include "prx/libSceAudioIn/include/AudioIn.hpp"
#include "prx/libSceAudioIn/include/Exports.hpp"
#include "SceTypes.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <stdexcept>

namespace {

constexpr int audioInErrorInvalidHandle = static_cast<int>(0x80260101);
constexpr int audioInErrorInvalidSize = static_cast<int>(0x80260102);
constexpr int audioInErrorInvalidFreq = static_cast<int>(0x80260103);
constexpr int audioInErrorInvalidType = static_cast<int>(0x80260104);
constexpr int audioInErrorInvalidPointer = static_cast<int>(0x80260105);
constexpr int audioInErrorInvalidParam = static_cast<int>(0x80260106);
constexpr int audioInErrorPortFull = static_cast<int>(0x80260107);
constexpr int audioInErrorInvalidState = static_cast<int>(0x80260109);
constexpr int audioInErrorInvalidDevice = static_cast<int>(0x8026010F);
constexpr std::uint32_t handleBase = 0x30000000;
constexpr int userId = 0x10000000;

void Require(bool condition) {
    if (!condition) {
        std::abort();
    }
}

template <typename TFunc>
bool Throws(TFunc&& func) {
    try {
        func();
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

void TestPortConfigurationBounds() {
    Require(sceAudioInInit() == 0);

    for (const std::uint32_t type : {0u, 1u, 2u, 3u, 4u, 5u}) {
        const int h = sceAudioInOpen(userId, type, 0, 128, 48000, 2);
        Require(h >= static_cast<int>(handleBase));
        Require(sceAudioInClose(h) == 0);
    }

    for (const std::uint32_t badType : {6u, 7u, 8u, 10u, 100u, 255u}) {
        Require(sceAudioInOpen(userId, badType, 0, 128, 48000, 2) == audioInErrorInvalidType);
    }

    for (const std::uint32_t badIndex : {1u, 2u, 5u, 100u}) {
        Require(sceAudioInOpen(userId, 0, badIndex, 128, 48000, 2) == audioInErrorInvalidParam);
    }

    for (const std::uint32_t badSize : {0u, 64u, 127u, 129u, 200u, 255u, 257u, 512u, 1024u}) {
        Require(sceAudioInOpen(userId, 0, 0, badSize, 48000, 2) == audioInErrorInvalidSize);
    }

    for (const std::uint32_t badFreq : {0u, 8000u, 11025u, 22050u, 32000u, 44100u, 96000u, 192000u}) {
        Require(sceAudioInOpen(userId, 0, 0, 128, badFreq, 2) == audioInErrorInvalidFreq);
    }

    for (const std::uint32_t validParam : {0u, 1u, 2u, 0x10u, 0x11u, 0x12u}) {
        const int h = sceAudioInOpen(userId, 0, 0, 128, 16000, validParam);
        Require(h >= static_cast<int>(handleBase));
        Require(sceAudioInClose(h) == 0);
    }

    for (const std::uint32_t badChannel : {3u, 4u, 5u, 7u, 8u, 15u, 0x13u, 0x14u, 0x1fu}) {
        Require(sceAudioInOpen(userId, 0, 0, 128, 48000, badChannel) == audioInErrorInvalidParam);
    }

    for (const std::uint32_t badEncoding : {0x20u, 0x30u, 0x40u, 0x50u, 0x80u, 0xf0u, 0x22u, 0x32u, 0x82u}) {
        Require(sceAudioInOpen(userId, 0, 0, 128, 48000, badEncoding) == audioInErrorInvalidParam);
    }

    const int exPort = sceAudioInOpenEx(userId, 0, 0, 999, 256, 16000, 0x12);
    Require(exPort >= static_cast<int>(handleBase));
    Require(sceAudioInClose(exPort) == 0);
    Require(sceAudioInOpenEx(userId, 6, 0, 0, 256, 48000, 2) == audioInErrorInvalidType);
    Require(sceAudioInOpenEx(userId, 0, 1, 0, 256, 48000, 2) == audioInErrorInvalidParam);
    Require(sceAudioInOpenEx(userId, 0, 0, 0, 512, 48000, 2) == audioInErrorInvalidSize);
    Require(sceAudioInOpenEx(userId, 0, 0, 0, 256, 44100, 2) == audioInErrorInvalidFreq);

    Require(sceAudioInHqOpen(userId, 0, 0, 128, 16000, 2) == audioInErrorInvalidFreq);
    Require(sceAudioInHqOpen(userId, 0, 0, 256, 48000, 2) == audioInErrorInvalidSize);
    Require(sceAudioInHqOpen(userId, 0, 0, 128, 48000, 0) == audioInErrorInvalidParam);
    Require(sceAudioInHqOpen(userId, 0, 0, 128, 48000, 1) == audioInErrorInvalidParam);
    Require(sceAudioInHqOpen(userId, 0, 0, 128, 48000, 0x10) == audioInErrorInvalidParam);
    Require(sceAudioInHqOpen(userId, 6, 0, 128, 48000, 2) == audioInErrorInvalidType);
    Require(sceAudioInHqOpen(userId, 0, 1, 128, 48000, 2) == audioInErrorInvalidParam);
    const int hqPort = sceAudioInHqOpen(userId, 0, 0, 128, 48000, 2);
    Require(hqPort >= static_cast<int>(handleBase));
    Require(sceAudioInClose(hqPort) == 0);

    const int hqExPort = sceAudioInHqOpenEx(userId, 0, 0, 55, 256, 48000, 2);
    Require(hqExPort >= static_cast<int>(handleBase));
    Require(sceAudioInClose(hqExPort) == 0);

    Require(sceAudioInAsyncOpen(userId, 0, 0, 128, 16000, 0) == audioInErrorInvalidFreq);
    Require(sceAudioInAsyncOpen(userId, 0, 0, 256, 48000, 0) == audioInErrorInvalidSize);
    Require(sceAudioInAsyncOpen(userId, 6, 0, 128, 48000, 0) == audioInErrorInvalidType);
    Require(sceAudioInAsyncOpen(userId, 0, 1, 128, 48000, 0) == audioInErrorInvalidParam);
    const int asyncPort = sceAudioInAsyncOpen(userId, 0, 0, 128, 48000, 0);
    Require(asyncPort >= static_cast<int>(handleBase));
    Require(sceAudioInClose(asyncPort) == 0);

    Require(sceAudioInDeviceIdOpen(userId, 6, 0, 128, 48000, 0) == audioInErrorInvalidType);
    Require(sceAudioInDeviceIdOpen(userId, 0, 0, 128, 24000, 0) == audioInErrorInvalidFreq);
    Require(sceAudioInDeviceIdOpen(userId, 0, 0, 64, 48000, 0) == audioInErrorInvalidSize);
    const int devPort = sceAudioInDeviceIdOpen(userId, 3, 1234, 128, 48000, 0);
    Require(devPort >= static_cast<int>(handleBase));
    Require(sceAudioInClose(devPort) == 0);

    const int devAsyncPort = sceAudioInDeviceIdAsyncOpen(userId, 2, 5678, 256, 16000, 1);
    Require(devAsyncPort >= static_cast<int>(handleBase));
    Require(sceAudioInClose(devAsyncPort) == 0);

    const int devIdHqPort = sceAudioInDeviceIdHqOpen(userId, 1, 9999, 128, 48000, 2);
    Require(devIdHqPort >= static_cast<int>(handleBase));
    Require(sceAudioInClose(devIdHqPort) == 0);

    Require(sceAudioInDeviceHqOpen(userId, 0, 128, 24000, 0) == audioInErrorInvalidFreq);
    Require(sceAudioInDeviceHqOpen(userId, 0, 64, 48000, 0) == audioInErrorInvalidSize);
    const int devHqPort = sceAudioInDeviceHqOpen(userId, 42, 256, 48000, 0x10);
    Require(devHqPort >= static_cast<int>(handleBase));
    Require(sceAudioInClose(devHqPort) == 0);

    const int devExPort = sceAudioInDeviceOpenEx(userId, 0, 77, 88, 128, 48000, 2);
    Require(devExPort >= static_cast<int>(handleBase));
    Require(sceAudioInClose(devExPort) == 0);
}

void TestPortCapacityAndHandleBounds() {
    std::array<int, 8> handles{};
    for (int i = 0; i < 8; ++i) {
        handles[i] = sceAudioInOpen(userId, 0, 0, 256, 48000, 2);
        Require(handles[i] >= static_cast<int>(handleBase));
    }

    Require(sceAudioInOpen(userId, 0, 0, 256, 48000, 2) == audioInErrorPortFull);
    Require(sceAudioInHqOpen(userId, 0, 0, 128, 48000, 2) == audioInErrorPortFull);
    Require(sceAudioInAsyncOpen(userId, 0, 0, 128, 48000, 2) == audioInErrorPortFull);
    Require(sceAudioInDeviceIdOpen(userId, 0, 0, 128, 48000, 2) == audioInErrorPortFull);

    const int freedHandle = handles[2];
    Require(sceAudioInClose(freedHandle) == 0);
    Require(sceAudioInClose(freedHandle) == audioInErrorInvalidState);

    const int reusedHandle = sceAudioInOpen(userId, 0, 0, 128, 48000, 1);
    Require(reusedHandle == freedHandle);
    handles[2] = reusedHandle;

    for (int i = 0; i < 8; ++i) {
        Require(sceAudioInClose(handles[i]) == 0);
    }

    for (const int badHandle : {-100, -1, 0, 1, 7, 8, static_cast<int>(0x20000000), static_cast<int>(0x30000008), static_cast<int>(0x300000ff)}) {
        Require(sceAudioInClose(badHandle) == audioInErrorInvalidHandle);
    }
}

void TestMicrophoneGainAndLevelBounds() {
    Require(sceAudioInGetGain(static_cast<int>(handleBase), nullptr) == audioInErrorInvalidPointer);
    Require(sceAudioInGetGain(-1, nullptr) == audioInErrorInvalidPointer);

    float gain = -3.5f;
    Require(sceAudioInGetGain(-1, &gain) == audioInErrorInvalidHandle);
    Require(gain == 1.0f);

    gain = -3.5f;
    Require(sceAudioInGetGain(static_cast<int>(handleBase), &gain) == audioInErrorInvalidState);
    Require(gain == 1.0f);

    const int openPort = sceAudioInOpen(userId, 0, 0, 128, 48000, 2);
    Require(openPort >= static_cast<int>(handleBase));
    gain = 0.0f;
    Require(sceAudioInGetGain(openPort, &gain) == 0);
    Require(gain == 1.0f);
    Require(sceAudioInClose(openPort) == 0);

    gain = -10.0f;
    Require(sceAudioInGetGain(openPort, &gain) == audioInErrorInvalidState);
    Require(gain == 1.0f);

    Require(sceAudioInSetMicLevel(1, 0, 101, 0, 0) == audioInErrorInvalidParam);
    Require(sceAudioInSetMicLevel(1, 0, 200, 0, 0) == audioInErrorInvalidParam);
    Require(sceAudioInSetMicLevel(1, 0, -1, 0, 0) == audioInErrorInvalidParam);
    Require(sceAudioInSetMicLevel(0, 0, 101, 0, 0) == audioInErrorInvalidParam);
    Require(sceAudioInSetMicLevel(99, 0, 101, 0, 0) == audioInErrorInvalidParam);

    Require(sceAudioInSetMicLevel(0, 0, 50, 0, 0) == audioInErrorInvalidDevice);
    Require(sceAudioInSetMicLevel(5, 0, 50, 0, 0) == audioInErrorInvalidDevice);
    Require(sceAudioInSetMicLevel(6, 0, 50, 0, 0) == audioInErrorInvalidDevice);
    Require(sceAudioInSetMicLevel(100, 0, 50, 0, 0) == audioInErrorInvalidDevice);

    for (const int validType : {1, 2, 3, 4}) {
        Require(Throws([&] { sceAudioInSetMicLevel(validType, 0, 0, 0, 0); }));
        Require(Throws([&] { sceAudioInSetMicLevel(validType, 0, 50, 0, 0); }));
        Require(Throws([&] { sceAudioInSetMicLevel(validType, 0, 100, 0, 0); }));
    }
    Require(Throws([&] { sceAudioInSetMicLevel(-1, 0, 50, 0, 0); }));
    Require(Throws([&] { sceAudioInSetMicLevel(-10, 0, 100, 0, 0); }));

    Require(sceAudioInSetMicrophoneLevel(1, 0, 101, 0) == audioInErrorInvalidParam);
    Require(sceAudioInSetMicrophoneLevel(1, 0, -5, 0) == audioInErrorInvalidParam);
    Require(sceAudioInSetMicrophoneLevel(0, 0, 101, 0) == audioInErrorInvalidParam);
    Require(sceAudioInSetMicrophoneLevel(0, 0, 50, 0) == audioInErrorInvalidDevice);
    Require(sceAudioInSetMicrophoneLevel(5, 0, 50, 0) == audioInErrorInvalidDevice);
    Require(sceAudioInSetMicrophoneLevel(7, 0, 0, 0) == audioInErrorInvalidDevice);

    for (const int validType : {1, 2, 3, 4}) {
        Require(Throws([&] { sceAudioInSetMicrophoneLevel(validType, 0, 50, 0); }));
    }
    Require(Throws([&] { sceAudioInSetMicrophoneLevel(-1, 0, 50, 0); }));

    Require(Throws([&] { sceAudioInSetUsbGain(); }));
}

void TestStatusAndMuteBoundaries() {
    Require(sceAudioInGetSilentState(0) == audioInErrorInvalidHandle);
    Require(sceAudioInGetSilentState(-1) == audioInErrorInvalidHandle);
    Require(sceAudioInGetSilentState(static_cast<int>(0x30000008)) == audioInErrorInvalidHandle);
    Require(sceAudioInGetSilentState(static_cast<int>(handleBase)) == audioInErrorInvalidState);

    const int silPort = sceAudioInOpen(userId, 0, 0, 128, 48000, 2);
    Require(silPort >= static_cast<int>(handleBase));
    const int silent = sceAudioInGetSilentState(silPort);
    Require(silent == 0 || silent == 1);
    Require(sceAudioInClose(silPort) == 0);
    Require(sceAudioInGetSilentState(silPort) == audioInErrorInvalidState);

    Require(sceAudioInGetRerouteCount(0) == audioInErrorInvalidHandle);
    Require(sceAudioInGetRerouteCount(-1) == audioInErrorInvalidHandle);
    Require(sceAudioInGetRerouteCount(static_cast<int>(0x30000008)) == audioInErrorInvalidHandle);
    Require(sceAudioInGetRerouteCount(static_cast<int>(handleBase)) == audioInErrorInvalidState);

    const int rcPort = sceAudioInOpen(userId, 0, 0, 128, 48000, 2);
    Require(rcPort >= static_cast<int>(handleBase));
    Require(sceAudioInGetRerouteCount(rcPort) == 0);
    Require(sceAudioInClose(rcPort) == 0);
    Require(sceAudioInGetRerouteCount(rcPort) == audioInErrorInvalidState);

    Require(sceAudioInIsPadEmbeddedMic(0) == audioInErrorInvalidHandle);
    Require(sceAudioInIsPadEmbeddedMic(-1) == audioInErrorInvalidHandle);
    Require(sceAudioInIsPadEmbeddedMic(static_cast<int>(0x30000008)) == audioInErrorInvalidHandle);
    Require(sceAudioInIsPadEmbeddedMic(static_cast<int>(handleBase)) == audioInErrorInvalidState);

    const int micPort = sceAudioInOpen(userId, 0, 0, 128, 48000, 2);
    Require(micPort >= static_cast<int>(handleBase));
    Require(sceAudioInIsPadEmbeddedMic(micPort) == 0);
    Require(sceAudioInClose(micPort) == 0);
    Require(sceAudioInIsPadEmbeddedMic(micPort) == audioInErrorInvalidState);

    Require(sceAudioInIsSharedDevice(8) == audioInErrorInvalidHandle);
    Require(sceAudioInIsSharedDevice(9) == audioInErrorInvalidHandle);
    Require(sceAudioInIsSharedDevice(-1) == audioInErrorInvalidHandle);
    Require(sceAudioInIsSharedDevice(static_cast<int>(0x30000008)) == audioInErrorInvalidHandle);
    Require(sceAudioInIsSharedDevice(0) == audioInErrorInvalidState);
    Require(sceAudioInIsSharedDevice(1) == audioInErrorInvalidState);

    const int shPort = sceAudioInOpen(userId, 0, 0, 128, 48000, 2);
    Require(shPort >= static_cast<int>(handleBase));
    Require(Throws([&] { sceAudioInIsSharedDevice(shPort); }));
    Require(sceAudioInClose(shPort) == 0);
    Require(sceAudioInIsSharedDevice(shPort) == audioInErrorInvalidState);

    Require(sceAudioInGetHandleStatusInfo(0, nullptr) == audioInErrorInvalidHandle);
    Require(sceAudioInGetHandleStatusInfo(-1, nullptr) == audioInErrorInvalidHandle);
    Require(sceAudioInGetHandleStatusInfo(static_cast<int>(0x30000008), nullptr) == audioInErrorInvalidHandle);
    Require(sceAudioInGetHandleStatusInfo(static_cast<int>(handleBase), nullptr) == audioInErrorInvalidState);

    const int stPort = sceAudioInOpen(userId, 0, 0, 128, 48000, 2);
    Require(stPort >= static_cast<int>(handleBase));
    Require(Throws([&] { sceAudioInGetHandleStatusInfo(stPort, nullptr); }));
    Require(sceAudioInClose(stPort) == 0);
    Require(sceAudioInGetHandleStatusInfo(stPort, nullptr) == audioInErrorInvalidState);

    for (const int badMuteType : {0, 5, 8, 9, 10, 20}) {
        Require(sceAudioInSetDeviceMuteStatus(badMuteType, 0, 0) == audioInErrorInvalidDevice);
        Require(sceAudioInSetDeviceMuteStatus(badMuteType, 0, 1) == audioInErrorInvalidDevice);
    }
    for (const int validMuteType : {1, 2, 3, 4, 6, 7}) {
        Require(Throws([&] { sceAudioInSetDeviceMuteStatus(validMuteType, 0, 0); }));
        Require(Throws([&] { sceAudioInSetDeviceMuteStatus(validMuteType, 0, 1); }));
    }

    for (const int badMuteType : {0, 5, 8, 9, 10}) {
        Require(sceAudioInSetUserMute(badMuteType, 0, 0) == audioInErrorInvalidDevice);
    }
    for (const int validMuteType : {1, 2, 3, 4, 6, 7}) {
        Require(Throws([&] { sceAudioInSetUserMute(validMuteType, 0, 0); }));
    }

    Require(Throws([&] { sceAudioInSetMode(0, 1); }));
    Require(Throws([&] { sceAudioInSetMode(1, 0); }));
    Require(Throws([&] { sceAudioInSetMode2(0, 1, nullptr); }));
    Require(Throws([&] { sceAudioInSetMode2(1, 0, nullptr); }));

    Require(Throws([&] { sceAudioInCountPorts(0, 0, 0, 0); }));

    Require(sceAudioInSetCompressorPreGain() == 0);
    Require(sceAudioInSetNoiseGateThreshold() == 0);

    Require(sceAudioInVmicCreate(userId, 512, 48000, 0) == audioInErrorInvalidSize);
    Require(sceAudioInVmicCreate(userId, 128, 44100, 0) == audioInErrorInvalidFreq);
    Require(Throws([&] { sceAudioInVmicCreate(userId, 128, 48000, 0); }));
    Require(Throws([&] { sceAudioInVmicDestroy(); }));
    Require(Throws([&] { sceAudioInVmicWrite(); }));
    Require(Throws([&] { sceAudioInVmicGetStatus(); }));
    Require(Throws([&] { sceAudioInVmicSetMute(); }));

    Require(Throws([&] { sceAudioInSetConnections(); }));
    Require(Throws([&] { sceAudioInSetPortConnections(); }));
    Require(Throws([&] { sceAudioInSetPortStatuses(); }));
    Require(Throws([&] { sceAudioInChangeAppModuleState(); }));
    Require(Throws([&] { sceAudioInSetSparkParam(); }));
    Require(Throws([&] { sceAudioInSetSparkSideTone(); }));
    Require(Throws([&] { sceAudioInSetDevConnection(); }));
    Require(Throws([&] { sceAudioInExtCtrl(); }));
    Require(Throws([&] { sceAudioInInputs(); }));
    Require(Throws([&] { sceAudioInExtClose(); }));
    Require(Throws([&] { sceAudioInExtInput(); }));
    Require(Throws([&] { sceAudioInExtOpen(); }));
    Require(Throws([&] { sceAudioInExtSetAecMode(); }));
    Require(Throws([&] { sceAudioInSetConnectionsForUser(); }));
    Require(Throws([&] { sceAudioInSetFocusForUser(); }));
}

} // namespace

int main() {
    TestPortConfigurationBounds();
    TestPortCapacityAndHandleBounds();
    TestMicrophoneGainAndLevelBounds();
    TestStatusAndMuteBoundaries();

    LibcRunShutdown_nid_postfix();
    return 0;
}
