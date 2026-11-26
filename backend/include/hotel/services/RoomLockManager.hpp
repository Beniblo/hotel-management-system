#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace hotel::services {

class RoomLockManager {
public:
    class ScopedLock {
    public:
        ScopedLock() = default;
        ScopedLock(ScopedLock&&) noexcept = default;
        ScopedLock& operator=(ScopedLock&&) noexcept = default;
        ScopedLock(const ScopedLock&) = delete;
        ScopedLock& operator=(const ScopedLock&) = delete;
        ~ScopedLock() = default;

    private:
        friend class RoomLockManager;

        explicit ScopedLock(std::vector<std::shared_ptr<std::mutex>> mutexes);

        // Orden critico: mutexes_ debe vivir mas que locks_ para desbloquear antes de liberar.
        std::vector<std::shared_ptr<std::mutex>> mutexes_;
        std::vector<std::unique_lock<std::mutex>> locks_;
    };

    [[nodiscard]] ScopedLock lock(const std::string& habitacionId);
    [[nodiscard]] ScopedLock lockTwo(const std::string& firstHabitacionId,
                                     const std::string& secondHabitacionId);

private:
    std::shared_ptr<std::mutex> getOrCreateMutex(const std::string& habitacionId);

    std::mutex tableMutex_;
    std::unordered_map<std::string, std::shared_ptr<std::mutex>> roomMutexes_;
};

}  // namespace hotel::services