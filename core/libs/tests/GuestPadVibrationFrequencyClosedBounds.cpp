#include "SceTypes.hpp"
#include "prx/libScePad/include/Pad.hpp"
#include "prx/libScePad/include/PadState.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <source_location>

static_assert(sizeof(PadVibrationParam) == 2);
static_assert(offsetof(PadVibrationParam, large_motor) == 0);
static_assert(offsetof(PadVibrationParam, small_motor) == 1);

static void Require(bool condition, const std::source_location location = std::source_location::current()) {
    if (!condition) {
        std::fprintf(stderr, "Require failed at line %u\n", static_cast<unsigned>(location.line()));
        std::fflush(stderr);
        std::abort();
    }
}

int main() {
    constexpr int errorNotInitialized = static_cast<int>(0x80920005u);
    constexpr int errorInvalidArg = -2137915391;
    constexpr int errorInvalidHandle = -2137915389;

    const PadVibrationParam initialVib{10, 20};
    for (int handle : {-9999, -1, 0, 1, 2, 0x10, 0x20, 9999}) {
        Require(scePadSetVibration(handle, &initialVib) == errorInvalidHandle);
        Require(scePadSetVibration(handle, nullptr) == errorInvalidHandle);
        Require(scePadDisableVibration(handle, true) == errorInvalidHandle);
        Require(scePadDisableVibration(handle, false) == errorInvalidHandle);
        Require(scePadSetVibrationMode(handle, 1) == errorNotInitialized);
        Require(scePadSetVibrationMode(handle, 0) == errorNotInitialized);
        Require(scePadSetVibrationMode(handle, -1) == errorNotInitialized);
        Require(scePadSetVibrationForce(handle, &initialVib) == errorNotInitialized);
        Require(scePadSetVibrationForce(handle, nullptr) == errorNotInitialized);
        Require(scePadSetPS4BcVibrationMode(0) == errorNotInitialized);
    }

    Require(scePadSetVibrationTriggerEffectWeakWhileEmbeddedMicInUse(true) == PAD_OK);
    Require(scePadSetVibrationTriggerEffectWeakWhileEmbeddedMicInUse(false) == PAD_OK);

    Require(scePadInit_nid_postfix() == PAD_OK);

    for (int handle : {-9999, -100, -2, -1, 0, 5, 9999}) {
        Require(scePadSetVibration(handle, &initialVib) == errorInvalidHandle);
        Require(scePadSetVibration(handle, nullptr) == errorInvalidHandle);
        Require(scePadDisableVibration(handle, true) == errorInvalidHandle);
        Require(scePadDisableVibration(handle, false) == errorInvalidHandle);
        Require(scePadSetVibrationMode(handle, 0) == errorInvalidArg);
        Require(scePadSetVibrationMode(handle, -1) == errorInvalidArg);
        Require(scePadSetVibrationMode(handle, 1) == errorInvalidHandle);
        Require(scePadSetVibrationMode(handle, 2) == errorInvalidHandle);
        Require(scePadSetVibrationMode(handle, 3) == errorInvalidHandle);
        Require(scePadSetVibrationMode(handle, 4) == errorInvalidHandle);
        Require(scePadSetVibrationMode(handle, 5) == errorInvalidHandle);
        Require(scePadSetVibrationMode(handle, 6) == errorInvalidHandle);
        Require(scePadSetVibrationForce(handle, nullptr) == errorInvalidArg);
    }

    const int special = scePadOpen_nid_postfix(0x10000000, PAD_PORT_TYPE_SPECIAL, 0, nullptr);
    Require(special > 0);

    Require(scePadSetVibration(special, nullptr) == errorInvalidArg);
    Require(scePadSetVibration(special, &initialVib) == PAD_OK);
    Require(scePadDisableVibration(special, true) == PAD_OK);
    Require(scePadDisableVibration(special, false) == PAD_OK);
    Require(scePadSetVibrationMode(special, 0) == errorInvalidArg);
    Require(scePadSetVibrationMode(special, -1) == errorInvalidArg);
    Require(scePadSetVibrationMode(special, 1) == PAD_OK);
    Require(scePadSetVibrationMode(special, 2) == PAD_OK);
    Require(scePadSetVibrationMode(special, 3) == PAD_OK);
    Require(scePadSetVibrationMode(special, 4) == PAD_OK);
    Require(scePadSetVibrationMode(special, 5) == PAD_OK);
    Require(scePadSetVibrationMode(special, 6) == errorInvalidArg);
    Require(scePadSetVibrationForce(special, nullptr) == errorInvalidArg);
    Require(scePadSetVibrationForce(special, &initialVib) == PAD_OK);

    Require(scePadClose_nid_postfix(special) == PAD_OK);
    Require(scePadClose_nid_postfix(special) == errorInvalidHandle);

    Require(scePadSetVibration(special, &initialVib) == errorInvalidHandle);
    Require(scePadSetVibration(special, nullptr) == errorInvalidHandle);
    Require(scePadDisableVibration(special, true) == errorInvalidHandle);
    Require(scePadDisableVibration(special, false) == errorInvalidHandle);
    Require(scePadSetVibrationMode(special, 1) == errorInvalidHandle);

    const int standard = scePadOpen_nid_postfix(0x10000000, PAD_PORT_TYPE_STANDARD, 0, nullptr);
    Require(standard > 0);

    Require(scePadSetVibration(standard, nullptr) == errorInvalidArg);

    std::uint32_t seq = 0;
    PadOutputState out{};

    const PadVibrationParam off{0, 0};
    Require(scePadSetVibration(standard, &off) == PAD_OK);
    Require(PadFetchOutput_nid_postfix(&seq, &out));
    Require(out.vibrationLarge == 0 && out.vibrationSmall == 0);

    const PadVibrationParam largeMin{1, 0};
    Require(scePadSetVibration(standard, &largeMin) == PAD_OK);
    Require(PadFetchOutput_nid_postfix(&seq, &out));
    Require(out.vibrationLarge == 1 && out.vibrationSmall == 0);

    const PadVibrationParam largeMid{128, 0};
    Require(scePadSetVibration(standard, &largeMid) == PAD_OK);
    Require(PadFetchOutput_nid_postfix(&seq, &out));
    Require(out.vibrationLarge == 128 && out.vibrationSmall == 0);

    const PadVibrationParam largeMax{255, 0};
    Require(scePadSetVibration(standard, &largeMax) == PAD_OK);
    Require(PadFetchOutput_nid_postfix(&seq, &out));
    Require(out.vibrationLarge == 255 && out.vibrationSmall == 0);

    const PadVibrationParam smallMin{0, 1};
    Require(scePadSetVibration(standard, &smallMin) == PAD_OK);
    Require(PadFetchOutput_nid_postfix(&seq, &out));
    Require(out.vibrationLarge == 0 && out.vibrationSmall == 1);

    const PadVibrationParam smallMid{0, 128};
    Require(scePadSetVibration(standard, &smallMid) == PAD_OK);
    Require(PadFetchOutput_nid_postfix(&seq, &out));
    Require(out.vibrationLarge == 0 && out.vibrationSmall == 128);

    const PadVibrationParam smallMax{0, 255};
    Require(scePadSetVibration(standard, &smallMax) == PAD_OK);
    Require(PadFetchOutput_nid_postfix(&seq, &out));
    Require(out.vibrationLarge == 0 && out.vibrationSmall == 255);

    const PadVibrationParam dualMin{1, 1};
    Require(scePadSetVibration(standard, &dualMin) == PAD_OK);
    Require(PadFetchOutput_nid_postfix(&seq, &out));
    Require(out.vibrationLarge == 1 && out.vibrationSmall == 1);

    const PadVibrationParam dualMax{255, 255};
    Require(scePadSetVibration(standard, &dualMax) == PAD_OK);
    Require(PadFetchOutput_nid_postfix(&seq, &out));
    Require(out.vibrationLarge == 255 && out.vibrationSmall == 255);

    const PadVibrationParam asym1{255, 1};
    Require(scePadSetVibration(standard, &asym1) == PAD_OK);
    Require(PadFetchOutput_nid_postfix(&seq, &out));
    Require(out.vibrationLarge == 255 && out.vibrationSmall == 1);

    const PadVibrationParam asym2{1, 255};
    Require(scePadSetVibration(standard, &asym2) == PAD_OK);
    Require(PadFetchOutput_nid_postfix(&seq, &out));
    Require(out.vibrationLarge == 1 && out.vibrationSmall == 255);

    const PadVibrationParam compound{64, 192};
    Require(scePadSetVibration(standard, &compound) == PAD_OK);
    Require(PadFetchOutput_nid_postfix(&seq, &out));
    Require(out.vibrationLarge == 64 && out.vibrationSmall == 192);

    const std::uint32_t prevSeq = seq;
    Require(scePadSetVibration(standard, &compound) == PAD_OK);
    Require(!PadFetchOutput_nid_postfix(&seq, &out));
    Require(seq == prevSeq);

    Require(scePadDisableVibration(standard, true) == PAD_OK);
    Require(PadFetchOutput_nid_postfix(&seq, &out));
    Require(out.vibrationLarge == 0 && out.vibrationSmall == 0);

    Require(scePadDisableVibration(standard, false) == PAD_OK);
    Require(scePadSetVibration(standard, &compound) == PAD_OK);
    Require(PadFetchOutput_nid_postfix(&seq, &out));
    Require(out.vibrationLarge == 64 && out.vibrationSmall == 192);

    for (int mode : {1, 2, 3, 4, 5}) {
        Require(scePadSetVibrationMode(standard, mode) == PAD_OK);
        Require(PadFetchOutput_nid_postfix(&seq, &out));
        Require(out.vibrationMode == mode);
    }

    const std::uint32_t modeSeq = seq;
    Require(scePadSetVibrationMode(standard, 5) == PAD_OK);
    Require(!PadFetchOutput_nid_postfix(&seq, &out));
    Require(seq == modeSeq);

    for (int invalidMode : {-99, -1, 0, 6, 7, 100}) {
        Require(scePadSetVibrationMode(standard, invalidMode) == errorInvalidArg);
    }

    Require(scePadSetVibrationForce(standard, nullptr) == errorInvalidArg);
    const PadVibrationParam forceParam{77, 88};
    Require(scePadSetVibrationForce(standard, &forceParam) == PAD_OK);
    Require(PadFetchOutput_nid_postfix(&seq, &out));
    Require(out.vibrationLarge == 77 && out.vibrationSmall == 88);

    Require(scePadClose_nid_postfix(standard) == PAD_OK);
    Require(scePadClose_nid_postfix(standard) == errorInvalidHandle);

    std::uint64_t baddr = 0;
    Require(scePadGetBluetoothAddress(standard, &baddr) == errorInvalidHandle);

    Require(scePadDisableVibration(standard, true) == errorInvalidHandle);
    Require(scePadDisableVibration(standard, false) == errorInvalidHandle);
    Require(scePadSetVibration(standard, &initialVib) == errorInvalidHandle);
    Require(scePadSetVibration(standard, nullptr) == errorInvalidHandle);
    Require(scePadSetVibrationMode(standard, 1) == errorInvalidHandle);
    Require(scePadSetVibrationForce(standard, &initialVib) == errorInvalidHandle);

    return 0;
}
