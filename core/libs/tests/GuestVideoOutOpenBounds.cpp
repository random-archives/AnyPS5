#include "prx/libSceVideoOut/include/Output.hpp"
#include "SceTypes.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include "tests/VideoOutTestEnvironment.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>

namespace {

constexpr int SystemUser = 255;
constexpr int MainBus = 0;
constexpr int OverlayBus = 1;
constexpr int SubBus = 2;
constexpr int Hmd2MainBus = 0x20;

constexpr int VideoOutErrorInvalidValue = static_cast<int>(0x80290001u);
constexpr int VideoOutErrorResourceBusy = static_cast<int>(0x80290009u);
constexpr int VideoOutErrorInvalidHandle = static_cast<int>(0x8029000Bu);

struct VideoOutOpenParam {
    std::uint32_t firstWord = 16;
    std::uint32_t setPriority = 0;
    std::int32_t priority = 0;
    std::uint32_t setAffinity = 0;
    std::uint64_t affinity = 0;
};
static_assert(offsetof(VideoOutOpenParam, affinity) == 16);

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

void TestInvalidCloseHandles() {
    for (const int badHandle : {-1000, -100, -1, 0, 4, 10, 100, 9999, 0x7FFFFFFF}) {
        Require(sceVideoOutClose(badHandle) == VideoOutErrorInvalidHandle);
    }
}

void TestInvalidOpenParameters() {
    for (const int badUser : {-100, -1, 1, 2, 3, 10, 254, 256, 1000, 0x7FFFFFFF}) {
        Require(Throws([badUser] {
            return sceVideoOutOpen(badUser, MainBus, 0, nullptr);
        }));
    }

    Require(sceVideoOutOpen(SystemUser, Hmd2MainBus, 0, nullptr) == VideoOutErrorInvalidValue);
    Require(sceVideoOutOpen(0, Hmd2MainBus, 0, nullptr) == VideoOutErrorInvalidValue);

    for (const int badBus : {-100, -1, 3, 4, 5, 10, 0x1F, 0x21, 100, 0x7FFFFFFF}) {
        Require(Throws([badBus] {
            return sceVideoOutOpen(SystemUser, badBus, 0, nullptr);
        }));
    }

    for (const int badIndex : {-100, -1, 1, 2, 10, 100, 0x7FFFFFFF}) {
        Require(Throws([badIndex] {
            return sceVideoOutOpen(SystemUser, MainBus, badIndex, nullptr);
        }));
    }
}

void TestInvalidOpenParamStruct() {
    for (const std::uint32_t badFirstWord : {0u, 1u, 15u, 17u, 32u, 100u, 0xFFFFFFFFu}) {
        VideoOutOpenParam param{};
        param.firstWord = badFirstWord;
        Require(Throws([&param] {
            return sceVideoOutOpen(SystemUser, MainBus, 0, &param);
        }));
    }

    for (const std::uint32_t badPriorityFlag : {2u, 3u, 10u, 100u, 0xFFFFFFFFu}) {
        VideoOutOpenParam param{};
        param.setPriority = badPriorityFlag;
        Require(Throws([&param] {
            return sceVideoOutOpen(SystemUser, MainBus, 0, &param);
        }));
    }

    for (const std::int32_t badPriority : {-1000, -1, 0, 1, 255, 768, 769, 1000, 0x7FFFFFFF}) {
        VideoOutOpenParam param{};
        param.setPriority = 1;
        param.priority = badPriority;
        Require(Throws([&param] {
            return sceVideoOutOpen(SystemUser, MainBus, 0, &param);
        }));
    }

    for (const std::uint32_t badAffinityFlag : {2u, 3u, 10u, 100u, 0xFFFFFFFFu}) {
        VideoOutOpenParam param{};
        param.setAffinity = badAffinityFlag;
        Require(Throws([&param] {
            return sceVideoOutOpen(SystemUser, MainBus, 0, &param);
        }));
    }

    for (const std::uint64_t badAffinity : {0ULL, 0x2000ULL, 0x4000ULL, 0x10000ULL, 0xFFFFFFFFFFFF0000ULL, 0xFFFFFFFFFFFFFFFFULL}) {
        VideoOutOpenParam param{};
        param.setAffinity = 1;
        param.affinity = badAffinity;
        Require(Throws([&param] {
            return sceVideoOutOpen(SystemUser, MainBus, 0, &param);
        }));
    }
}

void TestOpenCloseLifecycle(int& handle) {
    Require(handle > 0);

    Require(sceVideoOutOpen(SystemUser, MainBus, 0, nullptr) == VideoOutErrorResourceBusy);
    Require(sceVideoOutOpen(0, MainBus, 0, nullptr) == VideoOutErrorResourceBusy);

    Require(sceVideoOutClose(handle) == 0);
    Require(sceVideoOutClose(handle) == VideoOutErrorInvalidHandle);

    handle = sceVideoOutOpen(SystemUser, MainBus, 0, nullptr);
    Require(handle > 0);
    Require(sceVideoOutClose(handle) == 0);

    const int userZero = sceVideoOutOpen(0, MainBus, 0, nullptr);
    Require(userZero > 0);
    Require(sceVideoOutClose(userZero) == 0);

    VideoOutOpenParam p1{};
    p1.setPriority = 1;
    p1.priority = 256;
    const int h1 = sceVideoOutOpen(SystemUser, MainBus, 0, &p1);
    Require(h1 > 0);
    Require(sceVideoOutClose(h1) == 0);

    VideoOutOpenParam p2{};
    p2.setPriority = 1;
    p2.priority = 767;
    const int h2 = sceVideoOutOpen(SystemUser, MainBus, 0, &p2);
    Require(h2 > 0);
    Require(sceVideoOutClose(h2) == 0);

    VideoOutOpenParam p3{};
    p3.setAffinity = 1;
    p3.affinity = 0x1FFF;
    const int h3 = sceVideoOutOpen(SystemUser, MainBus, 0, &p3);
    Require(h3 > 0);
    Require(sceVideoOutClose(h3) == 0);

    VideoOutOpenParam p4{};
    p4.setPriority = 1;
    p4.priority = 512;
    p4.setAffinity = 1;
    p4.affinity = 0x3E;
    const int h4 = sceVideoOutOpen(SystemUser, MainBus, 0, &p4);
    Require(h4 > 0);
    Require(sceVideoOutClose(h4) == 0);
}

} // namespace

int main() {
    VideoOutTestEnvironment environment;

    TestInvalidCloseHandles();
    TestInvalidOpenParameters();
    TestInvalidOpenParamStruct();

    int handle = -1;
    try {
        handle = sceVideoOutOpen(SystemUser, MainBus, 0, nullptr);
    } catch (const std::runtime_error& error) {
        if (std::getenv("ANYPS5_REQUIRE_DISPLAY") != nullptr) throw;
        return 77;
    }

    TestOpenCloseLifecycle(handle);

    LibcRunShutdown_nid_postfix();
    return 0;
}
