#include "prx/libSceFios2/include/Api.hpp"
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <source_location>

namespace {

constexpr std::int32_t FiosOk = 0;
constexpr std::int32_t FiosErrorUnimplemented = static_cast<std::int32_t>(0x80820000u);
constexpr std::int32_t FiosErrorBadFh = static_cast<std::int32_t>(0x8082000Bu);

void Require(bool condition, const std::source_location location = std::source_location::current()) {
    if (!condition) {
        std::fprintf(stderr, "Require failed at line %u\n", static_cast<unsigned>(location.line()));
        std::fflush(stderr);
        std::abort();
    }
}

void TestPreInitializationBounds() {
    Require(sceFiosArchiveGetDecompressorThreadCount() == 0);
    Require(sceFiosArchiveSetDecompressorThreadCount(0) == 1);
    Require(sceFiosArchiveGetDecompressorThreadCount() == 1);
    Require(sceFiosArchiveSetDecompressorThreadCount(1) == 0);
    Require(sceFiosArchiveGetDecompressorThreadCount() == 0);
    Require(sceFiosArchiveSetDecompressorThreadCount(5) == 0);
    Require(sceFiosArchiveGetDecompressorThreadCount() == 0);
}

void TestDefaultInitializedDecompressorBounds() {
    Require(sceFiosInitialize(nullptr) == FiosOk);
    Require(sceFiosArchiveGetDecompressorThreadCount() == 2);

    Require(sceFiosArchiveSetDecompressorThreadCount(0) == 1);
    Require(sceFiosArchiveGetDecompressorThreadCount() == 1);

    Require(sceFiosArchiveSetDecompressorThreadCount(1) == 1);
    Require(sceFiosArchiveGetDecompressorThreadCount() == 1);

    Require(sceFiosArchiveSetDecompressorThreadCount(2) == 2);
    Require(sceFiosArchiveGetDecompressorThreadCount() == 2);

    Require(sceFiosArchiveSetDecompressorThreadCount(3) == 2);
    Require(sceFiosArchiveGetDecompressorThreadCount() == 2);

    Require(sceFiosArchiveSetDecompressorThreadCount(10) == 2);
    Require(sceFiosArchiveGetDecompressorThreadCount() == 2);

    Require(sceFiosArchiveSetDecompressorThreadCount(255) == 2);
    Require(sceFiosArchiveGetDecompressorThreadCount() == 2);

    sceFiosTerminate();
    Require(sceFiosArchiveGetDecompressorThreadCount() == 0);
}

void TestCustomParamsDecompressorLimits() {
    std::array<std::uint8_t, 0xB0> paramsBuffer{};
    const std::uint32_t sizeHeader = static_cast<std::uint32_t>(paramsBuffer.size()) << 1;
    std::memcpy(paramsBuffer.data(), &sizeHeader, sizeof(sizeHeader));
    paramsBuffer[0x18] = 3;

    const auto* customParams = reinterpret_cast<const SceFiosParams*>(paramsBuffer.data());
    Require(sceFiosInitialize(customParams) == FiosOk);
    Require(sceFiosArchiveGetDecompressorThreadCount() == 3);

    Require(sceFiosArchiveSetDecompressorThreadCount(0) == 1);
    Require(sceFiosArchiveGetDecompressorThreadCount() == 1);

    Require(sceFiosArchiveSetDecompressorThreadCount(1) == 1);
    Require(sceFiosArchiveGetDecompressorThreadCount() == 1);

    Require(sceFiosArchiveSetDecompressorThreadCount(2) == 2);
    Require(sceFiosArchiveGetDecompressorThreadCount() == 2);

    Require(sceFiosArchiveSetDecompressorThreadCount(3) == 3);
    Require(sceFiosArchiveGetDecompressorThreadCount() == 3);

    Require(sceFiosArchiveSetDecompressorThreadCount(4) == 3);
    Require(sceFiosArchiveGetDecompressorThreadCount() == 3);

    Require(sceFiosArchiveSetDecompressorThreadCount(100) == 3);
    Require(sceFiosArchiveGetDecompressorThreadCount() == 3);

    Require(sceFiosArchiveSetDecompressorThreadCount(255) == 3);
    Require(sceFiosArchiveGetDecompressorThreadCount() == 3);
}

void TestUnmountedArchiveErrorHandling() {
    SceFiosSize mountBufferSize = 0;
    Require(sceFiosArchiveGetMountBufferSize(nullptr, "/app0/archive.psarc", &mountBufferSize) == 0);
    Require(sceFiosArchiveGetMountBufferSizeSync(nullptr, "/app0/archive.psarc", &mountBufferSize) == FiosErrorUnimplemented);

    SceFiosFH outArchive = 0;
    Require(sceFiosArchiveMount(nullptr, &outArchive, "/app0/archive.psarc", "/archive", nullptr, 0, nullptr) == 0);
    Require(sceFiosArchiveMountSync(nullptr, &outArchive, "/app0/archive.psarc", "/archive", nullptr, 0, nullptr) == FiosErrorUnimplemented);

    Require(sceFiosArchiveMountWithOrder(nullptr, &outArchive, "/app0/archive.psarc", "/archive", nullptr, 0, nullptr, 1) == 0);
    Require(sceFiosArchiveMountWithOrderSync(nullptr, &outArchive, "/app0/archive.psarc", "/archive", nullptr, 0, nullptr, 1) == FiosErrorUnimplemented);

    Require(sceFiosArchiveUnmount(nullptr, 42) == 0);
    Require(sceFiosArchiveUnmountSync(nullptr, 42) == FiosErrorUnimplemented);

    char readBuffer[64]{};
    Require(sceFiosFHCloseSync(nullptr, 999) == FiosErrorBadFh);
    Require(sceFiosFHReadSync(nullptr, 999, readBuffer, sizeof(readBuffer)) == FiosErrorBadFh);
    Require(sceFiosFHSeek(999, 0, 0) == FiosErrorBadFh);
    Require(sceFiosFHGetSize(999) == FiosErrorBadFh);
}

void TestTerminationReset() {
    sceFiosTerminate();
    Require(sceFiosArchiveGetDecompressorThreadCount() == 0);
}

} // namespace

int main() {
    TestPreInitializationBounds();
    TestDefaultInitializedDecompressorBounds();
    TestCustomParamsDecompressorLimits();
    TestUnmountedArchiveErrorHandling();
    TestTerminationReset();
    return 0;
}
