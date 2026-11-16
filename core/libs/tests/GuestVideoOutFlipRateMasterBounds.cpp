#include "prx/libSceVideoOut/include/Output.hpp"
#include "prx/libSceVideoOut/include/VideoOut.hpp"
#include "SceTypes.hpp"
#include "tests/VideoOutTestEnvironment.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

namespace {

constexpr int SystemUser = 255;
constexpr int MainBus = 0;

constexpr int ErrorInvalidValue = static_cast<int>(0x80290001u);
constexpr int ErrorInvalidHandle = static_cast<int>(0x8029000Bu);
constexpr int ErrorFreshMasterState = static_cast<int>(0x8029001Cu);

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

void TestUnopenedHandleGates(int openHandle) {
    for (const int badHandle : {-100, -1, 0, openHandle + 1, openHandle + 2, 8, 100, 9999}) {
        if (badHandle == openHandle) continue;
        Require(Throws([&] { return sceVideoOutSetFlipRate(badHandle, 0); }));
        Require(Throws([&] { return sceVideoOutSetFlipRate(badHandle, 1); }));
        Require(Throws([&] { return sceVideoOutSetFlipRate(badHandle, 2); }));
        Require(Throws([&] { return sceVideoOutSetFlipRate(badHandle, 60); }));
        Require(Throws([&] { return sceVideoOutSetFlipRate(badHandle, -1); }));

        Require(sceVideoOutSetFlipMaster(badHandle, 0) == ErrorInvalidHandle);
        Require(sceVideoOutSetFlipMaster(badHandle, 1) == ErrorInvalidHandle);
        Require(sceVideoOutSetFlipMaster(badHandle, 2) == ErrorInvalidHandle);
        Require(sceVideoOutSetFlipMaster(badHandle, -1) == ErrorInvalidHandle);

        Require(sceVideoOutClearFlipMaster(badHandle) == ErrorInvalidHandle);

        if (badHandle == -1) {
            Require(sceVideoOutVrrPegToFixedRate(badHandle, 0, 0) == ErrorInvalidHandle);
            Require(sceVideoOutVrrPegToFixedRate(badHandle, 1, 0) == ErrorInvalidHandle);
            Require(sceVideoOutVrrPegToFixedRate(badHandle, 0, 1) == ErrorInvalidHandle);
        } else {
            Require(Throws([&] { return sceVideoOutVrrPegToFixedRate(badHandle, 0, 0); }));
            Require(Throws([&] { return sceVideoOutVrrPegToFixedRate(badHandle, 1, 0); }));
            Require(Throws([&] { return sceVideoOutVrrPegToFixedRate(badHandle, 0, 1); }));
        }
    }
}

void TestOpenHandleOperations(int handle) {
    for (const int validRate : {0, 1, 2}) {
        Require(sceVideoOutSetFlipRate(handle, validRate) == 0);
    }

    for (const int invalidRate : {-100, -1, 3, 4, 15, 60, 120, 1000}) {
        Require(sceVideoOutSetFlipRate(handle, invalidRate) == ErrorInvalidValue);
    }

    Require(sceVideoOutSetFlipRate(handle, 0) == 0);

    Require(sceVideoOutSetFlipMaster(handle, 0) == ErrorInvalidHandle);
    Require(sceVideoOutSetFlipMaster(handle, 1) == ErrorInvalidHandle);

    for (const int unhandledMaster : {2, 3, 4, 10, -1, -100}) {
        Require(Throws([&] { return sceVideoOutSetFlipMaster(handle, unhandledMaster); }));
    }

    Require(sceVideoOutClearFlipMaster(handle) == ErrorFreshMasterState);

    Require(sceVideoOutVrrPegToFixedRate(handle, 0, 0) == 0);
    Require(Throws([&] { return sceVideoOutVrrPegToFixedRate(handle, 1, 0); }));
    Require(Throws([&] { return sceVideoOutVrrPegToFixedRate(handle, 0, 1); }));
    Require(Throws([&] { return sceVideoOutVrrPegToFixedRate(handle, 42, 99); }));

    Require(sceVideoOutAddVrrStatusFlagsPrivilege() == 0);
}

void TestPostCloseGates(int handle) {
    Require(Throws([&] { return sceVideoOutSetFlipRate(handle, 0); }));
    Require(Throws([&] { return sceVideoOutSetFlipRate(handle, 60); }));

    Require(sceVideoOutSetFlipMaster(handle, 0) == ErrorInvalidHandle);
    Require(sceVideoOutSetFlipMaster(handle, 1) == ErrorInvalidHandle);
    Require(sceVideoOutSetFlipMaster(handle, 2) == ErrorInvalidHandle);

    Require(sceVideoOutClearFlipMaster(handle) == ErrorInvalidHandle);

    Require(Throws([&] { return sceVideoOutVrrPegToFixedRate(handle, 0, 0); }));
    Require(sceVideoOutClose(handle) == ErrorInvalidHandle);
}

} // namespace

int main() {
    VideoOutTestEnvironment environment;

    int handle = -1;
    try {
        handle = sceVideoOutOpen(SystemUser, MainBus, 0, nullptr);
    } catch (const std::runtime_error& error) {
        if (std::getenv("ANYPS5_REQUIRE_DISPLAY") != nullptr) throw;
        std::printf("skipped, no display or Vulkan device: %s\n", error.what());
        return 77;
    }

    Require(handle >= 0);

    TestUnopenedHandleGates(handle);

    TestOpenHandleOperations(handle);

    Require(sceVideoOutClose(handle) == 0);

    TestPostCloseGates(handle);

    return 0;
}
