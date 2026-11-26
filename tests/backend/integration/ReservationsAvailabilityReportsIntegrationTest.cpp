#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "hotel/config/AppConfig.hpp"
#include "hotel/core/AppContext.hpp"
#include "hotel/core/Exceptions.hpp"
#include "hotel/models/Enums.hpp"

namespace {

using hotel::config::AppConfig;
using hotel::core::AppContext;
using hotel::core::ConflictException;
using hotel::models::EstadoHabitacion;
using hotel::models::EstadoReservacion;
using hotel::services::CreateHabitacionInput;
using hotel::services::CreateHuespedInput;
using hotel::services::CreateReservacionInput;
using hotel::services::DisponibilidadQuery;
using hotel::services::UpdateReservacionInput;

std::string uniqueSuffix() {
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
    return std::to_string(ms);
}

// Guard RAII para garantizar limpieza de registros temporales en Atlas aun si falla una asercion.
struct ScopedTestFixture {
    AppContext& ctx;
    std::string huespedId;
    std::string habitacionId;
    std::vector<std::string> reservacionIds;

    explicit ScopedTestFixture(AppContext& context) : ctx(context) {
        const std::string tag = uniqueSuffix();

        CreateHuespedInput hIn;
        hIn.nombre = "Huesped Fixture " + tag;
        hIn.correo = "fixture." + tag + "@ejemplo.com";
        hIn.telefono = "4778880001";
        huespedId = ctx.huespedService().crear(hIn).id;

        CreateHabitacionInput habIn;
        habIn.numero = "FX-" + tag.substr(tag.size() > 6 ? tag.size() - 6 : 0);
        habIn.tipo = "suite";
        habIn.capacidad = 4;
        habIn.tarifa = 1000.0;
        habIn.servicios = {"wifi", "balcon"};
        habitacionId = ctx.habitacionService().crear(habIn).id;
    }

    ~ScopedTestFixture() {
        for (const auto& resId : reservacionIds) {
            try {
                const auto r = ctx.reservacionService().obtenerPorId(resId);
                if (r.estado != EstadoReservacion::Cancelada &&
                    r.estado != EstadoReservacion::Completada) {
                    static_cast<void>(ctx.reservacionService().cancelar(resId));
                }
            } catch (...) {
            }
            try {
                ctx.reservacionService().eliminar(resId);
            } catch (...) {
            }
        }
        if (!habitacionId.empty()) {
            try {
                ctx.habitacionService().eliminar(habitacionId);
            } catch (...) {
            }
        }
        if (!huespedId.empty()) {
            try {
                static_cast<void>(ctx.huespedService().eliminar(huespedId, true));
            } catch (...) {
            }
        }
    }
};

TEST(ReservationsIntegrationTest, CrudOverlapsBoundaryAndAvailabilityFiltering) {
    const AppConfig config = AppConfig::load();
    AppContext ctx(config);
    ScopedTestFixture fx(ctx);

    // 1. Crear reservacion principal: 2029-05-10 a 2029-05-13 (3 noches * 1000 = 3000)
    CreateReservacionInput req1;
    req1.huespedId = fx.huespedId;
    req1.habitacionId = fx.habitacionId;
    req1.fechaEntrada = "2029-05-10";
    req1.fechaSalida = "2029-05-13";
    req1.personas = 2;
    req1.estado = EstadoReservacion::Confirmada;

    const auto res1 = ctx.reservacionService().crear(req1);
    fx.reservacionIds.push_back(res1.id);
    EXPECT_EQ(res1.noches, 3);
    EXPECT_DOUBLE_EQ(res1.tarifaAplicada, 1000.0);
    EXPECT_DOUBLE_EQ(res1.total, 3000.0);

    // 2. Disponibilidad debe excluir la habitacion en fechas traslapadas
    DisponibilidadQuery dispOverlap;
    dispOverlap.fechaEntrada = "2029-05-11";
    dispOverlap.fechaSalida = "2029-05-12";
    const auto dispResOverlap = ctx.disponibilidadService().consultar(dispOverlap);
    const bool foundWhileBooked = std::any_of(
        dispResOverlap.habitaciones.begin(),
        dispResOverlap.habitaciones.end(),
        [&](const auto& item) { return item.habitacion.id == fx.habitacionId; });
    EXPECT_FALSE(foundWhileBooked);

    // 3. Intento de reservacion traslapada -> HTTP 409 RESERVATION_OVERLAP
    CreateReservacionInput reqOverlap = req1;
    reqOverlap.fechaEntrada = "2029-05-12";
    reqOverlap.fechaSalida = "2029-05-14";
    EXPECT_THROW(static_cast<void>(ctx.reservacionService().crear(reqOverlap)), ConflictException);

    // 4. Borde exacto check-out == check-in (2029-05-13 a 2029-05-15) -> PERMITIDO
    CreateReservacionInput reqBoundary = req1;
    reqBoundary.fechaEntrada = "2029-05-13";
    reqBoundary.fechaSalida = "2029-05-15";
    const auto res2 = ctx.reservacionService().crear(reqBoundary);
    fx.reservacionIds.push_back(res2.id);
    EXPECT_EQ(res2.noches, 2);
    EXPECT_DOUBLE_EQ(res2.total, 2000.0);

    // 5. Actualizacion de fechas recalcula el total (extender res2 a 3 noches)
    UpdateReservacionInput patch2;
    patch2.fechaSalida = "2029-05-16";
    const auto updated2 = ctx.reservacionService().actualizar(res2.id, patch2);
    EXPECT_EQ(updated2.noches, 3);
    EXPECT_DOUBLE_EQ(updated2.total, 3000.0);

    // 6. No se permite borrar fisicamente una reservacion confirmada sin cancelar primero
    EXPECT_THROW(ctx.reservacionService().eliminar(res1.id), ConflictException);

    // 7. Cancelar libera la habitacion en disponibilidad
    const auto cancelled1 = ctx.reservacionService().cancelar(res1.id);
    EXPECT_EQ(cancelled1.estado, EstadoReservacion::Cancelada);

    const auto dispAfterCancel = ctx.disponibilidadService().consultar(dispOverlap);
    const bool foundAfterCancel = std::any_of(
        dispAfterCancel.habitaciones.begin(),
        dispAfterCancel.habitaciones.end(),
        [&](const auto& item) { return item.habitacion.id == fx.habitacionId; });
    EXPECT_TRUE(foundAfterCancel);
}

TEST(ReservationsIntegrationTest, ConcurrentRequestsAllowExactlyOneWinner) {
    const AppConfig config = AppConfig::load();
    AppContext ctx(config);
    ScopedTestFixture fx(ctx);

    std::atomic<int> successes{0};
    std::atomic<int> overlapConflicts{0};
    std::mutex winnerMutex;

    auto worker = [&]() {
        CreateReservacionInput req;
        req.huespedId = fx.huespedId;
        req.habitacionId = fx.habitacionId;
        req.fechaEntrada = "2029-05-20";
        req.fechaSalida = "2029-05-22";
        req.personas = 2;
        req.estado = EstadoReservacion::Confirmada;

        try {
            const auto created = ctx.reservacionService().crear(req);
            ++successes;
            std::lock_guard<std::mutex> guard(winnerMutex);
            fx.reservacionIds.push_back(created.id);
        } catch (const ConflictException& ex) {
            if (ex.errorCode() == "RESERVATION_OVERLAP") {
                ++overlapConflicts;
            }
        }
    };

    std::vector<std::thread> threads;
    for (int i = 0; i < 4; ++i) {
        threads.emplace_back(worker);
    }
    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(successes.load(), 1);
    EXPECT_EQ(overlapConflicts.load(), 3);
}

TEST(ReservationsIntegrationTest, ReportCalculatesOccupancyAndExcludesCancelledIncome) {
    const AppConfig config = AppConfig::load();
    AppContext ctx(config);
    ScopedTestFixture fx(ctx);

    const auto baseline = ctx.reporteService().generarResumen("2029-06-01", "2029-06-30");

    // 1. Reservacion confirmada: 2 noches * 1000 = 2000 MXN (suma a ingresos y ocupacion)
    CreateReservacionInput activeReq;
    activeReq.huespedId = fx.huespedId;
    activeReq.habitacionId = fx.habitacionId;
    activeReq.fechaEntrada = "2029-06-05";
    activeReq.fechaSalida = "2029-06-07";
    activeReq.personas = 2;
    activeReq.estado = EstadoReservacion::Confirmada;
    const auto activeRes = ctx.reservacionService().crear(activeReq);
    fx.reservacionIds.push_back(activeRes.id);

    // 2. Reservacion posteriormente cancelada: 3 noches * 1000 = 3000 MXN (NO debe sumar a ingresos)
    CreateReservacionInput cancelReq = activeReq;
    cancelReq.fechaEntrada = "2029-06-10";
    cancelReq.fechaSalida = "2029-06-13";
    const auto cancelledRes = ctx.reservacionService().crear(cancelReq);
    fx.reservacionIds.push_back(cancelledRes.id);
    static_cast<void>(ctx.reservacionService().cancelar(cancelledRes.id));

    const auto report = ctx.reporteService().generarResumen("2029-06-01", "2029-06-30");

    EXPECT_EQ(report.totalReservaciones, baseline.totalReservaciones + 2);
    EXPECT_EQ(report.reservacionesPorEstado.at("confirmada"),
              baseline.reservacionesPorEstado.at("confirmada") + 1);
    EXPECT_EQ(report.reservacionesPorEstado.at("cancelada"),
              baseline.reservacionesPorEstado.at("cancelada") + 1);
    EXPECT_EQ(report.habitacionesOcupadas, baseline.habitacionesOcupadas + 1);
    EXPECT_DOUBLE_EQ(report.ingresosEstimados, baseline.ingresosEstimados + 2000.0);
}

}  // namespace