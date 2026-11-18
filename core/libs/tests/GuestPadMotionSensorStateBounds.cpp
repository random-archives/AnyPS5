#include "SceTypes.hpp"
#include "prx/libScePad/include/Pad.hpp"
#include "prx/libScePad/include/PadState.hpp"

#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <stdexcept>
#include <string_view>

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

static void VerifyUnopenedHandles(const std::initializer_list<int>& handles) {
    PadMotionSensorPosition position{};
    float unit = 1.0f;

    for (int handle : handles) {
        Require(scePadSetMotionSensorState(handle, false) == PAD_ERROR_INVALID_HANDLE);
        Require(scePadSetMotionSensorState(handle, true) == PAD_ERROR_INVALID_HANDLE);
        Require(scePadResetOrientation(handle) == PAD_ERROR_INVALID_HANDLE);
        Require(scePadSetAngularVelocityDeadbandState(handle, false) == PAD_ERROR_INVALID_HANDLE);
        Require(scePadSetAngularVelocityDeadbandState(handle, true) == PAD_ERROR_INVALID_HANDLE);
        Require(scePadSetTiltCorrectionState(handle, false) == PAD_ERROR_INVALID_HANDLE);
        Require(scePadSetTiltCorrectionState(handle, true) == PAD_ERROR_INVALID_HANDLE);
        Require(scePadSetAngularVelocityBiasCorrectionState(handle, false) == PAD_ERROR_INVALID_HANDLE);
        Require(scePadSetAngularVelocityBiasCorrectionState(handle, true) == PAD_ERROR_INVALID_HANDLE);

        Require(scePadGetMotionSensorPosition(handle, nullptr) == PAD_ERROR_INVALID_ARG);
        Require(scePadGetMotionSensorPosition(handle, &position) == PAD_ERROR_INVALID_HANDLE);
        Require(scePadGetMotionTimerUnit(handle, nullptr) == PAD_ERROR_INVALID_ARG);
        Require(scePadGetMotionTimerUnit(handle, &unit) == PAD_ERROR_INVALID_HANDLE);
    }
}

int main() {
    PadMotionSensorPosition earlyPos{1.0f, 2.0f, 3.0f};
    float earlyUnit = 5.0f;

    for (int handle : {-10000, -100, -2, -1, 0, 1, 2, 3, 4, 5, 15, 16, 31, 32, 47, 48, 100, 99999, 0x7fffffff}) {
        Require(scePadSetMotionSensorState(handle, false) == PAD_ERROR_INVALID_HANDLE);
        Require(scePadSetMotionSensorState(handle, true) == PAD_ERROR_INVALID_HANDLE);
        Require(scePadResetOrientation(handle) == PAD_ERROR_INVALID_HANDLE);
        Require(scePadSetAngularVelocityDeadbandState(handle, false) == PAD_ERROR_INVALID_HANDLE);
        Require(scePadSetAngularVelocityDeadbandState(handle, true) == PAD_ERROR_INVALID_HANDLE);
        Require(scePadSetTiltCorrectionState(handle, false) == PAD_ERROR_INVALID_HANDLE);
        Require(scePadSetTiltCorrectionState(handle, true) == PAD_ERROR_INVALID_HANDLE);
        Require(scePadSetAngularVelocityBiasCorrectionState(handle, false) == PAD_ERROR_INVALID_HANDLE);
        Require(scePadSetAngularVelocityBiasCorrectionState(handle, true) == PAD_ERROR_INVALID_HANDLE);

        Require(scePadGetMotionSensorPosition(handle, nullptr) == PAD_ERROR_NOT_INITIALIZED);
        Require(scePadGetMotionSensorPosition(handle, &earlyPos) == PAD_ERROR_NOT_INITIALIZED);
        Require(earlyPos.x == 1.0f && earlyPos.y == 2.0f && earlyPos.z == 3.0f);

        Require(scePadGetMotionTimerUnit(handle, nullptr) == PAD_ERROR_NOT_INITIALIZED);
        Require(scePadGetMotionTimerUnit(handle, &earlyUnit) == PAD_ERROR_NOT_INITIALIZED);
        Require(earlyUnit == 5.0f);
    }

    RequireNotImplemented("scePadResetOrientationForTracker", [] {
        return scePadResetOrientationForTracker(1);
    });

    Require(scePadInit_nid_postfix() == PAD_OK);

    VerifyUnopenedHandles({-1000, -50, -1, 0, 1, 2, 3, 4, 5, 15, 16, 31, 32, 47, 48, 100, 99999});

    const int specialHandle = scePadOpen_nid_postfix(0x10000000, PAD_PORT_TYPE_SPECIAL, 0, nullptr);
    Require(specialHandle == PAD_SPECIAL_HANDLE);

    Require(scePadSetMotionSensorState(specialHandle, true) == PAD_OK);
    Require(scePadSetMotionSensorState(specialHandle, false) == PAD_OK);
    Require(scePadResetOrientation(specialHandle) == PAD_OK);
    Require(scePadSetAngularVelocityDeadbandState(specialHandle, true) == PAD_OK);
    Require(scePadSetAngularVelocityDeadbandState(specialHandle, false) == PAD_OK);
    Require(scePadSetTiltCorrectionState(specialHandle, true) == PAD_OK);
    Require(scePadSetTiltCorrectionState(specialHandle, false) == PAD_OK);
    Require(scePadSetAngularVelocityBiasCorrectionState(specialHandle, false) == PAD_OK);
    RequireNotImplemented("scePadSetAngularVelocityBiasCorrectionState", [specialHandle] {
        return scePadSetAngularVelocityBiasCorrectionState(specialHandle, true);
    });

    PadMotionSensorPosition specialPos{};
    float specialUnit = 0.0f;
    Require(scePadGetMotionSensorPosition(specialHandle, &specialPos) == static_cast<int>(0x80920007));
    Require(scePadGetMotionTimerUnit(specialHandle, &specialUnit) == static_cast<int>(0x80920007));

    Require(scePadClose_nid_postfix(specialHandle) == PAD_OK);

    VerifyUnopenedHandles({specialHandle});

    const int h0 = scePadOpen_nid_postfix(0x10000000, PAD_PORT_TYPE_STANDARD, 0, nullptr);
    Require(h0 == PAD_HANDLE_BASE + 0);

    VerifyUnopenedHandles({PAD_HANDLE_BASE + 1, PAD_HANDLE_BASE + 2, PAD_HANDLE_BASE + 3});

    std::uint32_t seq0 = 0;
    PadOutputState out0{};
    Require(PadFetchOutputForPort_nid_postfix(0, &seq0, &out0));
    Require(out0.motionEnabled == true);

    Require(scePadSetMotionSensorState(h0, false) == PAD_OK);
    Require(PadFetchOutputForPort_nid_postfix(0, &seq0, &out0));
    Require(out0.motionEnabled == false);

    Require(scePadSetMotionSensorState(h0, false) == PAD_OK);
    Require(!PadFetchOutputForPort_nid_postfix(0, &seq0, &out0));

    Require(scePadSetMotionSensorState(h0, true) == PAD_OK);
    Require(PadFetchOutputForPort_nid_postfix(0, &seq0, &out0));
    Require(out0.motionEnabled == true);

    Require(scePadSetMotionSensorState(h0, true) == PAD_OK);
    Require(!PadFetchOutputForPort_nid_postfix(0, &seq0, &out0));

    PadInputState motionInput{};
    motionInput.hasMotion = true;
    motionInput.accel = {1.5f, 2.5f, 3.5f};
    motionInput.gyro = {0.1f, 0.2f, 0.3f};
    PadPublishInputForPort_nid_postfix(0, motionInput);

    PadData padData{};
    Require(scePadReadState(h0, &padData) == PAD_OK);
    Require(padData.acceleration_x == 1.5f && padData.acceleration_y == 2.5f && padData.acceleration_z == 3.5f);
    Require(padData.angular_velocity_x == 0.1f && padData.angular_velocity_y == 0.2f && padData.angular_velocity_z == 0.3f);

    Require(scePadSetMotionSensorState(h0, false) == PAD_OK);
    Require(scePadReadState(h0, &padData) == PAD_OK);
    Require(padData.acceleration_x == 0.0f && padData.acceleration_y == 1.0f && padData.acceleration_z == 0.0f);
    Require(padData.angular_velocity_x == 0.0f && padData.angular_velocity_y == 0.0f && padData.angular_velocity_z == 0.0f);

    Require(scePadSetMotionSensorState(h0, true) == PAD_OK);
    Require(scePadReadState(h0, &padData) == PAD_OK);
    Require(padData.acceleration_x == 1.5f && padData.acceleration_y == 2.5f && padData.acceleration_z == 3.5f);

    Require(scePadResetOrientation(h0) == PAD_OK);
    Require(scePadReadState(h0, &padData) == PAD_OK);
    Require(padData.orientation_x == 0.0f && padData.orientation_y == 0.0f &&
            padData.orientation_z == 0.0f && padData.orientation_w == 1.0f);

    const int h1 = scePadOpen_nid_postfix(0x10000001, PAD_PORT_TYPE_STANDARD, 0, nullptr);
    const int h2 = scePadOpen_nid_postfix(0x10000002, PAD_PORT_TYPE_STANDARD, 0, nullptr);
    const int h3 = scePadOpen_nid_postfix(0x10000003, PAD_PORT_TYPE_STANDARD, 0, nullptr);
    Require(h1 == PAD_HANDLE_BASE + 1);
    Require(h2 == PAD_HANDLE_BASE + 2);
    Require(h3 == PAD_HANDLE_BASE + 3);

    const int handles[4] = {h0, h1, h2, h3};

    Require(scePadSetMotionSensorState(handles[0], true) == PAD_OK);
    Require(scePadSetMotionSensorState(handles[1], false) == PAD_OK);
    Require(scePadSetMotionSensorState(handles[2], true) == PAD_OK);
    Require(scePadSetMotionSensorState(handles[3], false) == PAD_OK);

    for (int p = 0; p < 4; ++p) {
        std::uint32_t seq = 0;
        PadOutputState out{};
        Require(PadFetchOutputForPort_nid_postfix(p, &seq, &out));
        const bool expected = (p % 2 == 0);
        Require(out.motionEnabled == expected);
    }

    Require(scePadSetMotionSensorState(handles[0], false) == PAD_OK);
    Require(scePadSetMotionSensorState(handles[1], true) == PAD_OK);
    Require(scePadSetMotionSensorState(handles[2], false) == PAD_OK);
    Require(scePadSetMotionSensorState(handles[3], true) == PAD_OK);

    for (int p = 0; p < 4; ++p) {
        std::uint32_t seq = 0;
        PadOutputState out{};
        Require(PadFetchOutputForPort_nid_postfix(p, &seq, &out));
        const bool expected = (p % 2 == 1);
        Require(out.motionEnabled == expected);
    }

    for (int p = 0; p < 4; ++p) {
        Require(scePadResetOrientation(handles[p]) == PAD_OK);
        Require(scePadSetAngularVelocityDeadbandState(handles[p], true) == PAD_OK);
        Require(scePadSetAngularVelocityDeadbandState(handles[p], false) == PAD_OK);
        Require(scePadSetTiltCorrectionState(handles[p], true) == PAD_OK);
        Require(scePadSetTiltCorrectionState(handles[p], false) == PAD_OK);
        Require(scePadSetAngularVelocityBiasCorrectionState(handles[p], false) == PAD_OK);
        RequireNotImplemented("scePadSetAngularVelocityBiasCorrectionState", [h = handles[p]] {
            return scePadSetAngularVelocityBiasCorrectionState(h, true);
        });
        RequireNotImplemented("scePadResetOrientationForTracker", [h = handles[p]] {
            return scePadResetOrientationForTracker(h);
        });

        PadMotionSensorPosition pos{};
        float unit = 0.0f;
        Require(scePadGetMotionSensorPosition(handles[p], &pos) == static_cast<int>(0x80920007));
        Require(scePadGetMotionTimerUnit(handles[p], &unit) == static_cast<int>(0x80920007));
    }

    for (int p = 0; p < 4; ++p) {
        Require(scePadClose_nid_postfix(handles[p]) == PAD_OK);
    }

    for (int p = 0; p < 4; ++p) {
        Require(scePadSetMotionSensorState(handles[p], true) == PAD_OK);
        Require(scePadSetMotionSensorState(handles[p], false) == PAD_OK);
        Require(scePadResetOrientation(handles[p]) == PAD_OK);
        Require(scePadSetAngularVelocityDeadbandState(handles[p], true) == PAD_OK);
        Require(scePadSetTiltCorrectionState(handles[p], true) == PAD_OK);

        PadMotionSensorPosition closedPos{};
        float closedUnit = 0.0f;
        Require(scePadGetMotionSensorPosition(handles[p], &closedPos) == PAD_ERROR_INVALID_HANDLE);
        Require(scePadGetMotionTimerUnit(handles[p], &closedUnit) == PAD_ERROR_INVALID_HANDLE);
    }

    return 0;
}
