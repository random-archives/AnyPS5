#include "prx/libSceVideoOut/include/Buffer.hpp"
#include "prx/libSceVideoOut/include/InventoryExports.hpp"
#include "prx/libSceVideoOut/include/Output.hpp"
#include "prx/libSceVideoOut/include/VideoOut.hpp"
#include "SceTypes.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include "tests/VideoOutTestEnvironment.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

namespace {

constexpr int SystemUser = 255;
constexpr int MainBus = 0;
constexpr int VideoOutErrorInvalidValue = static_cast<int>(0x80290001u);
constexpr int VideoOutErrorInvalidAddress = static_cast<int>(0x80290002u);
constexpr int VideoOutErrorInvalidIndex = static_cast<int>(0x8029000au);
constexpr int VideoOutErrorFlipQueueFull = static_cast<int>(0x80290012u);
constexpr int VideoOutErrorInvalidHandle = static_cast<int>(0x8029000bu);
constexpr int BlackBufferIndex = -1;

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

    Require(Throws([] { sceVideoOutSetFlipRate(0, 0); }));
    Require(Throws([] { sceVideoOutSetFlipRate(2, 0); }));
    Require(Throws([] { sceVideoOutVrrPegToFixedRate(0, 0, 0); }));
    Require(Throws([] { sceVideoOutVrrPegToFixedRate(2, 0, 0); }));
    Require(Throws([] { sceVideoOutVrrUnpegFromFixedRate(0); }));
    Require(Throws([] { sceVideoOutVrrUnpegFromFixedRate(2); }));

    std::uint32_t canaryToken = 0x5a5a5a5au;
    Require(sceVideoOutSubmitEopFlip(-1, BlackBufferIndex, 1, 0, &canaryToken) == VideoOutErrorInvalidHandle);
    Require(canaryToken == 0x5a5a5a5au);
    Require(sceVideoOutSubmitEopFlip(0, BlackBufferIndex, 1, 0, &canaryToken) == VideoOutErrorInvalidHandle);
    Require(canaryToken == 0x5a5a5a5au);
    Require(sceVideoOutSubmitEopFlip(2, BlackBufferIndex, 1, 0, &canaryToken) == VideoOutErrorInvalidHandle);
    Require(canaryToken == 0x5a5a5a5au);
    Require(sceVideoOutSubmitEopFlip(1, BlackBufferIndex, 1, 0, nullptr) == VideoOutErrorInvalidAddress);

    Require(sceVideoOutAddVrrStatusFlagsPrivilege() == 0);
    Require(sceVideoOutAddBufferHdrPrivilege() == 0);

    Require(Throws([] { sceVideoOutAddBuffer4k2kPrivilege(); }));
    Require(Throws([] { sceVideoOutAddBufferAnisoPrivilege(); }));
    Require(Throws([] { sceVideoOutAddBufferYccPrivilege(); }));
    Require(Throws([] { sceVideoOutAddS3dHmdPrivilege(); }));
    Require(Throws([] { sceVideoOutDebugAddVrrPrivilege(); }));
    Require(Throws([] { sceVideoOutSysUpdatePrivilege(); }));
    Require(Throws([] { sceDbgVideoOutAddOutputModeVr60Privilege(); }));

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

    Require(sceVideoOutVrrPegToFixedRate(handle, 0, 0) == 0);
    Require(Throws([&] { sceVideoOutVrrPegToFixedRate(handle, 1, 0); }));
    Require(Throws([&] { sceVideoOutVrrPegToFixedRate(handle, 0, 1); }));
    Require(Throws([&] { sceVideoOutVrrPegToFixedRate(handle, 1, 1); }));
    Require(sceVideoOutVrrUnpegFromFixedRate(handle) == 0);

    Require(sceVideoOutSubmitEopFlip(handle, BlackBufferIndex, 1, 0, nullptr) == VideoOutErrorInvalidAddress);
    Require(sceVideoOutSubmitEopFlip(handle, BlackBufferIndex, 0, 0, &canaryToken) == VideoOutErrorInvalidValue);
    Require(sceVideoOutSubmitEopFlip(handle, BlackBufferIndex, 7, 0, &canaryToken) == VideoOutErrorInvalidValue);
    Require(canaryToken == 0x5a5a5a5au);

    Require(sceVideoOutSubmitEopFlip(handle, -3, 1, 0, &canaryToken) == VideoOutErrorInvalidIndex);
    Require(sceVideoOutSubmitEopFlip(handle, 16, 1, 0, &canaryToken) == VideoOutErrorInvalidIndex);
    Require(sceVideoOutSubmitEopFlip(handle, 100, 1, 0, &canaryToken) == VideoOutErrorInvalidIndex);
    Require(canaryToken == 0x5a5a5a5au);

    std::array<std::uint32_t, 16> reservedTokens{};
    for (std::size_t i = 0; i < reservedTokens.size(); ++i) {
        Require(sceVideoOutSubmitEopFlip(handle, BlackBufferIndex, 1, 0, &reservedTokens[i]) == 0);
        Require(reservedTokens[i] >= 0x04000000u && reservedTokens[i] <= 0x07ffffffu);
        if (i > 0) {
            Require(reservedTokens[i] > reservedTokens[i - 1]);
        }
    }

    std::uint32_t overflowToken = 0xa5a5a5a5u;
    Require(sceVideoOutSubmitEopFlip(handle, BlackBufferIndex, 1, 0, &overflowToken) == VideoOutErrorFlipQueueFull);
    Require(overflowToken == 0xa5a5a5a5u);

    Require(sceVideoOutClose(handle) == 0);

    Require(sceVideoOutSubmitEopFlip(handle, BlackBufferIndex, 1, 0, &canaryToken) == VideoOutErrorInvalidHandle);
    Require(Throws([&] { sceVideoOutSubmitFlip(handle, BlackBufferIndex, 1, 0); }));
    Require(Throws([&] { sceVideoOutSetFlipRate(handle, 0); }));
    Require(Throws([&] { sceVideoOutVrrPegToFixedRate(handle, 0, 0); }));
    Require(Throws([&] { sceVideoOutVrrUnpegFromFixedRate(handle); }));

    LibcRunShutdown_nid_postfix();
    return 0;
}
