#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

#include "hotel/core/Exceptions.hpp"
#include "hotel/services/RoomLockManager.hpp"
#include "hotel/validators/InputValidators.hpp"

namespace {

using hotel::core::ValidationException;
using hotel::services::RoomLockManager;
using namespace hotel::validators;

TEST(OverlapsTest, DetectsAllOverlapCases) {
    // Base: [2026-12-10, 2026-12-15)

    // 1. Traslape exacto
    EXPECT_TRUE(datesOverlap("2026-12-10", "2026-12-15", "2026-12-10", "2026-12-15"));

    // 2. Traslape parcial izquierdo
    EXPECT_TRUE(datesOverlap("2026-12-10", "2026-12-15", "2026-12-08", "2026-12-11"));

    // 3. Traslape parcial derecho
    EXPECT_TRUE(datesOverlap("2026-12-10", "2026-12-15", "2026-12-14", "2026-12-18"));

    // 4. Intervalo contenido internamente
    EXPECT_TRUE(datesOverlap("2026-12-10", "2026-12-15", "2026-12-11", "2026-12-13"));

    // 5. Intervalo envolvente externo
    EXPECT_TRUE(datesOverlap("2026-12-10", "2026-12-15", "2026-12-09", "2026-12-20"));
}

TEST(OverlapsTest, AllowsBoundaryCheckOutEqualsCheckInAndDisjointRanges) {
    // Borde exacto derecho: S1 == E2 (check-out el 15, nuevo check-in el 15) -> PERMITIDO
    EXPECT_FALSE(datesOverlap("2026-12-10", "2026-12-15", "2026-12-15", "2026-12-18"));

    // Borde exacto izquierdo: S2 == E1 (check-out el 10, nuevo check-in el 10) -> PERMITIDO
    EXPECT_FALSE(datesOverlap("2026-12-10", "2026-12-15", "2026-12-07", "2026-12-10"));

    // Completamente disjuntos
    EXPECT_FALSE(datesOverlap("2026-12-10", "2026-12-15", "2026-12-20", "2026-12-25"));
    EXPECT_FALSE(datesOverlap("2026-12-10", "2026-12-15", "2026-12-01", "2026-12-05"));
}

TEST(TotalsTest, CalculatesNightsAndTotalsAccurately) {
    EXPECT_EQ(calculateNights("2026-12-05", "2026-12-08"), 3);
    EXPECT_EQ(calculateNights("2024-02-28", "2024-03-01"), 2);  // cruza 29 feb bisiesto
    EXPECT_EQ(calculateNights("2026-12-30", "2027-01-02"), 3);  // cruza fin de ano

    EXPECT_THROW(static_cast<void>(calculateNights("2026-12-05", "2026-12-05")),
                 ValidationException);
    EXPECT_THROW(static_cast<void>(calculateNights("2026-12-08", "2026-12-05")),
                 ValidationException);

    EXPECT_DOUBLE_EQ(roundCurrency(1450.556), 1450.56);
    EXPECT_DOUBLE_EQ(roundCurrency(-0.0), 0.0);

    EXPECT_DOUBLE_EQ(calculateReservationTotal(3, 1450.0), 4350.0);
    EXPECT_DOUBLE_EQ(calculateReservationTotal(2, 999.995), 1999.99);

    EXPECT_THROW(static_cast<void>(calculateReservationTotal(0, 1450.0)), ValidationException);
    EXPECT_THROW(static_cast<void>(calculateReservationTotal(2, 0.0)), ValidationException);
    EXPECT_THROW(static_cast<void>(calculateReservationTotal(2, -500.0)), ValidationException);
}

TEST(RoomLockManagerTest, SerializesSameRoomAndPreventsDeadlockOnLockTwo) {
    RoomLockManager manager;
    int sharedCounter = 0;

    // 1. Exclusion mutua sobre la misma habitacion
    std::vector<std::thread> workers;
    for (int t = 0; t < 4; ++t) {
        workers.emplace_back([&]() {
            for (int i = 0; i < 200; ++i) {
                auto guard = manager.lock("room-101");
                ++sharedCounter;
            }
        });
    }
    for (auto& w : workers) {
        w.join();
    }
    EXPECT_EQ(sharedCounter, 800);

    // 2. Bloqueos opuestos simultaneos (A, B) y (B, A) sin deadlock
    std::atomic<int> completedSwaps{0};
    std::thread t1([&]() {
        for (int i = 0; i < 100; ++i) {
            auto guard = manager.lockTwo("room-A", "room-B");
            ++completedSwaps;
        }
    });
    std::thread t2([&]() {
        for (int i = 0; i < 100; ++i) {
            auto guard = manager.lockTwo("room-B", "room-A");
            ++completedSwaps;
        }
    });
    t1.join();
    t2.join();
    EXPECT_EQ(completedSwaps.load(), 200);
}

}  // namespace