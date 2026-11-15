#include "prx/libSceVideoOut/include/Buffer.hpp"
#include "prx/libSceVideoOut/include/Output.hpp"
#include "SceTypes.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include "tests/VideoOutTestEnvironment.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace {

constexpr int SystemUser = 255;
constexpr int MainBus = 0;
constexpr int VideoOutErrorInvalidValue = static_cast<int>(0x80290001u);
constexpr int VideoOutErrorSlotOccupied = static_cast<int>(0x80290010u);
constexpr std::uint64_t PixelFormatBgra = 0x8000000000000000ull;
constexpr std::uint64_t StrictColorimetryOption = 8;

void Require(bool value) {
    if (!value) std::abort();
}

template <typename F>
bool Throws(F&& action) {
    try {
        action();
        return false;
    } catch (const std::runtime_error&) {
        return true;
    }
}

} // namespace

int main() {
    VideoOutTestEnvironment environment;

    std::vector<std::byte> allocation(65536u * 2u + 65535u);
    const auto address = reinterpret_cast<std::uintptr_t>(allocation.data());
    const auto offset = (65536u - (address & 65535u)) & 65535u;
    void* alignedBase = allocation.data() + offset;

    VideoOutBuffers buffers[2]{};
    buffers[0].data = alignedBase;
    buffers[1].data = static_cast<std::byte*>(alignedBase) + 65536u;

    VideoOutBufferAttribute2 attribute{};
    attribute.width = 64;
    attribute.height = 64;
    attribute.pixel_format = PixelFormatBgra;
    attribute.option = 0;

    for (int unopened : {-1, 0, 1, 2, 0x7FFFFFFF}) {
        Require(Throws([&] { sceVideoOutRegisterBuffers2(unopened, 0, 0, buffers, 1, &attribute, 0, nullptr); }));
        Require(Throws([&] { sceVideoOutUnregisterBuffers(unopened, 0); }));
    }

    Require(Throws([] { sceVideoOutSetBufferAttribute2(nullptr, PixelFormatBgra, 0, 64, 64, 0, 0, 0); }));
    VideoOutBufferAttribute2 helperAttr{};
    sceVideoOutSetBufferAttribute2(&helperAttr, PixelFormatBgra, 0, 64, 64, 0, 0, 0);
    Require(helperAttr.width == 64 && helperAttr.height == 64 && helperAttr.pixel_format == PixelFormatBgra);

    Require(Throws([] { sceVideoOutSetBufferAttribute(nullptr, 0x80000000u, 1, 0, 64, 64, 64); }));
    VideoOutBufferAttribute ps4Attr{};
    sceVideoOutSetBufferAttribute(&ps4Attr, 0x80000000u, 1, 0, 64, 64, 64);
    Require(ps4Attr.width == 64 && ps4Attr.height == 64 && ps4Attr.pixel_format == 0x80000000u);

    Require(Throws([] { sceVideoOutAddBufferAttributeOption(nullptr, StrictColorimetryOption); }));
    sceVideoOutAddBufferAttributeOption(&helperAttr, StrictColorimetryOption);
    Require(helperAttr.option == StrictColorimetryOption);

    Require(Throws([] { sceVideoOutRemoveBufferAttributeOption(nullptr, StrictColorimetryOption); }));
    sceVideoOutRemoveBufferAttributeOption(&helperAttr, StrictColorimetryOption);
    Require(helperAttr.option == 0);

    int handle = 0;
    try {
        handle = sceVideoOutOpen(SystemUser, MainBus, 0, nullptr);
    } catch (const std::runtime_error& error) {
        if (std::getenv("ANYPS5_REQUIRE_DISPLAY") != nullptr) throw;
        std::printf("skipped, no display or Vulkan device: %s\n", error.what());
        LibcRunShutdown_nid_postfix();
        return 77;
    }
    Require(handle > 0);

    Require(Throws([&] { sceVideoOutRegisterBuffers2(handle, 0, 0, nullptr, 1, &attribute, 0, nullptr); }));
    Require(Throws([&] { sceVideoOutRegisterBuffers2(handle, 0, 0, buffers, 1, nullptr, 0, nullptr); }));
    int dummyOption = 1;
    Require(Throws([&] { sceVideoOutRegisterBuffers2(handle, 0, 0, buffers, 1, &attribute, 0, &dummyOption); }));

    Require(Throws([&] { sceVideoOutRegisterBuffers2(handle, -1, 0, buffers, 1, &attribute, 0, nullptr); }));
    Require(Throws([&] { sceVideoOutRegisterBuffers2(handle, 4, 0, buffers, 1, &attribute, 0, nullptr); }));
    Require(Throws([&] { sceVideoOutRegisterBuffers2(handle, 10, 0, buffers, 1, &attribute, 0, nullptr); }));

    Require(Throws([&] { sceVideoOutRegisterBuffers2(handle, 0, -1, buffers, 1, &attribute, 0, nullptr); }));
    Require(Throws([&] { sceVideoOutRegisterBuffers2(handle, 0, 16, buffers, 1, &attribute, 0, nullptr); }));
    Require(Throws([&] { sceVideoOutRegisterBuffers2(handle, 0, 20, buffers, 1, &attribute, 0, nullptr); }));

    Require(Throws([&] { sceVideoOutRegisterBuffers2(handle, 0, 0, buffers, 0, &attribute, 0, nullptr); }));
    Require(Throws([&] { sceVideoOutRegisterBuffers2(handle, 0, 0, buffers, -1, &attribute, 0, nullptr); }));
    Require(Throws([&] { sceVideoOutRegisterBuffers2(handle, 0, 0, buffers, 17, &attribute, 0, nullptr); }));

    Require(Throws([&] { sceVideoOutRegisterBuffers2(handle, 0, 0, buffers, 1, &attribute, -1, nullptr); }));
    Require(Throws([&] { sceVideoOutRegisterBuffers2(handle, 0, 0, buffers, 1, &attribute, 2, nullptr); }));

    Require(sceVideoOutRegisterBuffers2(handle, 0, 15, buffers, 2, &attribute, 0, nullptr) == VideoOutErrorInvalidValue);
    Require(sceVideoOutRegisterBuffers2(handle, 0, 14, buffers, 3, &attribute, 0, nullptr) == VideoOutErrorInvalidValue);

    for (std::uint64_t invalidOption : {1ull, 2ull, 4ull, 16ull, 0x20ull}) {
        VideoOutBufferAttribute2 badAttr = attribute;
        badAttr.option = invalidOption;
        Require(Throws([&] { sceVideoOutRegisterBuffers2(handle, 0, 0, buffers, 1, &badAttr, 0, nullptr); }));
    }

    VideoOutBufferAttribute2 dccAttr = attribute;
    dccAttr.dcc_control = 0x208;
    Require(Throws([&] { sceVideoOutRegisterBuffers2(handle, 0, 0, buffers, 1, &dccAttr, 0, nullptr); }));
    dccAttr.dcc_control = 0;
    dccAttr.dcc_cb_register_clear_color = 0xFFFFFFFFull;
    Require(Throws([&] { sceVideoOutRegisterBuffers2(handle, 0, 0, buffers, 1, &dccAttr, 0, nullptr); }));

    Require(sceVideoOutRegisterBuffers2(handle, 0, 0, buffers, 2, &attribute, 0, nullptr) == 0);

    Require(sceVideoOutRegisterBuffers2(handle, 1, 0, buffers, 1, &attribute, 0, nullptr) == VideoOutErrorSlotOccupied);
    Require(sceVideoOutRegisterBuffers2(handle, 1, 1, buffers, 1, &attribute, 0, nullptr) == VideoOutErrorSlotOccupied);

    Require(Throws([&] { sceVideoOutRegisterBuffers2(handle, 0, 2, buffers, 1, &attribute, 0, nullptr); }));

    VideoOutBufferAttribute2 strictAttr = attribute;
    strictAttr.option = StrictColorimetryOption;
    Require(sceVideoOutRegisterBuffers2(handle, 1, 2, buffers, 1, &strictAttr, 0, nullptr) == 0);

    Require(Throws([&] { sceVideoOutUnregisterBuffers(handle, -1); }));
    Require(Throws([&] { sceVideoOutUnregisterBuffers(handle, 4); }));
    Require(Throws([&] { sceVideoOutUnregisterBuffers(handle, 2); }));
    Require(Throws([&] { sceVideoOutUnregisterBuffers(handle, 3); }));

    Require(sceVideoOutUnregisterBuffers(handle, 1) == 0);
    Require(Throws([&] { sceVideoOutUnregisterBuffers(handle, 1); }));

    Require(sceVideoOutRegisterBuffers2(handle, 1, 2, buffers, 1, &strictAttr, 0, nullptr) == 0);
    Require(sceVideoOutUnregisterBuffers(handle, 1) == 0);

    Require(sceVideoOutUnregisterBuffers(handle, 0) == 0);
    Require(Throws([&] { sceVideoOutUnregisterBuffers(handle, 0); }));

    Require(sceVideoOutClose(handle) == 0);

    Require(Throws([&] { sceVideoOutRegisterBuffers2(handle, 0, 0, buffers, 1, &attribute, 0, nullptr); }));
    Require(Throws([&] { sceVideoOutUnregisterBuffers(handle, 0); }));

    LibcRunShutdown_nid_postfix();
    return 0;
}
