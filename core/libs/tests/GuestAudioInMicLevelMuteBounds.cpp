#include "prx/libSceAudioIn/include/AudioIn.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include <cstdint>
#include <cstdlib>
#include <stdexcept>

namespace {

constexpr int errorInvalidParam = static_cast<int>(0x80260106);
constexpr int errorInvalidDevice = static_cast<int>(0x8026010F);

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

static void TestSetMicLevelBounds() {
    for (int level : {-100, -1, 101, 102, 150, 200, 1000}) {
        Require(sceAudioInSetMicLevel(1, 0, level, 0, 0) == errorInvalidParam);
        Require(sceAudioInSetMicrophoneLevel(1, 0, level, 0) == errorInvalidParam);
    }

    for (int invalidType : {0, 5, 6, 7, 8, 10, 100}) {
        Require(sceAudioInSetMicLevel(invalidType, 0, 101, 0, 0) == errorInvalidParam);
        Require(sceAudioInSetMicrophoneLevel(invalidType, 0, 101, 0) == errorInvalidParam);
        Require(sceAudioInSetMicLevel(invalidType, 0, -5, 0, 0) == errorInvalidParam);
        Require(sceAudioInSetMicrophoneLevel(invalidType, 0, -5, 0) == errorInvalidParam);
    }

    for (int invalidType : {0, 5, 6, 7, 8, 10, 100}) {
        Require(sceAudioInSetMicLevel(invalidType, 0, 50, 0, 0) == errorInvalidDevice);
        Require(sceAudioInSetMicrophoneLevel(invalidType, 0, 50, 0) == errorInvalidDevice);
        Require(sceAudioInSetMicLevel(invalidType, 0, 0, 0, 0) == errorInvalidDevice);
        Require(sceAudioInSetMicrophoneLevel(invalidType, 0, 0, 0) == errorInvalidDevice);
        Require(sceAudioInSetMicLevel(invalidType, 0, 100, 0, 0) == errorInvalidDevice);
        Require(sceAudioInSetMicrophoneLevel(invalidType, 0, 100, 0) == errorInvalidDevice);
    }

    for (int validType : {1, 2, 3, 4}) {
        for (int validLevel : {0, 1, 50, 99, 100}) {
            Require(ThrowsRuntimeError([validType, validLevel] {
                static_cast<void>(sceAudioInSetMicLevel(validType, 0, validLevel, 0, 0));
            }));
            Require(ThrowsRuntimeError([validType, validLevel] {
                static_cast<void>(sceAudioInSetMicrophoneLevel(validType, 0, validLevel, 0));
            }));
        }
    }

    for (int rawType : {-1, -2, -10}) {
        for (int validLevel : {0, 50, 100}) {
            Require(ThrowsRuntimeError([rawType, validLevel] {
                static_cast<void>(sceAudioInSetMicLevel(rawType, 0, validLevel, 0, 0));
            }));
            Require(ThrowsRuntimeError([rawType, validLevel] {
                static_cast<void>(sceAudioInSetMicrophoneLevel(rawType, 0, validLevel, 0));
            }));
        }
    }
}

static void TestSetMuteBounds() {
    for (int validType : {1, 2, 3, 4, 6, 7}) {
        for (int mute : {0, 1, 2}) {
            Require(ThrowsRuntimeError([validType, mute] {
                static_cast<void>(sceAudioInSetUserMute(validType, 0, mute));
            }));
            Require(ThrowsRuntimeError([validType, mute] {
                static_cast<void>(sceAudioInSetDeviceMuteStatus(validType, 0, mute));
            }));
        }
    }

    for (std::uint32_t baseType : {1u, 2u, 3u, 4u, 6u, 7u}) {
        const int maskedType = static_cast<int>(0x80000000u | baseType);
        Require(ThrowsRuntimeError([maskedType] {
            static_cast<void>(sceAudioInSetUserMute(maskedType, 0, 1));
        }));
        Require(ThrowsRuntimeError([maskedType] {
            static_cast<void>(sceAudioInSetDeviceMuteStatus(maskedType, 0, 1));
        }));
    }

    for (int invalidType : {0, 5, 8, 9, 10, 100}) {
        Require(sceAudioInSetUserMute(invalidType, 0, 1) == errorInvalidDevice);
        Require(sceAudioInSetDeviceMuteStatus(invalidType, 0, 1) == errorInvalidDevice);
        Require(sceAudioInSetUserMute(invalidType, 0, 0) == errorInvalidDevice);
        Require(sceAudioInSetDeviceMuteStatus(invalidType, 0, 0) == errorInvalidDevice);
    }

    Require(sceAudioInSetUserMute(-1, 0, 1) == errorInvalidDevice);
    Require(sceAudioInSetDeviceMuteStatus(-1, 0, 1) == errorInvalidDevice);

    const int maskedFive = static_cast<int>(0x80000000u | 5u);
    Require(sceAudioInSetUserMute(maskedFive, 0, 1) == errorInvalidDevice);
    Require(sceAudioInSetDeviceMuteStatus(maskedFive, 0, 1) == errorInvalidDevice);

    const int maskedZero = static_cast<int>(0x80000000u);
    Require(sceAudioInSetUserMute(maskedZero, 0, 1) == errorInvalidDevice);
    Require(sceAudioInSetDeviceMuteStatus(maskedZero, 0, 1) == errorInvalidDevice);
}

} // namespace

int main() {
    Require(sceAudioInInit() == 0);

    TestSetMicLevelBounds();
    TestSetMuteBounds();

    LibcRunShutdown_nid_postfix();
    return 0;
}
