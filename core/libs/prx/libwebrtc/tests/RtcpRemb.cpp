#include "prx/libwebrtc/libwebrtc.hpp"
#include "prx/libc/libc.hpp"
#include "prx/libc/include/ApplicationHeap.hpp"

#include <array>
#include <bit>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <stdexcept>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif
#include <vector>

namespace {

int allocations = 0;
int frees = 0;
bool failAllocation = false;
void Require(bool condition) {
    if (!condition) throw std::runtime_error("Remb assertion failed");
}

enum class Access { ReadWrite, ReadOnly, None };

struct GuardedPages {
    std::size_t pageSize = 0;
    std::size_t size = 0;
    unsigned char* base = nullptr;

    explicit GuardedPages(std::size_t bytes = 1) {
#ifdef _WIN32
        SYSTEM_INFO info{};
        GetSystemInfo(&info);
        pageSize = info.dwPageSize;
        size = (bytes + pageSize - 1) / pageSize * pageSize;
        base = static_cast<unsigned char*>(VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
#else
        pageSize = static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
        size = (bytes + pageSize - 1) / pageSize * pageSize;
        void* mapped = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        base = mapped == MAP_FAILED ? nullptr : static_cast<unsigned char*>(mapped);
#endif
        Require(base != nullptr);
    }

    ~GuardedPages() {
#ifdef _WIN32
        VirtualFree(base, 0, MEM_RELEASE);
#else
        munmap(base, size);
#endif
    }

    GuardedPages(const GuardedPages&) = delete;
    GuardedPages& operator=(const GuardedPages&) = delete;

    template<typename T> T* As(std::size_t offset = 0) const { return reinterpret_cast<T*>(base + offset); }

    void Protect(Access access, std::size_t offset = 0) const {
#ifdef _WIN32
        DWORD previous = 0;
        const DWORD mode = access == Access::ReadWrite ? PAGE_READWRITE : access == Access::ReadOnly ? PAGE_READONLY : PAGE_NOACCESS;
        Require(VirtualProtect(base + offset, size - offset, mode, &previous) != 0);
#else
        const int mode = access == Access::ReadWrite ? PROT_READ | PROT_WRITE : access == Access::ReadOnly ? PROT_READ : PROT_NONE;
        Require(mprotect(base + offset, size - offset, mode) == 0);
#endif
    }
};

template<typename TAction> bool ThrowsInvalidArgument(TAction action) {
    try {
        action();
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}
void* APS5_VABI Allocate_nid_no_patch(std::size_t size) {
    if (failAllocation) throw std::bad_alloc();
    ++allocations;
    return std::malloc(size);
}
void APS5_VABI Free_nid_no_patch(void* pointer) { if (pointer) ++frees; std::free(pointer); }
void* APS5_VABI Calloc_nid_no_patch(std::size_t count, std::size_t size) { return std::calloc(count, size); }
void* APS5_VABI Reallocate_nid_no_patch(void* pointer, std::size_t size) { return std::realloc(pointer, size); }
void* APS5_VABI Align_nid_no_patch(std::size_t, std::size_t size) { return Allocate_nid_no_patch(size); }
void* APS5_VABI Realign_nid_no_patch(void* pointer, std::size_t size, std::size_t) { return Reallocate_nid_no_patch(pointer, size); }
int APS5_VABI PosixAlign_nid_no_patch(void** pointer, std::size_t alignment, std::size_t size) {
    *pointer = Align_nid_no_patch(alignment, size);
    return *pointer ? 0 : 12;
}


std::vector<std::uint8_t> Packet(std::uint8_t count, std::uint8_t exponent = 0, std::uint32_t mantissa = 0) {
    std::vector<std::uint8_t> bytes {0x8f, 206, static_cast<std::uint8_t>((4 + count) >> 8), static_cast<std::uint8_t>(4 + count),
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
        'R', 'E', 'M', 'B', count, static_cast<std::uint8_t>((exponent << 2) | (mantissa >> 16)),
        static_cast<std::uint8_t>(mantissa >> 8), static_cast<std::uint8_t>(mantissa)};
    for (unsigned i = 0; i < count; ++i) {
        bytes.insert(bytes.end(), {0x11, 0x22, 0x33, static_cast<std::uint8_t>(i)});
    }
    return bytes;
}

bool Parse(WebRtcRtcpRemb& remb, const std::vector<std::uint8_t>& bytes) {
    WebRtcRtcpCommonHeader header {};
    Require(_ZN6webrtc4rtcp12CommonHeader5ParseEPKhm_nid_postfix(&header, bytes.data(), bytes.size()));
    return _ZN6webrtc4rtcp4Remb5ParseERKNS0_12CommonHeaderE_nid_postfix(&remb, &header);
}

void TestValidAndReplacement() {
    const int before = allocations;
    WebRtcRtcpRemb remb {};
    remb.vtable = &remb;
    Require(Parse(remb, Packet(0)));
    Require(remb.vtable == &remb && remb.senderSsrc == 0x12345678 && remb.mediaSsrc == 0x9abcdef0);
    Require(remb.bitrateBps == 0 && remb.ssrcBegin == nullptr && allocations == before);
    Require(Parse(remb, Packet(3, 5, 0x3ffff)));
    Require(remb.bitrateBps == (0x3ffffLL << 5));
    Require(remb.ssrcEnd - remb.ssrcBegin == 3 && remb.ssrcCapacityEnd - remb.ssrcBegin == 3);
    for (int i = 0; i < 3; ++i) Require(remb.ssrcBegin[i] == 0x11223300u + i);
    auto* storage = remb.ssrcBegin;
    Require(Parse(remb, Packet(1, 63, 0)));
    Require(remb.ssrcBegin == storage && remb.ssrcEnd == storage + 1 && remb.bitrateBps == 0);
    Require(Parse(remb, Packet(0)));
    Require(remb.ssrcEnd == storage && remb.ssrcCapacityEnd == storage + 3);
    Require(Parse(remb, Packet(255, 45, 0x3ffff)));
    Require(remb.ssrcEnd - remb.ssrcBegin == 255 && remb.ssrcBegin[254] == 0x112233fe);
    Require(remb.bitrateBps == (0x3ffffLL << 45));
    _ZdlPv_nid_postfix(remb.ssrcBegin);
}

void TestFailures() {
    WebRtcRtcpRemb remb {};
    Require(Parse(remb, Packet(2)));
    auto* storage = remb.ssrcBegin;
    remb.senderSsrc = 17; remb.mediaSsrc = 18; remb.bitrateBps = 19;
    auto unchanged = [&] {
        Require(remb.senderSsrc == 17 && remb.mediaSsrc == 18 && remb.bitrateBps == 19);
        Require(remb.ssrcBegin == storage && remb.ssrcEnd == storage + 2 && storage[1] == 0x11223301);
    };
    auto wrong = Packet(0); wrong[12] = 'X';
    Require(!Parse(remb, wrong)); unchanged();
    auto shortPacket = Packet(0); shortPacket.resize(16); shortPacket[3] = 3;
    Require(!Parse(remb, shortPacket)); unchanged();
    auto mismatch = Packet(0); mismatch[16] = 1;
    Require(!Parse(remb, mismatch)); unchanged();
    mismatch = Packet(1); mismatch[16] = 0;
    Require(!Parse(remb, mismatch)); unchanged();
    for (auto pair : {std::pair<unsigned, unsigned>{63, 1}, {63, 2}, {47, 0x3ffff}}) {
        Require(!Parse(remb, Packet(0, pair.first, pair.second)));
        Require(remb.senderSsrc == 0x12345678 && remb.mediaSsrc == 0x9abcdef0);
        Require(remb.bitrateBps == std::bit_cast<std::int64_t>(static_cast<std::uint64_t>(pair.second) << pair.first));
        Require(remb.ssrcBegin == storage && remb.ssrcEnd == storage + 2 && storage[1] == 0x11223301);
    }
    failAllocation = true;
    bool threw = false;
    try { Parse(remb, Packet(3)); } catch (const std::bad_alloc&) { threw = true; }
    failAllocation = false;
    Require(threw && remb.ssrcBegin == storage && remb.ssrcEnd == storage && remb.ssrcCapacityEnd == storage + 2);
    _ZdlPv_nid_postfix(storage);
}

void TestReleaseAndNulls() {
    WebRtcRtcpRemb remb {};
    auto bytes = Packet(0);
    bytes[0] = 0x81; bytes[1] = 200;
    Require(Parse(remb, bytes));
    WebRtcRtcpCommonHeader header {};
    auto invalid = [&](WebRtcRtcpRemb* object, const WebRtcRtcpCommonHeader* common) {
        bool threw = false;
        try { _ZN6webrtc4rtcp4Remb5ParseERKNS0_12CommonHeaderE_nid_postfix(object, common); }
        catch (const std::invalid_argument&) { threw = true; }
        Require(threw);
    };
    invalid(nullptr, &header);
    invalid(&remb, nullptr);
    Require(!_ZN6webrtc4rtcp4Remb5ParseERKNS0_12CommonHeaderE_nid_postfix(&remb, &header));
    header.payloadSize = 16;
    invalid(&remb, &header);
}

void TestInaccessibleGuestPointersThrow() {
    GuardedPages hidden;
    hidden.Protect(Access::None);
    auto parse = [](WebRtcRtcpRemb* remb, const WebRtcRtcpCommonHeader* header) { return _ZN6webrtc4rtcp4Remb5ParseERKNS0_12CommonHeaderE_nid_postfix(remb, header); };
    WebRtcRtcpRemb remb {};
    Require(ThrowsInvalidArgument([&] { parse(&remb, hidden.As<WebRtcRtcpCommonHeader>()); }));
    const auto bytes = Packet(2);
    WebRtcRtcpCommonHeader header {};
    Require(_ZN6webrtc4rtcp12CommonHeader5ParseEPKhm_nid_postfix(&header, bytes.data(), bytes.size()));
    WebRtcRtcpCommonHeader moved = header;
    moved.payload = hidden.As<std::uint8_t>();
    Require(ThrowsInvalidArgument([&] { parse(&remb, &moved); }));
    GuardedPages tail(2 * hidden.pageSize);
    std::memcpy(tail.As<std::uint8_t>(tail.pageSize - 16), header.payload, 16);
    tail.Protect(Access::None, tail.pageSize);
    moved.payload = tail.As<std::uint8_t>(tail.pageSize - 16);
    Require(ThrowsInvalidArgument([&] { parse(&remb, &moved); }));
    Require(remb.senderSsrc == 0 && remb.bitrateBps == 0 && remb.ssrcBegin == nullptr);

    GuardedPages readOnly;
    *readOnly.As<WebRtcRtcpRemb>() = WebRtcRtcpRemb {};
    readOnly.Protect(Access::ReadOnly);
    Require(ThrowsInvalidArgument([&] { parse(readOnly.As<WebRtcRtcpRemb>(), &header); }));

    GuardedPages storage;
    storage.As<std::uint32_t>()[0] = 7;
    storage.As<std::uint32_t>()[1] = 8;
    storage.Protect(Access::ReadOnly);
    remb.ssrcBegin = storage.As<std::uint32_t>();
    remb.ssrcEnd = remb.ssrcBegin + 2;
    remb.ssrcCapacityEnd = remb.ssrcBegin + 2;
    const WebRtcRtcpRemb before = remb;
    Require(ThrowsInvalidArgument([&] { parse(&remb, &header); }));
    Require(std::memcmp(&remb, &before, sizeof(remb)) == 0 && storage.As<std::uint32_t>()[1] == 8);
}

}

int main() {
    const std::array<void*, 10> api {
        reinterpret_cast<void*>(&Allocate_nid_no_patch), reinterpret_cast<void*>(&Free_nid_no_patch),
        reinterpret_cast<void*>(&Calloc_nid_no_patch), reinterpret_cast<void*>(&Reallocate_nid_no_patch),
        reinterpret_cast<void*>(&Align_nid_no_patch), reinterpret_cast<void*>(&Realign_nid_no_patch),
        reinterpret_cast<void*>(&PosixAlign_nid_no_patch), nullptr, nullptr, nullptr};
    ApplicationHeapRegister_nid_no_patch(api.data());
    try {
        TestValidAndReplacement();
        TestFailures();
        TestReleaseAndNulls();
        TestInaccessibleGuestPointersThrow();
        Require(allocations == frees);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
