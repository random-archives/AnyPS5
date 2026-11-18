#include "prx/libc/include/Api.hpp"
#include "prx/libkernel/include/Api.hpp"
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <stdexcept>

namespace {

constexpr int PosixEinval = 22;
constexpr int ConcurrentThreadCount = 8;

void Require(bool condition) {
    if (!condition) {
        std::abort();
    }
}

template <typename TCall>
bool Throws(TCall call) {
    try {
        call();
    } catch (const std::exception&) {
        return true;
    }
    return false;
}

int gDummyCalls = 0;
void APS5_VABI DummyRoutine() {
    ++gDummyCalls;
}

int gCountCalls = 0;
void APS5_VABI CountRoutine() {
    ++gCountCalls;
}

int gThrowAttempts = 0;
void APS5_VABI ThrowRoutine() {
    ++gThrowAttempts;
    throw std::runtime_error("simulated failure in once initialization");
}

int gSuccessCalls = 0;
void APS5_VABI SuccessRoutine() {
    ++gSuccessCalls;
}

GuestOnce gInnerOnce = {0, nullptr};
int gOuterCalls = 0;
int gInnerCalls = 0;

void APS5_VABI InnerRoutine() {
    ++gInnerCalls;
}

void APS5_VABI OuterRoutine() {
    ++gOuterCalls;
    Require(pthread_once_nid_postfix(&gInnerOnce, InnerRoutine) == 0);
}

std::atomic<int> gConcurrentCalls{0};
GuestOnce gConcurrentOnce = {0, nullptr};

void APS5_VABI ConcurrentRoutine() {
    gConcurrentCalls.fetch_add(1, std::memory_order_relaxed);
    scePthreadYield();
}

struct WorkerResult {
    std::atomic<int> returnCode{-1};
};

void* APS5_VABI ConcurrencyWorkerThread(void* arg) {
    auto* result = static_cast<WorkerResult*>(arg);
    const int rc = pthread_once_nid_postfix(&gConcurrentOnce, ConcurrentRoutine);
    result->returnCode.store(rc, std::memory_order_release);
    return nullptr;
}

void TestNullParameterValidation() {
    Require(pthread_once_nid_postfix(nullptr, DummyRoutine) == PosixEinval);

    GuestOnce once = {0, nullptr};
    Require(pthread_once_nid_postfix(&once, nullptr) == PosixEinval);
    Require(once.state == 0);

    Require(pthread_once_nid_postfix(nullptr, nullptr) == PosixEinval);
    Require(gDummyCalls == 0);
}

void TestStateTransitionsAndInitLifecycle() {
    GuestOnce once = {0, nullptr};
    gCountCalls = 0;

    Require(pthread_once_nid_postfix(&once, CountRoutine) == 0);
    Require(gCountCalls == 1);
    Require(once.state == 1);

    for (int i = 0; i < 5; ++i) {
        Require(pthread_once_nid_postfix(&once, CountRoutine) == 0);
        Require(gCountCalls == 1);
        Require(once.state == 1);
    }
}

void TestInvalidControlStates() {
    const int invalidStates[] = {-1000, -50, -1, 3, 4, 10, 42, 100, 9999};
    for (const int state : invalidStates) {
        GuestOnce badOnce = {state, nullptr};
        const int beforeCount = gCountCalls;
        Require(pthread_once_nid_postfix(&badOnce, CountRoutine) == PosixEinval);
        Require(gCountCalls == beforeCount);
        Require(badOnce.state == state);
    }
}

void TestExceptionRollbackAndRetry() {
    GuestOnce failOnce = {0, nullptr};
    gThrowAttempts = 0;
    gSuccessCalls = 0;

    Require(Throws([&] { pthread_once_nid_postfix(&failOnce, ThrowRoutine); }));
    Require(gThrowAttempts == 1);
    Require(failOnce.state == 0);

    Require(pthread_once_nid_postfix(&failOnce, SuccessRoutine) == 0);
    Require(gSuccessCalls == 1);
    Require(failOnce.state == 1);

    Require(pthread_once_nid_postfix(&failOnce, SuccessRoutine) == 0);
    Require(gSuccessCalls == 1);
    Require(failOnce.state == 1);
}

void TestNestedOnceControls() {
    GuestOnce outerOnce = {0, nullptr};
    gInnerOnce = {0, nullptr};
    gOuterCalls = 0;
    gInnerCalls = 0;

    Require(pthread_once_nid_postfix(&outerOnce, OuterRoutine) == 0);
    Require(gOuterCalls == 1);
    Require(gInnerCalls == 1);
    Require(outerOnce.state == 1);
    Require(gInnerOnce.state == 1);

    Require(pthread_once_nid_postfix(&outerOnce, OuterRoutine) == 0);
    Require(pthread_once_nid_postfix(&gInnerOnce, InnerRoutine) == 0);
    Require(gOuterCalls == 1);
    Require(gInnerCalls == 1);
}

void TestMultiThreadConcurrencyGuarantees() {
    gConcurrentOnce = {0, nullptr};
    gConcurrentCalls.store(0, std::memory_order_relaxed);

    Pthread threads[ConcurrentThreadCount]{};
    WorkerResult results[ConcurrentThreadCount]{};

    for (int i = 0; i < ConcurrentThreadCount; ++i) {
        Require(scePthreadCreate(&threads[i], nullptr, ConcurrencyWorkerThread, &results[i], nullptr) == 0);
    }

    for (int i = 0; i < ConcurrentThreadCount; ++i) {
        Require(scePthreadJoin(threads[i], nullptr) == 0);
        Require(results[i].returnCode.load(std::memory_order_acquire) == 0);
    }

    Require(gConcurrentCalls.load(std::memory_order_relaxed) == 1);
    Require(gConcurrentOnce.state == 1);

    Require(pthread_once_nid_postfix(&gConcurrentOnce, ConcurrentRoutine) == 0);
    Require(gConcurrentCalls.load(std::memory_order_relaxed) == 1);
}

} // namespace

int main() {
    TestNullParameterValidation();
    TestStateTransitionsAndInitLifecycle();
    TestInvalidControlStates();
    TestExceptionRollbackAndRetry();
    TestNestedOnceControls();
    TestMultiThreadConcurrencyGuarantees();
    return 0;
}
