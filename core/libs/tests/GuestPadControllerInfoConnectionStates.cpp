#include "SceTypes.hpp"
#include "prx/libScePad/include/Pad.hpp"
#include "prx/libScePad/include/PadState.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <stdexcept>
#include <string_view>

static_assert(sizeof(PadTouchPadInformation) == 8);
static_assert(offsetof(PadTouchPadInformation, pixelDensity) == 0);
static_assert(offsetof(PadTouchPadInformation, resolution) == 4);

static_assert(sizeof(PadStickInformation) == 2);
static_assert(offsetof(PadStickInformation, deadZoneLeft) == 0);
static_assert(offsetof(PadStickInformation, deadZoneRight) == 1);

static_assert(sizeof(PadControllerInformation) == 28);
static_assert(offsetof(PadControllerInformation, touchPadInfo) == 0);
static_assert(offsetof(PadControllerInformation, stickInfo) == 8);
static_assert(offsetof(PadControllerInformation, connectionType) == 10);
static_assert(offsetof(PadControllerInformation, connectedCount) == 11);
static_assert(offsetof(PadControllerInformation, connected) == 12);
static_assert(offsetof(PadControllerInformation, deviceClass) == 16);
static_assert(offsetof(PadControllerInformation, reserve) == 20);

static void Require(bool condition) {
    if (!condition) {
        std::abort();
    }
}

template <typename F>
static void RequireNotImplemented(const char* expectedName, F&& func) {
    try {
        func();
        std::abort();
    } catch (const std::runtime_error& err) {
        const std::string_view message = err.what();
        const std::string_view expected = expectedName;
        if (message.find(expected) == std::string_view::npos ||
            message.find("not implemented") == std::string_view::npos) {
            std::abort();
        }
    }
}

static void VerifyCanaryPreserved(const PadControllerInformation& info) {
    const auto* raw = reinterpret_cast<const std::uint8_t*>(&info);
    for (std::size_t i = 0; i < sizeof(PadControllerInformation); ++i) {
        Require(raw[i] == 0xAA);
    }
}

static void VerifyUnopenedHandles(const std::initializer_list<int>& handles) {
    for (int handle : handles) {
        PadControllerInformation canary{};
        std::memset(&canary, 0xAA, sizeof(canary));
        Require(scePadGetControllerInformation(handle, &canary) == PAD_ERROR_INVALID_HANDLE);
        VerifyCanaryPreserved(canary);

        Require(scePadGetControllerInformation(handle, nullptr) == PAD_ERROR_INVALID_HANDLE);
    }
}

static void VerifyStandardDualSenseInfo(const PadControllerInformation& info, bool expectedConnected) {
    Require(info.touchPadInfo.pixelDensity == 44.86f);
    Require(info.touchPadInfo.resolution.x == kTouchPadWidth);
    Require(info.touchPadInfo.resolution.y == kTouchPadHeight);
    Require(info.stickInfo.deadZoneLeft == 13);
    Require(info.stickInfo.deadZoneRight == 13);
    Require(info.connectionType == PAD_CONNECTION_TYPE_LOCAL);
    Require(info.deviceClass == PAD_DEVICE_CLASS_STANDARD);
    Require(info.connected == expectedConnected);
    Require(info.connectedCount == (expectedConnected ? 1 : 0));
}

static void VerifySpecialHandleZeroedInfo(const PadControllerInformation& info) {
    Require(info.touchPadInfo.pixelDensity == 0.0f);
    Require(info.touchPadInfo.resolution.x == 0);
    Require(info.touchPadInfo.resolution.y == 0);
    Require(info.stickInfo.deadZoneLeft == 0);
    Require(info.stickInfo.deadZoneRight == 0);
    Require(info.connectionType == 0);
    Require(info.deviceClass == 0);
    Require(info.connected == false);
    Require(info.connectedCount == 0);
}

int main() {
    VerifyUnopenedHandles({-10000, -100, -2, -1, 0, 1, 2, 3, 4, 5, 15, 16, 31, 32, 47, 48, 100, 99999, 0x7fffffff});

    PadControllerInformation uninitInfo{};
    std::memset(&uninitInfo, 0x55, sizeof(uninitInfo));
    Require(scePadGetControllerInformation(1, &uninitInfo) == PAD_ERROR_INVALID_HANDLE);
    const auto* rawUninit = reinterpret_cast<const std::uint8_t*>(&uninitInfo);
    for (std::size_t i = 0; i < sizeof(PadControllerInformation); ++i) {
        Require(rawUninit[i] == 0x55);
    }

    Require(scePadInit_nid_postfix() == PAD_OK);

    VerifyUnopenedHandles({-1000, -50, -1, 0, 1, 2, 3, 4, 5, 15, 16, 31, 32, 47, 48, 100, 99999});

    const int invalidUserOpen = scePadOpen_nid_postfix(PAD_USER_ID_INVALID, PAD_PORT_TYPE_STANDARD, 0, nullptr);
    Require(invalidUserOpen == static_cast<int>(0x809b0001));

    const int outOfRangeUserLow = scePadOpen_nid_postfix(0x0fffffff, PAD_PORT_TYPE_STANDARD, 0, nullptr);
    Require(outOfRangeUserLow == PAD_ERROR_INVALID_ARG);

    const int outOfRangeUserHigh = scePadOpen_nid_postfix(0x10000004, PAD_PORT_TYPE_STANDARD, 0, nullptr);
    Require(outOfRangeUserHigh == PAD_ERROR_INVALID_ARG);

    const int invalidIndexOpen = scePadOpen_nid_postfix(0x10000000, PAD_PORT_TYPE_STANDARD, 1, nullptr);
    Require(invalidIndexOpen == static_cast<int>(0x809b0003));

    const int invalidType3 = scePadOpen_nid_postfix(0x10000000, 3, 0, nullptr);
    Require(invalidType3 == PAD_ERROR_INVALID_ARG);

    const int invalidType4 = scePadOpen_nid_postfix(0x10000000, 4, 0, nullptr);
    Require(invalidType4 == PAD_ERROR_INVALID_ARG);

    const int invalidTypeNeg = scePadOpen_nid_postfix(0x10000000, -1, 0, nullptr);
    Require(invalidTypeNeg == PAD_ERROR_INVALID_ARG);

    const int specialHandle = scePadOpen_nid_postfix(0x10000000, PAD_PORT_TYPE_SPECIAL, 0, nullptr);
    Require(specialHandle == PAD_SPECIAL_HANDLE);

    Require(scePadGetControllerInformation(specialHandle, nullptr) == PAD_ERROR_INVALID_ARG);

    PadControllerInformation specialInfo{};
    std::memset(&specialInfo, 0xCC, sizeof(specialInfo));
    Require(scePadGetControllerInformation(specialHandle, &specialInfo) == PAD_OK);
    VerifySpecialHandleZeroedInfo(specialInfo);

    Require(scePadClose_nid_postfix(specialHandle) == PAD_OK);

    VerifyUnopenedHandles({specialHandle});

    const int h0 = scePadOpen_nid_postfix(0x10000000, PAD_PORT_TYPE_STANDARD, 0, nullptr);
    Require(h0 == PAD_HANDLE_BASE + 0);

    Require(scePadGetControllerInformation(h0, nullptr) == PAD_ERROR_INVALID_ARG);

    VerifyUnopenedHandles({PAD_HANDLE_BASE + 1, PAD_HANDLE_BASE + 2, PAD_HANDLE_BASE + 3});

    PadControllerInformation h0Info{};
    Require(scePadGetControllerInformation(h0, &h0Info) == PAD_OK);
    VerifyStandardDualSenseInfo(h0Info, true);

    const int h1 = scePadOpen_nid_postfix(0x10000001, PAD_PORT_TYPE_STANDARD, 0, nullptr);
    const int h2 = scePadOpen_nid_postfix(0x10000002, PAD_PORT_TYPE_STANDARD, 0, nullptr);
    const int h3 = scePadOpen_nid_postfix(0x10000003, PAD_PORT_TYPE_STANDARD, 0, nullptr);
    Require(h1 == PAD_HANDLE_BASE + 1);
    Require(h2 == PAD_HANDLE_BASE + 2);
    Require(h3 == PAD_HANDLE_BASE + 3);

    const int handles[4] = {h0, h1, h2, h3};

    for (int p = 0; p < 4; ++p) {
        Require(scePadGetControllerInformation(handles[p], nullptr) == PAD_ERROR_INVALID_ARG);
    }

    PadControllerInformation portInitialInfo{};
    Require(scePadGetControllerInformation(handles[0], &portInitialInfo) == PAD_OK);
    VerifyStandardDualSenseInfo(portInitialInfo, true);

    for (int p = 1; p < 4; ++p) {
        PadControllerInformation info{};
        Require(scePadGetControllerInformation(handles[p], &info) == PAD_OK);
        VerifyStandardDualSenseInfo(info, false);
    }

    for (int mask = 0; mask < 16; ++mask) {
        for (int p = 0; p < 4; ++p) {
            const bool shouldConnect = ((mask & (1 << p)) != 0);
            PadSetPortConnected_nid_postfix(p, shouldConnect);
        }

        for (int p = 0; p < 4; ++p) {
            PadControllerInformation info{};
            std::memset(&info, 0xFF, sizeof(info));
            Require(scePadGetControllerInformation(handles[p], &info) == PAD_OK);
            const bool expected = ((mask & (1 << p)) != 0);
            VerifyStandardDualSenseInfo(info, expected);
        }
    }

    for (int p = 0; p < 4; ++p) {
        PadSetPortConnected_nid_postfix(p, true);
        PadControllerInformation info{};
        Require(scePadGetControllerInformation(handles[p], &info) == PAD_OK);
        VerifyStandardDualSenseInfo(info, true);
    }

    for (int p = 0; p < 4; ++p) {
        PadSetPortConnected_nid_postfix(p, false);
        PadControllerInformation info{};
        Require(scePadGetControllerInformation(handles[p], &info) == PAD_OK);
        VerifyStandardDualSenseInfo(info, false);
    }

    for (int p = 0; p < 4; ++p) {
        PadSetPortConnected_nid_postfix(p, true);
    }

    for (int p = 0; p < 4; ++p) {
        Require(scePadClose_nid_postfix(handles[p]) == PAD_OK);
    }

    for (int p = 0; p < 4; ++p) {
        PadControllerInformation closedInfo{};
        Require(scePadGetControllerInformation(handles[p], &closedInfo) == PAD_OK);
        VerifyStandardDualSenseInfo(closedInfo, true);

        Require(scePadGetControllerInformation(handles[p], nullptr) == PAD_ERROR_INVALID_ARG);
    }

    PadSetPortConnected_nid_postfix(0, false);
    PadSetPortConnected_nid_postfix(2, false);

    PadControllerInformation closedP0{};
    Require(scePadGetControllerInformation(handles[0], &closedP0) == PAD_OK);
    VerifyStandardDualSenseInfo(closedP0, false);

    PadControllerInformation closedP1{};
    Require(scePadGetControllerInformation(handles[1], &closedP1) == PAD_OK);
    VerifyStandardDualSenseInfo(closedP1, true);

    PadControllerInformation closedP2{};
    Require(scePadGetControllerInformation(handles[2], &closedP2) == PAD_OK);
    VerifyStandardDualSenseInfo(closedP2, false);

    PadControllerInformation closedP3{};
    Require(scePadGetControllerInformation(handles[3], &closedP3) == PAD_OK);
    VerifyStandardDualSenseInfo(closedP3, true);

    for (int p = 0; p < 4; ++p) {
        PadSetPortConnected_nid_postfix(p, true);
    }

    RequireNotImplemented("scePadSetConnection", [h0] {
        return scePadSetConnection(h0, true);
    });
    RequireNotImplemented("scePadSetConnection", [h0] {
        return scePadSetConnection(h0, false);
    });

    RequireNotImplemented("scePadConnectPort", [] {
        return scePadConnectPort(0x10000000, PAD_PORT_TYPE_STANDARD);
    });
    RequireNotImplemented("scePadDisconnectPort", [] {
        return scePadDisconnectPort(0x10000000, PAD_PORT_TYPE_STANDARD);
    });
    RequireNotImplemented("scePadDisconnectDevice", [] {
        return scePadDisconnectDevice(1);
    });

    return 0;
}
