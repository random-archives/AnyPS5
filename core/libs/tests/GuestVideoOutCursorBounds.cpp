#include "prx/libSceVideoOut/include/Output.hpp"
#include "prx/libSceVideoOut/include/VideoOut.hpp"
#include "tests/VideoOutTestEnvironment.hpp"
#include <cstdlib>
#include <stdexcept>

namespace {

constexpr int SystemUser = 255;
constexpr int MainBus = 0;
constexpr int ErrorInvalidHandle = -2144796661;
constexpr int ErrorInvalidAddress = -2144796670;

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

template <typename Fn>
int Call(Fn&& fn) {
    return fn();
}

void TestUnopenedHandleBounds() {
    for (const int h : {-1000, -1, 0, 1, 2, 42, 9999}) {
        Require(Call([h] { return sceVideoOutCursorDisable(h, 0); }) == ErrorInvalidHandle);
        Require(Call([h] { return sceVideoOutCursorSetPosition(h, 0, 100, 100); }) == ErrorInvalidHandle);
        Require(Call([h] { return sceVideoOutCursorSet2xMagnify(h, 0, false); }) == ErrorInvalidHandle);
        Require(Call([h] { return sceVideoOutCursorEnable(h, 0, nullptr); }) == ErrorInvalidHandle);
        Require(Call([h] { return sceVideoOutCursorSetImageAddress(h, 0, nullptr); }) == ErrorInvalidHandle);
    }
}

void TestNonZeroIndexThrows() {
    for (const int index : {1, 2, 3, 100}) {
        Require(Throws([index] { return sceVideoOutCursorEnable(-1, index, nullptr); }));
        Require(Throws([index] { return sceVideoOutCursorDisable(-1, index); }));
        Require(Throws([index] { return sceVideoOutCursorSetImageAddress(-1, index, nullptr); }));
        Require(Throws([index] { return sceVideoOutCursorSetPosition(-1, index, 100, 100); }));
        Require(Throws([index] { return sceVideoOutCursorSet2xMagnify(-1, index, false); }));
    }
}

void TestNullAddressOnOpenHandle(int handle) {
    Require(Call([handle] { return sceVideoOutCursorEnable(handle, 0, nullptr); }) == ErrorInvalidAddress);
    Require(Call([handle] { return sceVideoOutCursorSetImageAddress(handle, 0, nullptr); }) == ErrorInvalidAddress);
}

void TestOpenHandleNullFreeOperations(int handle) {
    Require(Call([handle] { return sceVideoOutCursorDisable(handle, 0); }) == 0);
    Require(Call([handle] { return sceVideoOutCursorSetPosition(handle, 0, 0, 0); }) == 0);
    Require(Call([handle] { return sceVideoOutCursorSetPosition(handle, 0, 100, 100); }) == 0);
    Require(Call([handle] { return sceVideoOutCursorSetPosition(handle, 0, 3840, 2160); }) == 0);
    Require(Call([handle] { return sceVideoOutCursorSet2xMagnify(handle, 0, false); }) == 0);
    Require(Call([handle] { return sceVideoOutCursorSet2xMagnify(handle, 0, true); }) == 0);
}

void TestClosedHandleBounds(int handle) {
    Require(Call([handle] { return sceVideoOutCursorDisable(handle, 0); }) == ErrorInvalidHandle);
    Require(Call([handle] { return sceVideoOutCursorSetPosition(handle, 0, 100, 100); }) == ErrorInvalidHandle);
    Require(Call([handle] { return sceVideoOutCursorSet2xMagnify(handle, 0, false); }) == ErrorInvalidHandle);
    Require(Call([handle] { return sceVideoOutCursorEnable(handle, 0, nullptr); }) == ErrorInvalidHandle);
    Require(Call([handle] { return sceVideoOutCursorSetImageAddress(handle, 0, nullptr); }) == ErrorInvalidHandle);
}

} // namespace

int main() {
    VideoOutTestEnvironment environment;

    int handle = -1;
    try {
        handle = sceVideoOutOpen(SystemUser, MainBus, 0, nullptr);
    } catch (const std::runtime_error&) {
        if (std::getenv("ANYPS5_REQUIRE_DISPLAY") != nullptr) throw;
        return 77;
    }

    Require(handle >= 0);

    TestNonZeroIndexThrows();
    TestNullAddressOnOpenHandle(handle);
    TestOpenHandleNullFreeOperations(handle);

    Require(sceVideoOutClose(handle) == 0);

    TestUnopenedHandleBounds();
    TestClosedHandleBounds(handle);

    return 0;
}
