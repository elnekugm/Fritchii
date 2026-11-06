// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <atomic>
#include <cstddef>
#include <memory>

#include "common/logging.h"
#include "common/thread_worker.h"

namespace Vulkan {

constexpr bool DefersShaderEmission(bool native, size_t pending, size_t capacity) {
    return native && pending < capacity;
}

// Caps how many cached pipelines are built in the background at once (from Lichii build-88).
class EmissionBudget {
public:
    class Reservation {
    public:
        Reservation(const Reservation&) = delete;
        Reservation& operator=(const Reservation&) = delete;

        ~Reservation() {
            if (pending) {
                pending->fetch_sub(1, std::memory_order_relaxed);
            }
        }

    private:
        friend class EmissionBudget;
        Reservation() = default;
        std::shared_ptr<std::atomic<size_t>> pending;
    };

    explicit EmissionBudget(size_t capacity_) : capacity{capacity_} {}

    std::unique_ptr<Reservation> TryAcquire(bool native) {
        size_t count = pending->load(std::memory_order_relaxed);
        if (!DefersShaderEmission(native, count, capacity)) {
            return nullptr;
        }
        auto reservation = std::unique_ptr<Reservation>{new Reservation};
        while (DefersShaderEmission(native, count, capacity)) {
            if (pending->compare_exchange_weak(count, count + 1, std::memory_order_relaxed)) {
                reservation->pending = pending;
                return reservation;
            }
        }
        return nullptr;
    }

    size_t Pending() const {
        return pending->load(std::memory_order_relaxed);
    }

private:
    const size_t capacity;
    std::shared_ptr<std::atomic<size_t>> pending = std::make_shared<std::atomic<size_t>>(0);
};

} // namespace Vulkan
