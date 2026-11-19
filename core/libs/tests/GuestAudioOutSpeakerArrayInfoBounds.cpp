#include "prx/libSceAudioOut/include/AudioOut.hpp"
#include "prx/libSceAudioOut/include/AudioOut2.hpp"
#include "SceTypes.hpp"
#include "prx/libc/include/Shutdown.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace {

struct GuestVbapParams {
    const AudioOut2Position* positions;
    std::uint32_t numSpeakers;
    std::uint8_t is3d;
    void* memory;
    std::size_t memorySize;
    std::uint32_t reserved;
};

constexpr int errorInvalidParam = static_cast<int>(0x80268001);
constexpr int errorNotInitialized = static_cast<int>(0x80268006);
constexpr int errorInvalidPointer = static_cast<int>(0x8026800C);

void Require(bool condition) {
    if (!condition) {
        std::abort();
    }
}

void TestSpeakerInfoUninitialized() {
    AudioOut2SpeakerInfo info{};
    Require(sceAudioOut2GetSpeakerInfo(&info, 0) == errorNotInitialized || sceAudioOut2GetSpeakerInfo(&info, 0) == 0);
}

void TestInitLifecycle() {
    Require(sceAudioOutInit() == 0 || sceAudioOutInit() == static_cast<int>(0x8026000e));
    Require(sceAudioOut2Initialize() == 0 || sceAudioOut2Initialize() == static_cast<int>(0x80268004));
}

void TestSpeakerInfoParameterBounds() {
    Require(sceAudioOut2GetSpeakerInfo(nullptr, 0) == errorInvalidPointer);
    Require(sceAudioOut2GetSpeakerInfo(nullptr, 1) == errorInvalidPointer);
    Require(sceAudioOut2GetSpeakerInfo(nullptr, 2) == errorInvalidPointer);

    AudioOut2SpeakerInfo info{};
    Require(sceAudioOut2GetSpeakerInfo(&info, 2) == errorInvalidParam);
    Require(sceAudioOut2GetSpeakerInfo(&info, 5) == errorInvalidParam);
    Require(sceAudioOut2GetSpeakerInfo(&info, 0xFFFFFFFFu) == errorInvalidParam);

    Require(sceAudioOut2GetSpeakerInfo(&info, 0) == 0);
    Require(info.type == 0);
    Require(info.available_bits == 0x3);
    Require(info.speaker_angle[0].azimuth == 30);
    Require(info.speaker_angle[0].elevation == 0);
    Require(info.speaker_angle[1].azimuth == -30);
    Require(info.speaker_angle[1].elevation == 0);
    for (std::size_t i = 2; i < 16; ++i) {
        Require(info.speaker_angle[i].azimuth == -360);
        Require(info.speaker_angle[i].elevation == -360);
    }

    Require(sceAudioOut2GetSpeakerInfo(&info, 1) == 0);
    Require(info.type == 0);
    Require(info.available_bits == 0x3);
}

void TestSpeakerArrayMemorySize() {
    bool threwZero = false;
    try {
        sceAudioOut2GetSpeakerArrayMemorySize(0, 0, 0);
    } catch (...) {
        threwZero = true;
    }
    Require(threwZero);

    bool threwOversized = false;
    try {
        sceAudioOut2GetSpeakerArrayMemorySize(33, 0, 0);
    } catch (...) {
        threwOversized = true;
    }
    Require(threwOversized);

    const std::size_t size1 = sceAudioOut2GetSpeakerArrayMemorySize(1, 0, 0);
    Require(size1 >= sizeof(AudioOut2Position));

    const std::size_t size2 = sceAudioOut2GetSpeakerArrayMemorySize(2, 1, 1);
    Require(size2 == size1);

    const std::size_t size32 = sceAudioOut2GetSpeakerArrayMemorySize(32, 1, 0);
    Require(size32 == size1);
}

void TestSpeakerArrayCreateParameterBounds() {
    alignas(16) std::array<std::uint8_t, 512> memory{};
    std::array<AudioOut2Position, 2> positions = {{{1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}}};
    AudioOut2SpeakerArrayHandle handle = nullptr;

    bool threw = false;
    try {
        sceAudioOut2SpeakerArrayCreate(nullptr, nullptr, nullptr);
    } catch (...) {
        threw = true;
    }
    Require(threw);

    threw = false;
    try {
        sceAudioOut2SpeakerArrayCreate(&handle, nullptr, nullptr);
    } catch (...) {
        threw = true;
    }
    Require(threw);

    GuestVbapParams badPositions{nullptr, 2, 1, memory.data(), memory.size(), 0};
    threw = false;
    try {
        sceAudioOut2SpeakerArrayCreate(&handle, &badPositions, nullptr);
    } catch (...) {
        threw = true;
    }
    Require(threw);

    GuestVbapParams zeroSpeakers{positions.data(), 0, 1, memory.data(), memory.size(), 0};
    threw = false;
    try {
        sceAudioOut2SpeakerArrayCreate(&handle, &zeroSpeakers, nullptr);
    } catch (...) {
        threw = true;
    }
    Require(threw);

    GuestVbapParams overSpeakers{positions.data(), 33, 1, memory.data(), memory.size(), 0};
    threw = false;
    try {
        sceAudioOut2SpeakerArrayCreate(&handle, &overSpeakers, nullptr);
    } catch (...) {
        threw = true;
    }
    Require(threw);

    GuestVbapParams nullMemory{positions.data(), 2, 1, nullptr, memory.size(), 0};
    threw = false;
    try {
        sceAudioOut2SpeakerArrayCreate(&handle, &nullMemory, nullptr);
    } catch (...) {
        threw = true;
    }
    Require(threw);

    GuestVbapParams shortMemory{positions.data(), 2, 1, memory.data(), 8, 0};
    threw = false;
    try {
        sceAudioOut2SpeakerArrayCreate(&handle, &shortMemory, nullptr);
    } catch (...) {
        threw = true;
    }
    Require(threw);

    GuestVbapParams unalignedMemory{positions.data(), 2, 1, memory.data() + 1, memory.size() - 1, 0};
    threw = false;
    try {
        sceAudioOut2SpeakerArrayCreate(&handle, &unalignedMemory, nullptr);
    } catch (...) {
        threw = true;
    }
    Require(threw);
}

void TestSpeakerArrayLifecycleAndCoefficients() {
    alignas(16) std::array<std::uint8_t, 512> memory{};
    std::array<AudioOut2Position, 2> positions = {{{1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}}};
    AudioOut2SpeakerArrayHandle handle = nullptr;

    GuestVbapParams params{positions.data(), 2, 1, memory.data(), memory.size(), 0};
    Require(sceAudioOut2SpeakerArrayCreate(&handle, &params, nullptr) == 0);
    Require(handle != nullptr);

    bool threw = false;
    try {
        sceAudioOut2GetSpeakerArrayCoefficients(handle, AudioOut2Position{0.0f, 0.0f, 1.0f}, 0.0f, nullptr, 2, 0, 0.0f);
    } catch (...) {
        threw = true;
    }
    Require(threw);

    std::array<float, 2> coeffs{};
    threw = false;
    try {
        sceAudioOut2GetSpeakerArrayCoefficients(handle, AudioOut2Position{0.0f, 0.0f, 1.0f}, 0.0f, coeffs.data(), 1, 0, 0.0f);
    } catch (...) {
        threw = true;
    }
    Require(threw);

    threw = false;
    try {
        sceAudioOut2GetSpeakerArrayCoefficients(handle, AudioOut2Position{0.0f, 0.0f, 1.0f}, 0.0f, coeffs.data(), 3, 0, 0.0f);
    } catch (...) {
        threw = true;
    }
    Require(threw);

    Require(sceAudioOut2GetSpeakerArrayCoefficients(handle, AudioOut2Position{0.0f, 0.0f, 1.0f}, 0.0f, coeffs.data(), 2, 0, 0.0f) == 0);
    Require(coeffs[0] == 1.0f);
    Require(coeffs[1] == 1.0f);

    threw = false;
    try {
        sceAudioOut2GetSpeakerArrayAmbisonicsCoefficients(handle, 0x40, nullptr, 2);
    } catch (...) {
        threw = true;
    }
    Require(threw);

    threw = false;
    try {
        sceAudioOut2GetSpeakerArrayAmbisonicsCoefficients(handle, 0x40, coeffs.data(), 1);
    } catch (...) {
        threw = true;
    }
    Require(threw);

    threw = false;
    try {
        sceAudioOut2GetSpeakerArrayAmbisonicsCoefficients(handle, 0, coeffs.data(), 2);
    } catch (...) {
        threw = true;
    }
    Require(threw);

    threw = false;
    try {
        sceAudioOut2GetSpeakerArrayAmbisonicsCoefficients(handle, 0x40 | 36, coeffs.data(), 2);
    } catch (...) {
        threw = true;
    }
    Require(threw);

    Require(sceAudioOut2GetSpeakerArrayAmbisonicsCoefficients(handle, 0x40, coeffs.data(), 2) == 0);

    Require(sceAudioOut2SpeakerArrayDestroy(handle) == 0);

    threw = false;
    try {
        sceAudioOut2GetSpeakerArrayCoefficients(handle, AudioOut2Position{}, 0.0f, coeffs.data(), 2, 0, 0.0f);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    Require(threw);

    threw = false;
    try {
        sceAudioOut2SpeakerArrayDestroy(handle);
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    Require(threw);
}

} // namespace

int main() {
    TestSpeakerInfoUninitialized();
    TestInitLifecycle();
    TestSpeakerInfoParameterBounds();
    TestSpeakerArrayMemorySize();
    TestSpeakerArrayCreateParameterBounds();
    TestSpeakerArrayLifecycleAndCoefficients();

    LibcRunShutdown_nid_postfix();
    return 0;
}
