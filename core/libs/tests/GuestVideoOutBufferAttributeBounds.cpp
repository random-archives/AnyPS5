#include "prx/libSceVideoOut/include/Buffer.hpp"
#include "prx/libSceVideoOut/include/Output.hpp"
#include "SceTypes.hpp"
#include "tests/VideoOutTestEnvironment.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace {

constexpr int SystemUser = 255;
constexpr int MainBus = 0;

constexpr int ErrorInvalidAddress = static_cast<int>(0x80290002u);
constexpr int ErrorInvalidHandle = static_cast<int>(0x8029000Bu);
constexpr int VideoOutNumMax = 4;

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

void TestSetBufferAttributeBounds() {
    Require(Throws([] {
        sceVideoOutSetBufferAttribute(nullptr, 0x80000000u, 1, 0, 1920, 1080, 1920);
    }));

    VideoOutBufferAttribute attr{};
    std::memset(&attr, 0x5A, sizeof(attr));

    for (const uint32_t width : {64u, 128u, 1280u, 1920u, 2560u, 3840u, 7680u}) {
        for (const uint32_t height : {64u, 128u, 720u, 1080u, 1440u, 2160u, 4320u}) {
            for (const uint32_t tiling : {0u, 1u, 2u}) {
                for (const uint32_t format : {0x80000000u, 0x80002200u, 0x88000000u, 0x12345678u}) {
                    std::memset(&attr, 0x5A, sizeof(attr));
                    sceVideoOutSetBufferAttribute(&attr, format, tiling, 0, width, height, width);

                    Require(attr.pixel_format == format);
                    Require(attr.tiling_mode == tiling);
                    Require(attr.aspect_ratio == 0);
                    Require(attr.width == width);
                    Require(attr.height == height);
                    Require(attr.pitch_in_pixel == width);
                    Require(attr.reserved0 == 0);
                    Require(attr.reserved1 == 0);
                }
            }
        }
    }
}

void TestSetBufferAttribute2Bounds() {
    Require(Throws([] {
        sceVideoOutSetBufferAttribute2(nullptr, 0x8000000000000000ull, 0, 1920, 1080, 0, 0, 0);
    }));

    VideoOutBufferAttribute2 attr2{};
    std::memset(&attr2, 0x5A, sizeof(attr2));

    for (const uint32_t width : {64u, 128u, 1280u, 1920u, 2560u, 3840u, 7680u}) {
        for (const uint32_t height : {64u, 128u, 720u, 1080u, 1440u, 2160u, 4320u}) {
            for (const uint32_t tiling : {0u, 1u, 2u}) {
                for (const uint64_t option : {0ull, 8ull, 0x10ull}) {
                    for (const uint64_t format : {0x8000000000000000ull, 0x8000000022000000ull, 0x8100000000000000ull}) {
                        std::memset(&attr2, 0x5A, sizeof(attr2));
                        sceVideoOutSetBufferAttribute2(&attr2, format, tiling, width, height, option, 0x208u, 0x11223344ull);

                        Require(attr2.reserved0 == 0);
                        Require(attr2.tiling_mode == tiling);
                        Require(attr2.aspect_ratio == 0);
                        Require(attr2.width == width);
                        Require(attr2.height == height);
                        Require(attr2.pitch_in_pixel == 0);
                        Require(attr2.option == option);
                        Require(attr2.pixel_format == format);
                        Require(attr2.dcc_cb_register_clear_color == 0x11223344ull);
                        Require(attr2.dcc_control == 0x208u);
                        Require(attr2.pad0 == 0);
                        Require(attr2.reserved1[0] == 0);
                        Require(attr2.reserved1[1] == 0);
                        Require(attr2.reserved1[2] == 0);
                    }
                }
            }
        }
    }
}

void TestAttributeOptionHelpers() {
    Require(Throws([] { sceVideoOutAddBufferAttributeOption(nullptr, 8); }));
    Require(Throws([] { sceVideoOutRemoveBufferAttributeOption(nullptr, 8); }));

    VideoOutBufferAttribute2 attr2{};
    sceVideoOutSetBufferAttribute2(&attr2, 0x8000000000000000ull, 0, 1920, 1080, 0, 0, 0);
    Require(attr2.option == 0);

    sceVideoOutAddBufferAttributeOption(&attr2, 8);
    Require(attr2.option == 8);

    sceVideoOutAddBufferAttributeOption(&attr2, 0x10);
    Require(attr2.option == 0x18);

    sceVideoOutRemoveBufferAttributeOption(&attr2, 8);
    Require(attr2.option == 0x10);

    sceVideoOutRemoveBufferAttributeOption(&attr2, 0x10);
    Require(attr2.option == 0);
}

void TestUnopenedHandleErrorPrecedence() {
    VideoOutBufferAttribute attr{};
    sceVideoOutSetBufferAttribute(&attr, 0x80000000u, 1, 0, 1920, 1080, 1920);

    VideoOutBufferAttribute2 attr2{};
    sceVideoOutSetBufferAttribute2(&attr2, 0x8000000000000000ull, 0, 1920, 1080, 0, 0, 0);

    for (const int badHandle : {-1000, -100, -1, 0, 1, 2, 8, 100, 9999, 0x7FFFFFFF}) {
        Require(sceVideoOutSubmitChangeBufferAttribute(badHandle, 0, nullptr) == ErrorInvalidAddress);
        Require(sceVideoOutSubmitChangeBufferAttribute(badHandle, -1, nullptr) == ErrorInvalidAddress);
        Require(sceVideoOutSubmitChangeBufferAttribute(badHandle, 16, nullptr) == ErrorInvalidAddress);

        Require(sceVideoOutSubmitChangeBufferAttribute(badHandle, 0, &attr) == ErrorInvalidHandle);
        Require(sceVideoOutSubmitChangeBufferAttribute(badHandle, -1, &attr) == ErrorInvalidHandle);

        Require(Throws([badHandle, &attr2] {
            return sceVideoOutSubmitChangeBufferAttribute2(badHandle, 0, &attr2, nullptr);
        }));
        Require(Throws([badHandle] {
            return sceVideoOutSubmitChangeBufferAttribute2(badHandle, 0, nullptr, nullptr);
        }));

        Require(Throws([badHandle, &attr] {
            void* dummyAddress = nullptr;
            return sceVideoOutRegisterBuffers(badHandle, 0, &dummyAddress, 1, &attr);
        }));
        VideoOutBuffers dummyBuffers{};
        if (badHandle <= 0 || badHandle >= VideoOutNumMax) {
            Require(sceVideoOutRegisterBuffers2(badHandle, 0, 0, &dummyBuffers, 1, &attr2, 0, nullptr) == ErrorInvalidHandle);
        } else {
            Require(Throws([badHandle, &dummyBuffers, &attr2] {
                return sceVideoOutRegisterBuffers2(badHandle, 0, 0, &dummyBuffers, 1, &attr2, 0, nullptr);
            }));
        }
    }
}

void TestPostCloseErrorPrecedence(int handle) {
    VideoOutBufferAttribute attr{};
    sceVideoOutSetBufferAttribute(&attr, 0x80000000u, 1, 0, 1920, 1080, 1920);

    Require(sceVideoOutSubmitChangeBufferAttribute(handle, 0, nullptr) == ErrorInvalidAddress);
    Require(sceVideoOutSubmitChangeBufferAttribute(handle, 0, &attr) == ErrorInvalidHandle);
}

} // namespace

int main() {
    VideoOutTestEnvironment environment;

    int handle = -1;
    try {
        handle = sceVideoOutOpen(SystemUser, MainBus, 0, nullptr);
    } catch (const std::runtime_error& error) {
        if (std::getenv("ANYPS5_REQUIRE_DISPLAY") != nullptr) throw;
        return 77;
    }

    Require(handle >= 0);

    TestSetBufferAttributeBounds();
    TestSetBufferAttribute2Bounds();
    TestAttributeOptionHelpers();

    Require(sceVideoOutClose(handle) == 0);

    TestPostCloseErrorPrecedence(handle);
    TestUnopenedHandleErrorPrecedence();

    return 0;
}
