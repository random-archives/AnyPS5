#include "SceTypes.hpp"
#include "prx/libScePad/include/Pad.hpp"
#include "prx/libScePad/include/PadState.hpp"
#include <array>
#include <cstdlib>
#include <initializer_list>

namespace {

constexpr int PAD_ERROR_INVALID_ARG = static_cast<int>(0x80920001u);
constexpr int PAD_ERROR_INVALID_HANDLE = static_cast<int>(0x80920003u);
constexpr int PAD_ERROR_NOT_INITIALIZED = static_cast<int>(0x80920005u);

void Require(bool condition) {
    if (!condition) {
        std::abort();
    }
}

void TestPreInitializationBounds() {
    PadTriggerEffectStateInformation info{};

    for (int handle : {-1000, -100, -2, -1, 0, 1, 2, 3, 4, 10, 32, 33, 100, PAD_REMOTE_HANDLE, PAD_SPECIAL_HANDLE, 9999}) {
        Require(scePadGetTriggerEffectState(handle, nullptr) == PAD_ERROR_NOT_INITIALIZED);
        Require(scePadGetTriggerEffectState(handle, &info) == PAD_ERROR_NOT_INITIALIZED);
    }

    bool caught = false;
    try {
        scePadVrControllerGetTriggerEffectState(1, 2, &info);
    } catch (...) {
        caught = true;
    }
    Require(caught);

    caught = false;
    try {
        scePadVrControllerGetTriggerEffectStates(1, 2, &info);
    } catch (...) {
        caught = true;
    }
    Require(caught);

    caught = false;
    try {
        scePadGetTriggerEffectInternalState(1, nullptr);
    } catch (...) {
        caught = true;
    }
    Require(caught);
}

void TestPostInitializationUnopenedHandleBounds() {
    Require(scePadInit_nid_postfix() == PAD_OK);

    PadTriggerEffectStateInformation info{};

    for (int handle : {-1000, -100, -2, -1, 0, 1, 2, 3, 4, 10, 32, 33, 100, PAD_REMOTE_HANDLE, PAD_SPECIAL_HANDLE, 9999}) {
        Require(scePadGetTriggerEffectState(handle, nullptr) == PAD_ERROR_INVALID_HANDLE);
        Require(scePadGetTriggerEffectState(handle, &info) == PAD_ERROR_INVALID_HANDLE);
    }
}

void TestStandardHandleTriggerEffectStateAndNibbles() {
    const int handle0 = scePadOpen_nid_postfix(0x10000000, PAD_PORT_TYPE_STANDARD, 0, nullptr);
    Require(handle0 == 1);

    Require(scePadGetTriggerEffectState(handle0, nullptr) == PAD_ERROR_INVALID_ARG);

    PadTriggerEffectStateInformation info{};
    Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
    Require(info.state[0] == 0 && info.state[1] == 0);

    PadPublishTriggerStatus_nid_postfix(0, 1, 0, 1, 0);
    Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
    Require(info.state[0] == 1 && info.state[1] == 1);

    PadPublishTriggerStatus_nid_postfix(0, 1, 1, 1, 1);
    Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
    Require(info.state[0] == 2 && info.state[1] == 2);

    PadPublishTriggerStatus_nid_postfix(0, 1, 2, 1, 2);
    Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
    Require(info.state[0] == 0 && info.state[1] == 0);

    PadPublishTriggerStatus_nid_postfix(0, 1, 255, 1, 255);
    Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
    Require(info.state[0] == 0 && info.state[1] == 0);

    PadPublishTriggerStatus_nid_postfix(0, 2, 0, 2, 0);
    Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
    Require(info.state[0] == 3 && info.state[1] == 3);

    PadPublishTriggerStatus_nid_postfix(0, 2, 1, 2, 1);
    Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
    Require(info.state[0] == 4 && info.state[1] == 4);

    PadPublishTriggerStatus_nid_postfix(0, 2, 2, 2, 2);
    Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
    Require(info.state[0] == 5 && info.state[1] == 5);

    PadPublishTriggerStatus_nid_postfix(0, 2, 3, 2, 3);
    Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
    Require(info.state[0] == 0 && info.state[1] == 0);

    PadPublishTriggerStatus_nid_postfix(0, 3, 0, 3, 0);
    Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
    Require(info.state[0] == 6 && info.state[1] == 6);

    PadPublishTriggerStatus_nid_postfix(0, 3, 1, 3, 1);
    Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
    Require(info.state[0] == 7 && info.state[1] == 7);

    PadPublishTriggerStatus_nid_postfix(0, 3, 2, 3, 2);
    Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
    Require(info.state[0] == 0 && info.state[1] == 0);

    for (std::uint8_t effect : {0u, 4u, 5u, 10u, 255u}) {
        PadPublishTriggerStatus_nid_postfix(0, effect, 0, effect, 1);
        Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
        Require(info.state[0] == 0 && info.state[1] == 0);
    }

    PadPublishTriggerStatus_nid_postfix(0, 1, 0, 2, 1);
    Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
    Require(info.state[0] == 1 && info.state[1] == 4);

    PadPublishTriggerStatus_nid_postfix(0, 3, 1, 2, 2);
    Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
    Require(info.state[0] == 7 && info.state[1] == 5);

    PadPublishTriggerStatus_nid_postfix(0, 2, 0, 3, 0);
    Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
    Require(info.state[0] == 3 && info.state[1] == 6);
}

void TestDisconnectAndClearStateReset() {
    const int handle0 = 1;
    PadTriggerEffectStateInformation info{};

    PadPublishTriggerStatus_nid_postfix(0, 3, 1, 2, 2);
    Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
    Require(info.state[0] == 7 && info.state[1] == 5);

    PadClearTriggerStatus_nid_postfix(0);
    Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
    Require(info.state[0] == 0 && info.state[1] == 0);

    PadPublishTriggerStatus_nid_postfix(0, 1, 0, 2, 1);
    Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
    Require(info.state[0] == 1 && info.state[1] == 4);

    PadSetPortConnected_nid_postfix(0, false);
    Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
    Require(info.state[0] == 1 && info.state[1] == 4);

    PadSetPortConnected_nid_postfix(0, true);
    PadClearTriggerStatus_nid_postfix(0);
    Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
    Require(info.state[0] == 0 && info.state[1] == 0);
}

void TestMultiControllerPortIsolation() {
    const int handle0 = 1;
    const int handle1 = scePadOpen_nid_postfix(0x10000001, PAD_PORT_TYPE_STANDARD, 0, nullptr);
    Require(handle1 == 2);

    PadPublishTriggerStatus_nid_postfix(0, 1, 0, 2, 1);
    PadPublishTriggerStatus_nid_postfix(1, 3, 0, 2, 2);

    PadTriggerEffectStateInformation info0{};
    PadTriggerEffectStateInformation info1{};

    Require(scePadGetTriggerEffectState(handle0, &info0) == PAD_OK);
    Require(info0.state[0] == 1 && info0.state[1] == 4);

    Require(scePadGetTriggerEffectState(handle1, &info1) == PAD_OK);
    Require(info1.state[0] == 6 && info1.state[1] == 5);

    PadClearTriggerStatus_nid_postfix(0);
    Require(scePadGetTriggerEffectState(handle0, &info0) == PAD_OK);
    Require(info0.state[0] == 0 && info0.state[1] == 0);

    Require(scePadGetTriggerEffectState(handle1, &info1) == PAD_OK);
    Require(info1.state[0] == 6 && info1.state[1] == 5);

    PadClearTriggerStatus_nid_postfix(1);
    Require(scePadGetTriggerEffectState(handle1, &info1) == PAD_OK);
    Require(info1.state[0] == 0 && info1.state[1] == 0);
}

void TestSpecialAndRemotePortBehavior() {
    const int special = scePadOpen_nid_postfix(0x10000000, PAD_PORT_TYPE_SPECIAL, 0, nullptr);
    Require(special == PAD_SPECIAL_HANDLE);

    PadTriggerEffectStateInformation specialInfo{99u, 99u};
    Require(scePadGetTriggerEffectState(special, nullptr) == PAD_ERROR_INVALID_ARG);
    Require(scePadGetTriggerEffectState(special, &specialInfo) == PAD_OK);
    Require(specialInfo.state[0] == 0 && specialInfo.state[1] == 0);

    Require(scePadClose_nid_postfix(special) == PAD_OK);
    Require(scePadGetTriggerEffectState(special, nullptr) == PAD_ERROR_INVALID_HANDLE);
    Require(scePadGetTriggerEffectState(special, &specialInfo) == PAD_ERROR_INVALID_HANDLE);

    const int remote = scePadOpen_nid_postfix(PAD_USER_ID_SYSTEM, PAD_PORT_TYPE_REMOTE, 0, nullptr);
    Require(remote == PAD_REMOTE_HANDLE);

    PadTriggerEffectStateInformation remoteInfo{};
    Require(scePadGetTriggerEffectState(remote, nullptr) == PAD_ERROR_INVALID_HANDLE);
    Require(scePadGetTriggerEffectState(remote, &remoteInfo) == PAD_ERROR_INVALID_HANDLE);

    Require(scePadClose_nid_postfix(remote) == PAD_OK);
    Require(scePadGetTriggerEffectState(remote, nullptr) == PAD_ERROR_INVALID_HANDLE);
    Require(scePadGetTriggerEffectState(remote, &remoteInfo) == PAD_ERROR_INVALID_HANDLE);
}

void TestClosedStandardHandleBehavior() {
    const int handle0 = 1;
    const int handle1 = 2;

    PadTriggerEffectStateInformation info{};

    Require(scePadClose_nid_postfix(handle1) == PAD_OK);
    Require(scePadGetTriggerEffectState(handle1, nullptr) == PAD_ERROR_INVALID_ARG);
    Require(scePadGetTriggerEffectState(handle1, &info) == PAD_OK);

    for (int invalid : {-100, -1, 0, 3, 4, 10, 32, 33, 9999}) {
        Require(scePadGetTriggerEffectState(invalid, nullptr) == PAD_ERROR_INVALID_HANDLE);
        Require(scePadGetTriggerEffectState(invalid, &info) == PAD_ERROR_INVALID_HANDLE);
    }

    Require(scePadClose_nid_postfix(handle0) == PAD_OK);
    Require(scePadGetTriggerEffectState(handle0, nullptr) == PAD_ERROR_INVALID_ARG);
    Require(scePadGetTriggerEffectState(handle0, &info) == PAD_OK);
}

void TestRelatedTriggerStubsThrow() {
    PadTriggerEffectStateInformation info{};

    bool caught = false;
    try {
        scePadVrControllerGetTriggerEffectState(1, 2, &info);
    } catch (...) {
        caught = true;
    }
    Require(caught);

    caught = false;
    try {
        scePadVrControllerGetTriggerEffectStates(1, 2, &info);
    } catch (...) {
        caught = true;
    }
    Require(caught);

    caught = false;
    try {
        scePadGetTriggerEffectInternalState(1, nullptr);
    } catch (...) {
        caught = true;
    }
    Require(caught);
}

} // namespace

int main() {
    TestPreInitializationBounds();
    TestPostInitializationUnopenedHandleBounds();
    TestStandardHandleTriggerEffectStateAndNibbles();
    TestDisconnectAndClearStateReset();
    TestMultiControllerPortIsolation();
    TestSpecialAndRemotePortBehavior();
    TestClosedStandardHandleBehavior();
    TestRelatedTriggerStubsThrow();
    return 0;
}
