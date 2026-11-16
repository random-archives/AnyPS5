#include "prx/libkernel/include/Api.hpp"
#include "prx/libc/include/Api.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>

namespace {

constexpr int guestEfault = 14;
constexpr int guestEinval = 22;
constexpr int maximumSignal = 128;

void Require(bool condition) {
    if (!condition) {
        std::abort();
    }
}

void RequireFailure(int result, int error) {
    Require(result == -1);
    Require(*__error_nid_postfix() == error);
}

void AuditInvalidSignals() {
    constexpr int invalidSignals[] = {
        0, -1, -2, -32, -100, 129, 130, 255, 1000, -2147483648, 2147483647
    };

    GuestSignalSet set{};
    Require(sigemptyset_nid_postfix(&set) == 0);

    for (const int signal : invalidSignals) {
        *__error_nid_postfix() = 0;
        RequireFailure(sigaddset_nid_postfix(&set, signal), guestEinval);

        *__error_nid_postfix() = 0;
        RequireFailure(sigdelset_nid_postfix(&set, signal), guestEinval);

        *__error_nid_postfix() = 0;
        RequireFailure(sigismember_nid_postfix(&set, signal), guestEinval);
    }

    Require(set.bits[0] == 0 && set.bits[1] == 0 && set.bits[2] == 0 && set.bits[3] == 0);

    for (const int signal : invalidSignals) {
        *__error_nid_postfix() = 0;
        RequireFailure(sigaddset_nid_postfix(nullptr, signal), guestEinval);

        *__error_nid_postfix() = 0;
        RequireFailure(sigdelset_nid_postfix(nullptr, signal), guestEinval);

        *__error_nid_postfix() = 0;
        RequireFailure(sigismember_nid_postfix(nullptr, signal), guestEinval);
    }

    auto* unmapped = reinterpret_cast<GuestSignalSet*>(std::uintptr_t{0x10});
    for (const int signal : invalidSignals) {
        *__error_nid_postfix() = 0;
        RequireFailure(sigaddset_nid_postfix(unmapped, signal), guestEinval);

        *__error_nid_postfix() = 0;
        RequireFailure(sigdelset_nid_postfix(unmapped, signal), guestEinval);

        *__error_nid_postfix() = 0;
        RequireFailure(sigismember_nid_postfix(unmapped, signal), guestEinval);
    }
}

void AuditPointerFaultsOnValidSignals() {
    constexpr int boundarySignals[] = {1, 2, 31, 32, 33, 64, 65, 96, 97, 127, 128};
    auto* unmappedLow = reinterpret_cast<GuestSignalSet*>(std::uintptr_t{0x10});
    auto* unmappedHigh = reinterpret_cast<GuestSignalSet*>(~std::uintptr_t{0xfff});

    *__error_nid_postfix() = 0;
    RequireFailure(sigemptyset_nid_postfix(nullptr), guestEfault);
    *__error_nid_postfix() = 0;
    RequireFailure(sigfillset_nid_postfix(nullptr), guestEfault);

    *__error_nid_postfix() = 0;
    RequireFailure(sigemptyset_nid_postfix(unmappedLow), guestEfault);
    *__error_nid_postfix() = 0;
    RequireFailure(sigfillset_nid_postfix(unmappedLow), guestEfault);

    *__error_nid_postfix() = 0;
    RequireFailure(sigemptyset_nid_postfix(unmappedHigh), guestEfault);
    *__error_nid_postfix() = 0;
    RequireFailure(sigfillset_nid_postfix(unmappedHigh), guestEfault);

    for (const int signal : boundarySignals) {
        *__error_nid_postfix() = 0;
        RequireFailure(sigaddset_nid_postfix(nullptr, signal), guestEfault);

        *__error_nid_postfix() = 0;
        RequireFailure(sigdelset_nid_postfix(nullptr, signal), guestEfault);

        *__error_nid_postfix() = 0;
        RequireFailure(sigismember_nid_postfix(nullptr, signal), guestEfault);

        *__error_nid_postfix() = 0;
        RequireFailure(sigaddset_nid_postfix(unmappedLow, signal), guestEfault);

        *__error_nid_postfix() = 0;
        RequireFailure(sigdelset_nid_postfix(unmappedLow, signal), guestEfault);

        *__error_nid_postfix() = 0;
        RequireFailure(sigismember_nid_postfix(unmappedLow, signal), guestEfault);

        *__error_nid_postfix() = 0;
        RequireFailure(sigaddset_nid_postfix(unmappedHigh, signal), guestEfault);

        *__error_nid_postfix() = 0;
        RequireFailure(sigdelset_nid_postfix(unmappedHigh, signal), guestEfault);

        *__error_nid_postfix() = 0;
        RequireFailure(sigismember_nid_postfix(unmappedHigh, signal), guestEfault);
    }
}

void AuditSigemptysetAndSigfillset() {
    GuestSignalSet set{{0x12345678u, 0x9abcdef0u, 0xa5a5a5a5u, 0x5a5a5a5au}};
    *__error_nid_postfix() = 0x5a5a;
    Require(sigemptyset_nid_postfix(&set) == 0);
    Require(*__error_nid_postfix() == 0x5a5a);
    Require(set.bits[0] == 0 && set.bits[1] == 0 && set.bits[2] == 0 && set.bits[3] == 0);

    for (int signal = 1; signal <= maximumSignal; ++signal) {
        Require(sigismember_nid_postfix(&set, signal) == 0);
    }

    *__error_nid_postfix() = 0x5a5a;
    Require(sigfillset_nid_postfix(&set) == 0);
    Require(*__error_nid_postfix() == 0x5a5a);
    Require(set.bits[0] == ~0u && set.bits[1] == ~0u && set.bits[2] == ~0u && set.bits[3] == ~0u);

    for (int signal = 1; signal <= maximumSignal; ++signal) {
        Require(sigismember_nid_postfix(&set, signal) == 1);
    }
}

void AuditBitsetProgressionAndBoundaries() {
    GuestSignalSet set{};
    Require(sigemptyset_nid_postfix(&set) == 0);

    for (int signal = 1; signal <= maximumSignal; ++signal) {
        Require(sigismember_nid_postfix(&set, signal) == 0);

        *__error_nid_postfix() = 0x5a5a;
        Require(sigaddset_nid_postfix(&set, signal) == 0);
        Require(*__error_nid_postfix() == 0x5a5a);

        Require(sigismember_nid_postfix(&set, signal) == 1);

        const std::size_t word = static_cast<std::size_t>(signal - 1) >> 5;
        const std::uint32_t bit = 1u << ((signal - 1) & 31);
        Require((set.bits[word] & bit) != 0);
    }

    Require(set.bits[0] == ~0u && set.bits[1] == ~0u && set.bits[2] == ~0u && set.bits[3] == ~0u);

    for (int signal = 1; signal <= maximumSignal; ++signal) {
        Require(sigismember_nid_postfix(&set, signal) == 1);

        *__error_nid_postfix() = 0x5a5a;
        Require(sigdelset_nid_postfix(&set, signal) == 0);
        Require(*__error_nid_postfix() == 0x5a5a);

        Require(sigismember_nid_postfix(&set, signal) == 0);

        const std::size_t word = static_cast<std::size_t>(signal - 1) >> 5;
        const std::uint32_t bit = 1u << ((signal - 1) & 31);
        Require((set.bits[word] & bit) == 0);
    }

    Require(set.bits[0] == 0 && set.bits[1] == 0 && set.bits[2] == 0 && set.bits[3] == 0);
}

void AuditWordBoundaryIsolation() {
    GuestSignalSet set{};
    Require(sigemptyset_nid_postfix(&set) == 0);

    Require(sigaddset_nid_postfix(&set, 32) == 0);
    Require(set.bits[0] == 0x80000000u && set.bits[1] == 0 && set.bits[2] == 0 && set.bits[3] == 0);

    Require(sigaddset_nid_postfix(&set, 33) == 0);
    Require(set.bits[0] == 0x80000000u && set.bits[1] == 1u && set.bits[2] == 0 && set.bits[3] == 0);

    Require(sigaddset_nid_postfix(&set, 64) == 0);
    Require(set.bits[0] == 0x80000000u && set.bits[1] == 0x80000001u && set.bits[2] == 0 && set.bits[3] == 0);

    Require(sigaddset_nid_postfix(&set, 65) == 0);
    Require(set.bits[0] == 0x80000000u && set.bits[1] == 0x80000001u && set.bits[2] == 1u && set.bits[3] == 0);

    Require(sigaddset_nid_postfix(&set, 96) == 0);
    Require(set.bits[0] == 0x80000000u && set.bits[1] == 0x80000001u && set.bits[2] == 0x80000001u && set.bits[3] == 0);

    Require(sigaddset_nid_postfix(&set, 97) == 0);
    Require(set.bits[0] == 0x80000000u && set.bits[1] == 0x80000001u && set.bits[2] == 0x80000001u && set.bits[3] == 1u);

    Require(sigaddset_nid_postfix(&set, 128) == 0);
    Require(set.bits[0] == 0x80000000u && set.bits[1] == 0x80000001u && set.bits[2] == 0x80000001u && set.bits[3] == 0x80000001u);

    Require(sigaddset_nid_postfix(&set, 32) == 0);
    Require(set.bits[0] == 0x80000000u);

    Require(sigdelset_nid_postfix(&set, 32) == 0);
    Require(set.bits[0] == 0 && set.bits[1] == 0x80000001u && set.bits[2] == 0x80000001u && set.bits[3] == 0x80000001u);

    Require(sigdelset_nid_postfix(&set, 32) == 0);
    Require(set.bits[0] == 0);
}

} // namespace

int main() {
    AuditInvalidSignals();
    AuditPointerFaultsOnValidSignals();
    AuditSigemptysetAndSigfillset();
    AuditBitsetProgressionAndBoundaries();
    AuditWordBoundaryIsolation();
    return 0;
}
