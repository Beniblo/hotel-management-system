#include "hotel/services/RoomLockManager.hpp"

#include <utility>

namespace hotel::services {

RoomLockManager::ScopedLock::ScopedLock(std::vector<std::shared_ptr<std::mutex>> mutexes)
    : mutexes_(std::move(mutexes)) {
    locks_.reserve(mutexes_.size());
    for (const auto& mtx : mutexes_) {
        locks_.emplace_back(*mtx);
    }
}

std::shared_ptr<std::mutex> RoomLockManager::getOrCreateMutex(const std::string& habitacionId) {
    std::lock_guard<std::mutex> guard(tableMutex_);
    auto& mtx = roomMutexes_[habitacionId];
    if (!mtx) {
        mtx = std::make_shared<std::mutex>();
    }
    return mtx;
}

RoomLockManager::ScopedLock RoomLockManager::lock(const std::string& habitacionId) {
    return ScopedLock({getOrCreateMutex(habitacionId)});
}

RoomLockManager::ScopedLock RoomLockManager::lockTwo(const std::string& firstHabitacionId,
                                                     const std::string& secondHabitacionId) {
    if (firstHabitacionId == secondHabitacionId) {
        return lock(firstHabitacionId);
    }

    // Orden lexicografico consistente para prevenir deadlocks ante intercambios simultaneos.
    const std::string& lowerId =
        (firstHabitacionId < secondHabitacionId) ? firstHabitacionId : secondHabitacionId;
    const std::string& higherId =
        (firstHabitacionId < secondHabitacionId) ? secondHabitacionId : firstHabitacionId;

    return ScopedLock({getOrCreateMutex(lowerId), getOrCreateMutex(higherId)});
}

}  // namespace hotel::services