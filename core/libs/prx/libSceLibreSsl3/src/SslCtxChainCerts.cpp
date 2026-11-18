#include "prx/libSceLibreSsl3/libSceLibreSsl3.hpp"
#include "prx/libc/include/Api.hpp"
#include "prx/libc/include/General.hpp"

#include <atomic>
#include <cstdint>

namespace {

void APS5_VABI ReleaseCertificate(void* pointer) {
    if (!pointer) return;
    auto* const certificate = static_cast<LibreSsl3X509*>(pointer);
    std::atomic_ref<std::int32_t> count(certificate->references);
    if (count.fetch_sub(1, std::memory_order_acq_rel) <= 1) {
        free_nid_postfix(certificate);
    }
}

}

extern "C" {

int APS5_VABI SSL_CTX_add0_chain_cert_nid_postfix(LibreSsl3SslCtx* context, LibreSsl3X509* certificate) {
    if (!context || !certificate) return 0;
    auto* stack = static_cast<LibreSslStack*>(context->extraCerts);
    if (!stack) {
        stack = sk_new_nid_postfix(nullptr);
        if (!stack) return 0;
        context->extraCerts = stack;
    }
    if (sk_push_nid_postfix(stack, certificate)) return 1;
    return 0;
}

int APS5_VABI SSL_CTX_clear_chain_certs_nid_postfix(LibreSsl3SslCtx* context) {
    if (!context) return 0;
    auto* const stack = static_cast<LibreSslStack*>(context->extraCerts);
    if (stack) {
        sk_pop_free_nid_postfix(stack, ReleaseCertificate);
        context->extraCerts = nullptr;
    }
    return 1;
}

}
