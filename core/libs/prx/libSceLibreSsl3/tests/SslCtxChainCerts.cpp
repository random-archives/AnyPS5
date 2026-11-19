#include "prx/libSceLibreSsl3/libSceLibreSsl3.hpp"
#include "prx/libc/libc.hpp"
#include "prx/libc/include/ApplicationHeap.hpp"
#include "prx/libc/include/general/VabiMacros.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <source_location>
#include <stdexcept>

namespace {

bool failAllocation = false;
std::size_t activeAllocations = 0;

void Require(bool condition, std::source_location location = std::source_location::current()) {
    if (condition) return;
    std::fprintf(stderr, "LibreSSL3 SSL_CTX chain certs check failed at line %u\n", location.line());
    std::abort();
}

void* APS5_VABI Allocate_nid_no_patch(std::size_t size) {
    if (failAllocation) return nullptr;
    ++activeAllocations;
    return std::malloc(size);
}

void APS5_VABI Free_nid_no_patch(void* pointer) {
    if (pointer) {
        --activeAllocations;
        std::free(pointer);
    }
}

void* APS5_VABI Callocate_nid_no_patch(std::size_t count, std::size_t size) {
    if (failAllocation) return nullptr;
    ++activeAllocations;
    return std::calloc(count, size);
}

void* APS5_VABI Reallocate_nid_no_patch(void* pointer, std::size_t size) {
    if (failAllocation) return nullptr;
    if (!pointer) ++activeAllocations;
    return std::realloc(pointer, size);
}

void* APS5_VABI UnexpectedAlign_nid_no_patch(std::size_t, std::size_t) {
    throw std::runtime_error("unexpected aligned allocation");
}

void* APS5_VABI UnexpectedRealign_nid_no_patch(void*, std::size_t, std::size_t) {
    throw std::runtime_error("unexpected aligned reallocation");
}

int APS5_VABI UnexpectedPosixAlign_nid_no_patch(void**, std::size_t, std::size_t) {
    throw std::runtime_error("unexpected posix_memalign");
}

void TestNullArguments() {
    LibreSsl3SslCtx context{};
    LibreSsl3X509 cert{};
    cert.references = 1;

    Require(SSL_CTX_add0_chain_cert_nid_postfix(nullptr, &cert) == 0);
    Require(SSL_CTX_add0_chain_cert_nid_postfix(&context, nullptr) == 0);
    Require(context.extraCerts == nullptr);
    Require(cert.references == 1);

    Require(SSL_CTX_clear_chain_certs_nid_postfix(nullptr) == 0);
}

void TestAddAndClear() {
    LibreSsl3SslCtx context{};

    Require(SSL_CTX_clear_chain_certs_nid_postfix(&context) == 1);
    Require(context.extraCerts == nullptr);

    auto* cert1 = static_cast<LibreSsl3X509*>(Allocate_nid_no_patch(sizeof(LibreSsl3X509)));
    cert1->certInfo = nullptr;
    cert1->sigAlg = nullptr;
    cert1->signature = nullptr;
    cert1->valid = 1;
    cert1->references = 1;

    Require(SSL_CTX_add0_chain_cert_nid_postfix(&context, cert1) == 1);
    auto* stack = static_cast<LibreSslStack*>(context.extraCerts);
    Require(stack != nullptr);
    Require(stack->num == 1);
    Require(stack->data[0] == reinterpret_cast<char*>(cert1));
    Require(cert1->references == 1);

    auto* cert2 = static_cast<LibreSsl3X509*>(Allocate_nid_no_patch(sizeof(LibreSsl3X509)));
    cert2->certInfo = nullptr;
    cert2->sigAlg = nullptr;
    cert2->signature = nullptr;
    cert2->valid = 1;
    cert2->references = 3;

    Require(SSL_CTX_add0_chain_cert_nid_postfix(&context, cert2) == 1);
    Require(context.extraCerts == stack);
    Require(stack->num == 2);
    Require(stack->data[1] == reinterpret_cast<char*>(cert2));
    Require(cert2->references == 3);

    std::array<LibreSsl3X509*, 6> additional{};
    for (std::size_t i = 0; i < additional.size(); ++i) {
        additional[i] = static_cast<LibreSsl3X509*>(Allocate_nid_no_patch(sizeof(LibreSsl3X509)));
        additional[i]->certInfo = nullptr;
        additional[i]->sigAlg = nullptr;
        additional[i]->signature = nullptr;
        additional[i]->valid = 1;
        additional[i]->references = 1;
        Require(SSL_CTX_add0_chain_cert_nid_postfix(&context, additional[i]) == 1);
    }
    stack = static_cast<LibreSslStack*>(context.extraCerts);
    Require(stack->num == 8);

    Require(SSL_CTX_clear_chain_certs_nid_postfix(&context) == 1);
    Require(context.extraCerts == nullptr);
    Require(cert2->references == 2);
    Free_nid_no_patch(cert2);

    Require(SSL_CTX_clear_chain_certs_nid_postfix(&context) == 1);
    Require(context.extraCerts == nullptr);
}

void TestAllocationFailure() {
    LibreSsl3SslCtx context{};
    auto* cert = static_cast<LibreSsl3X509*>(Allocate_nid_no_patch(sizeof(LibreSsl3X509)));
    cert->certInfo = nullptr;
    cert->sigAlg = nullptr;
    cert->signature = nullptr;
    cert->valid = 1;
    cert->references = 1;

    failAllocation = true;
    Require(SSL_CTX_add0_chain_cert_nid_postfix(&context, cert) == 0);
    Require(context.extraCerts == nullptr);
    Require(cert->references == 1);
    failAllocation = false;

    Require(SSL_CTX_add0_chain_cert_nid_postfix(&context, cert) == 1);
    Require(SSL_CTX_clear_chain_certs_nid_postfix(&context) == 1);
    Require(context.extraCerts == nullptr);
}

void TestAllocationTracking() {
    Require(activeAllocations == 0);
}

}

int main() {
    std::array<void*, 7> api{
        reinterpret_cast<void*>(Allocate_nid_no_patch),
        reinterpret_cast<void*>(Free_nid_no_patch),
        reinterpret_cast<void*>(Callocate_nid_no_patch),
        reinterpret_cast<void*>(Reallocate_nid_no_patch),
        reinterpret_cast<void*>(UnexpectedAlign_nid_no_patch),
        reinterpret_cast<void*>(UnexpectedRealign_nid_no_patch),
        reinterpret_cast<void*>(UnexpectedPosixAlign_nid_no_patch)
    };
    ApplicationHeapRegister_nid_no_patch(api.data());

    TestNullArguments();
    TestAddAndClear();
    TestAllocationFailure();
    TestAllocationTracking();

    std::puts("libSceLibreSsl3 SSL_CTX chain certs tests passed");
    return 0;
}
