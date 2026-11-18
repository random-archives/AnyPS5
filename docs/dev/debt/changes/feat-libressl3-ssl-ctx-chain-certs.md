# libSceLibreSsl3

## add Functional

- [SSL_CTX_add0_chain_cert and SSL_CTX_clear_chain_certs](../../../core/libs/prx/libSceLibreSsl3/src/SslCtxChainCerts.cpp) (libSceLibreSsl3) - `SSL_CTX_add0_chain_cert` appends an X509 certificate to `context->extraCerts` without incrementing its reference count (consuming the reference), allocating a new `LibreSslStack` if null. `SSL_CTX_clear_chain_certs` frees the stack via `sk_pop_free` decrementing certificate references and freeing certificate resources on the final reference, clearing `context->extraCerts` to null, returning 1 on success or 0 if context is null. Unverified on console.
