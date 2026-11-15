#include "prx/libc/include/Api.hpp"
#include "prx/libkernel/KernelErrors.hpp"
#include "prx/libkernel/include/Api.hpp"
#include "SceTypes.hpp"
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace {

constexpr int PosixEbadf = 9;
constexpr int PosixSigpipe = 13;
constexpr int PosixEfault = 14;
constexpr int PosixEinval = 22;
constexpr int PosixEpipe = 32;
constexpr int PosixEagain = 35;

constexpr int FlagNonblock = 0x00000004;
constexpr int FlagCloexec = 0x00100000;

volatile std::sig_atomic_t gBrokenPipeSignals = 0;

void Require(bool condition) {
    if (!condition) {
        std::abort();
    }
}

void APS5_VABI HandleSigpipe(int signal) {
    if (signal == PosixSigpipe) {
        gBrokenPipeSignals = gBrokenPipeSignals + 1;
    }
}

void TestPipeCreationAndLifecycle() {
    int fds[2] = {-1, -1};
    Require(pipe_nid_postfix(fds) == 0);
    Require(fds[0] >= 0);
    Require(fds[1] >= 0);
    Require(fds[0] != fds[1]);

    const char payload[] = {'h', 'e', 'l', 'l', 'o'};
    Require(write_nid_postfix(fds[1], payload, sizeof(payload)) == static_cast<std::int64_t>(sizeof(payload)));

    char received[16]{};
    Require(read_nid_postfix(fds[0], received, sizeof(received)) == static_cast<std::int64_t>(sizeof(payload)));
    Require(std::memcmp(received, payload, sizeof(payload)) == 0);

    Require(close_nid_postfix(fds[0]) == 0);
    Require(close_nid_postfix(fds[1]) == 0);

    *__error_nid_postfix() = 0;
    Require(close_nid_postfix(fds[0]) == -1);
    Require(*__error_nid_postfix() == PosixEbadf);
    *__error_nid_postfix() = 0;
    Require(close_nid_postfix(fds[1]) == -1);
    Require(*__error_nid_postfix() == PosixEbadf);
}

void TestPipe2FlagsValidation() {
    int fds[2] = {-1, -1};

    for (const int validFlag : {0, FlagCloexec, FlagNonblock, FlagCloexec | FlagNonblock}) {
        Require(pipe2_nid_postfix(fds, validFlag) == 0);
        Require(fds[0] >= 0 && fds[1] >= 0);
        Require(close_nid_postfix(fds[0]) == 0);
        Require(close_nid_postfix(fds[1]) == 0);
    }

    const int invalidFlags[] = {
        0x00000001,
        0x00000002,
        0x00000008,
        0x00000080,
        0x00000100,
        0x00000200,
        0x00000400,
        0x00000800,
        0x00010000,
        0x00020000,
        0x00200000,
        0x01000000,
        static_cast<int>(0x80000000u),
        -1
    };

    for (const int flag : invalidFlags) {
        *__error_nid_postfix() = 0;
        Require(pipe2_nid_postfix(fds, flag) == -1);
        Require(*__error_nid_postfix() == PosixEinval);
    }
}

void TestNullPointerAndPrecedence() {
    *__error_nid_postfix() = 0;
    Require(pipe_nid_postfix(nullptr) == -1);
    Require(*__error_nid_postfix() == PosixEfault);

    *__error_nid_postfix() = 0;
    Require(pipe2_nid_postfix(nullptr, 0) == -1);
    Require(*__error_nid_postfix() == PosixEfault);

    *__error_nid_postfix() = 0;
    Require(pipe2_nid_postfix(nullptr, FlagCloexec) == -1);
    Require(*__error_nid_postfix() == PosixEfault);

    *__error_nid_postfix() = 0;
    Require(pipe2_nid_postfix(nullptr, FlagNonblock) == -1);
    Require(*__error_nid_postfix() == PosixEfault);

    *__error_nid_postfix() = 0;
    Require(pipe2_nid_postfix(nullptr, FlagCloexec | FlagNonblock) == -1);
    Require(*__error_nid_postfix() == PosixEfault);

    *__error_nid_postfix() = 0;
    Require(pipe2_nid_postfix(nullptr, 0x00000001) == -1);
    Require(*__error_nid_postfix() == PosixEinval);

    *__error_nid_postfix() = 0;
    Require(pipe2_nid_postfix(nullptr, static_cast<int>(0x80000000u)) == -1);
    Require(*__error_nid_postfix() == PosixEinval);
}

void TestNonblockingPipeBehavior() {
    int fds[2] = {-1, -1};
    Require(pipe2_nid_postfix(fds, FlagNonblock) == 0);

    char buffer[16]{};
    *__error_nid_postfix() = 0;
    Require(read_nid_postfix(fds[0], buffer, sizeof(buffer)) == -1);
    Require(*__error_nid_postfix() == PosixEagain);

    const char msg[] = {'t', 'e', 's', 't'};
    Require(write_nid_postfix(fds[1], msg, sizeof(msg)) == static_cast<std::int64_t>(sizeof(msg)));

    Require(read_nid_postfix(fds[0], buffer, sizeof(buffer)) == static_cast<std::int64_t>(sizeof(msg)));
    Require(std::memcmp(buffer, msg, sizeof(msg)) == 0);

    *__error_nid_postfix() = 0;
    Require(read_nid_postfix(fds[0], buffer, sizeof(buffer)) == -1);
    Require(*__error_nid_postfix() == PosixEagain);

    Require(close_nid_postfix(fds[0]) == 0);
    Require(close_nid_postfix(fds[1]) == 0);
}

void TestClosedWriteEndEof() {
    int fds[2] = {-1, -1};
    Require(pipe_nid_postfix(fds) == 0);

    const char data[] = {'x', 'y', 'z'};
    Require(write_nid_postfix(fds[1], data, sizeof(data)) == static_cast<std::int64_t>(sizeof(data)));

    Require(close_nid_postfix(fds[1]) == 0);

    char readBuf[16]{};
    Require(read_nid_postfix(fds[0], readBuf, sizeof(readBuf)) == static_cast<std::int64_t>(sizeof(data)));
    Require(std::memcmp(readBuf, data, sizeof(data)) == 0);

    Require(read_nid_postfix(fds[0], readBuf, sizeof(readBuf)) == 0);
    Require(read_nid_postfix(fds[0], readBuf, 1) == 0);

    Require(close_nid_postfix(fds[0]) == 0);
}

void TestClosedReadEndEpipeAndSigpipe() {
    int fds[2] = {-1, -1};
    Require(pipe_nid_postfix(fds) == 0);

    const auto ignore = reinterpret_cast<GuestHandler>(std::uintptr_t{1});
    const auto prevHandler = signal_nid_postfix(PosixSigpipe, ignore);

    Require(close_nid_postfix(fds[0]) == 0);

    const char sample[] = {'b', 'y', 't', 'e'};
    *__error_nid_postfix() = 0;
    Require(write_nid_postfix(fds[1], sample, sizeof(sample)) == -1);
    Require(*__error_nid_postfix() == PosixEpipe);

    *__error_nid_postfix() = 0;
    Require(_write_nid_postfix(fds[1], sample, sizeof(sample)) == -1);
    Require(*__error_nid_postfix() == PosixEpipe);

    Require(write(fds[1], sample, sizeof(sample)) == SCE_KERNEL_ERROR_EPIPE);

    signal_nid_postfix(PosixSigpipe, HandleSigpipe);
    const auto signalsBefore = gBrokenPipeSignals;
    *__error_nid_postfix() = 0;
    Require(write_nid_postfix(fds[1], sample, sizeof(sample)) == -1);
    Require(*__error_nid_postfix() == PosixEpipe);
    Require(gBrokenPipeSignals >= signalsBefore);

    signal_nid_postfix(PosixSigpipe, prevHandler);
    Require(close_nid_postfix(fds[1]) == 0);
}

void TestBufferAndSizeBounds() {
    int fds[2] = {-1, -1};
    Require(pipe_nid_postfix(fds) == 0);

    *__error_nid_postfix() = 0;
    Require(write_nid_postfix(fds[1], nullptr, 10) == -1);
    Require(*__error_nid_postfix() == PosixEfault);

    *__error_nid_postfix() = 0;
    Require(read_nid_postfix(fds[0], nullptr, 10) == -1);
    Require(*__error_nid_postfix() == PosixEfault);

    Require(write_nid_postfix(fds[1], nullptr, 0) == 0);
    Require(read_nid_postfix(fds[0], nullptr, 0) == 0);

    char buf[8]{};
    *__error_nid_postfix() = 0;
    Require(write_nid_postfix(fds[1], buf, -1) == -1);
    Require(*__error_nid_postfix() == PosixEinval);

    *__error_nid_postfix() = 0;
    Require(read_nid_postfix(fds[0], buf, -1) == -1);
    Require(*__error_nid_postfix() == PosixEinval);

    Require(close_nid_postfix(fds[0]) == 0);
    Require(close_nid_postfix(fds[1]) == 0);
}

void TestClosedDescriptorOperations() {
    int fds[2] = {-1, -1};
    Require(pipe_nid_postfix(fds) == 0);
    Require(close_nid_postfix(fds[0]) == 0);
    Require(close_nid_postfix(fds[1]) == 0);

    char buf[8]{};
    *__error_nid_postfix() = 0;
    Require(read_nid_postfix(fds[0], buf, sizeof(buf)) == -1);
    Require(*__error_nid_postfix() == PosixEbadf);

    *__error_nid_postfix() = 0;
    Require(write_nid_postfix(fds[1], buf, sizeof(buf)) == -1);
    Require(*__error_nid_postfix() == PosixEbadf);

    Require(read(fds[0], buf, sizeof(buf)) == SCE_KERNEL_ERROR_EBADF);
    Require(write(fds[1], buf, sizeof(buf)) == SCE_KERNEL_ERROR_EBADF);
}

} // namespace

int main() {
    TestPipeCreationAndLifecycle();
    TestPipe2FlagsValidation();
    TestNullPointerAndPrecedence();
    TestNonblockingPipeBehavior();
    TestClosedWriteEndEof();
    TestClosedReadEndEpipeAndSigpipe();
    TestBufferAndSizeBounds();
    TestClosedDescriptorOperations();
    return 0;
}
