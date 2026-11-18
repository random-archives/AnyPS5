#include "SceTypes.hpp"
#include "prx/libScePad/include/Pad.hpp"
#include "prx/libScePad/include/PadState.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

static_assert(sizeof(PadDeviceClassExtendedInformation) == 20);
static_assert(offsetof(PadDeviceClassExtendedInformation, deviceClass) == 0);
static_assert(offsetof(PadDeviceClassExtendedInformation, reserved) == 4);
static_assert(offsetof(PadDeviceClassExtendedInformation, classData) == 8);

static void Require(bool value) { if (!value) std::abort(); }

static bool ThrowsOnExtensionUnitInfo(int handle) {
    PadExtensionUnitInfo* info = nullptr;
    try {
        scePadGetExtensionUnitInfo(handle, info);
        return false;
    } catch (const std::exception&) {
        return true;
    }
}

int main() {
    std::array<std::uint8_t, 0x40> buf{};

    for (int handle : {-100, -1, 0, 1, 99, 12345}) {
        std::memset(buf.data(), 0x5a, buf.size());
        Require(scePadGetExtControllerInformation(handle, buf.data()) == PAD_ERROR_INVALID_HANDLE);
        Require(buf[0] == 0x5a);
    }

    for (int handle : {-100, -1, 0, 1, 99, 12345}) {
        Require(scePadGetExtControllerInformation(handle, nullptr) == PAD_ERROR_INVALID_HANDLE);
    }

    Require(scePadGetExtensionUnitInfo(1, nullptr) == PAD_ERROR_NOT_INITIALIZED);

    Require(scePadInit_nid_postfix() == PAD_OK);

    for (int handle : {-100, -1, 0, 1, 99, 12345}) {
        std::memset(buf.data(), 0x5a, buf.size());
        Require(scePadGetExtControllerInformation(handle, buf.data()) == PAD_ERROR_INVALID_HANDLE);
        Require(buf[0] == 0x5a);
    }

    for (int handle : {-100, -1, 0, 1, 99, 12345}) {
        Require(scePadGetExtControllerInformation(handle, nullptr) == PAD_ERROR_INVALID_HANDLE);
    }

    Require(scePadGetExtensionUnitInfo(1, nullptr) == PAD_ERROR_INVALID_ARG);

    PadExtensionUnitInfo placeholder{};
    Require(ThrowsOnExtensionUnitInfo(1));

    const int standard = scePadOpen_nid_postfix(0x10000000, PAD_PORT_TYPE_STANDARD, 0, nullptr);
    Require(standard > 0);

    std::memset(buf.data(), 0x5a, buf.size());
    Require(scePadGetExtControllerInformation(standard, nullptr) == PAD_ERROR_INVALID_ARG);
    Require(buf[0] == 0x5a);

    std::memset(buf.data(), 0x5a, buf.size());
    Require(scePadGetExtControllerInformation(standard, buf.data()) == PAD_ERROR_NOT_PERMITTED);
    Require(buf[0] == 0x5a);

    Require(ThrowsOnExtensionUnitInfo(standard));

    const int special = scePadOpen_nid_postfix(0x10000000, PAD_PORT_TYPE_SPECIAL, 0, nullptr);
    Require(special > 0);

    Require(scePadGetExtControllerInformation(special, nullptr) == PAD_ERROR_INVALID_ARG);

    std::memset(buf.data(), 0x5a, buf.size());
    Require(scePadGetExtControllerInformation(special, buf.data()) == PAD_OK);
    for (int i = 0; i < 0x2c; ++i) {
        Require(buf[i] == 0);
    }
    for (int i = 0x2c; i < static_cast<int>(buf.size()); ++i) {
        Require(buf[i] == 0x5a);
    }

    std::memset(buf.data(), 0x5a, buf.size());
    Require(scePadGetExtControllerInformation(special, buf.data()) == PAD_OK);
    for (int i = 0; i < 0x2c; ++i) {
        Require(buf[i] == 0);
    }

    Require(ThrowsOnExtensionUnitInfo(special));

    PadSetPortConnected_nid_postfix(0, false);

    std::memset(buf.data(), 0x5a, buf.size());
    Require(scePadGetExtControllerInformation(standard, buf.data()) == PAD_ERROR_NOT_PERMITTED);
    Require(buf[0] == 0x5a);

    std::memset(buf.data(), 0x5a, buf.size());
    Require(scePadGetExtControllerInformation(special, buf.data()) == PAD_OK);
    for (int i = 0; i < 0x2c; ++i) {
        Require(buf[i] == 0);
    }

    PadSetPortConnected_nid_postfix(0, true);

    Require(scePadClose_nid_postfix(standard) == PAD_OK);

    std::memset(buf.data(), 0x5a, buf.size());
    Require(scePadGetExtControllerInformation(standard, buf.data()) == PAD_ERROR_INVALID_HANDLE);
    Require(buf[0] == 0x5a);

    Require(scePadGetExtControllerInformation(special, buf.data()) == PAD_OK);
    for (int i = 0; i < 0x2c; ++i) {
        Require(buf[i] == 0);
    }

    Require(scePadClose_nid_postfix(special) == PAD_OK);

    std::memset(buf.data(), 0x5a, buf.size());
    Require(scePadGetExtControllerInformation(special, buf.data()) == PAD_ERROR_INVALID_HANDLE);
    Require(buf[0] == 0x5a);
}
