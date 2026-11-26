#include "hotel/repositories/ReservacionRepository.hpp"

#include <chrono>

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/options/find.hpp>

#include "hotel/core/Exceptions.hpp"
#include "hotel/validators/InputValidators.hpp"

namespace hotel::repositories {

using bsoncxx::builder::basic::array;
using bsoncxx::builder::basic::document;
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;

namespace {

bsoncxx::types::b_date isoDateToBsonMidnight(std::string_view isoDate) {
    const auto ms = hotel::validators::isoDateToUtcMidnightMs(isoDate);
    return bsoncxx::types::b_date{std::chrono::milliseconds{ms}};
}

bsoncxx::array::value makeBlockingStatesArray() {
    array states;
    states.append(hotel::models::toString(hotel::models::EstadoReservacion::Pendiente));
    states.append(hotel::models::toString(hotel::models::EstadoReservacion::Confirmada));
    states.append(hotel::models::toString(hotel::models::EstadoReservacion::EnCurso));
    return states.extract();
}

}  // namespace

ReservacionRepository::ReservacionRepository(
    hotel::database::MongoConnectionManager& connectionManager)
    : connectionManager_(connectionManager) {}

hotel::models::Reservacion ReservacionRepository::fromDocument(
    const bsoncxx::document::view& doc) {
    hotel::models::Reservacion r;

    if (const auto idElem = doc["_id"]; idElem && idElem.type() == bsoncxx::type::k_oid) {
        r.id = idElem.get_oid().value.to_string();
    }
    if (const auto huespedElem = doc["huespedId"];
        huespedElem && huespedElem.type() == bsoncxx::type::k_oid) {
        r.huespedId = huespedElem.get_oid().value.to_string();
    }
    if (const auto habElem = doc["habitacionId"];
        habElem && habElem.type() == bsoncxx::type::k_oid) {
        r.habitacionId = habElem.get_oid().value.to_string();
    }

    if (const auto entradaElem = doc["fechaEntrada"];
        entradaElem && entradaElem.type() == bsoncxx::type::k_date) {
        r.fechaEntrada = hotel::validators::utcMsToIsoDate(entradaElem.get_date().value.count());
    }
    if (const auto salidaElem = doc["fechaSalida"];
        salidaElem && salidaElem.type() == bsoncxx::type::k_date) {
        r.fechaSalida = hotel::validators::utcMsToIsoDate(salidaElem.get_date().value.count());
    }

    if (hotel::validators::isValidIsoDate(r.fechaEntrada) &&
        hotel::validators::isValidIsoDate(r.fechaSalida)) {
        const auto inMs = hotel::validators::isoDateToUtcMidnightMs(r.fechaEntrada);
        const auto outMs = hotel::validators::isoDateToUtcMidnightMs(r.fechaSalida);
        if (outMs > inMs) {
            r.noches = static_cast<std::int32_t>((outMs - inMs) / (24LL * 60LL * 60LL * 1000LL));
        }
    }

    r.personas = readInt32OrDefault(doc, "personas", 1);

    const auto parsedEstado =
        hotel::models::parseEstadoReservacion(readStringOrEmpty(doc, "estado"));
    r.estado = parsedEstado.value_or(hotel::models::EstadoReservacion::Confirmada);

    r.tarifaAplicada =
        hotel::validators::roundCurrency(readDoubleOrDefault(doc, "tarifaAplicada", 0.0));
    r.total = hotel::validators::roundCurrency(readDoubleOrDefault(doc, "total", 0.0));
    r.creadoEn = readDateOrNow(doc, "creadoEn");
    r.actualizadoEn = readDateOrNow(doc, "actualizadoEn");

    return r;
}

hotel::models::Reservacion ReservacionRepository::create(
    const hotel::models::Reservacion& reservacion) {
    const auto huespedOid = parseOid(reservacion.huespedId, "huespedId");
    const auto habitacionOid = parseOid(reservacion.habitacionId, "habitacionId");
    const auto fechaEntradaBson = isoDateToBsonMidnight(reservacion.fechaEntrada);
    const auto fechaSalidaBson = isoDateToBsonMidnight(reservacion.fechaSalida);

    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        const auto doc = make_document(
            kvp("huespedId", huespedOid),
            kvp("habitacionId", habitacionOid),
            kvp("fechaEntrada", fechaEntradaBson),
            kvp("fechaSalida", fechaSalidaBson),
            kvp("personas", reservacion.personas),
            kvp("estado", hotel::models::toString(reservacion.estado)),
            kvp("tarifaAplicada", hotel::validators::roundCurrency(reservacion.tarifaAplicada)),
            kvp("total", hotel::validators::roundCurrency(reservacion.total)),
            kvp("creadoEn", toBsonDate(reservacion.creadoEn)),
            kvp("actualizadoEn", toBsonDate(reservacion.actualizadoEn))
        );

        const auto result = collection.insert_one(doc.view());
        if (!result || result->inserted_id().type() != bsoncxx::type::k_oid) {
            throw hotel::core::InternalServerException(
                "No se recibio identificador tras insertar la reservacion.");
        }

        hotel::models::Reservacion created = reservacion;
        created.id = result->inserted_id().get_oid().value.to_string();
        return created;
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        handleMongoException(ex, "ReservacionRepository::create");
    }
}

PagedResult<hotel::models::Reservacion> ReservacionRepository::findAll(
    const ReservacionFilter& filter) {
    hotel::validators::validatePagination(filter.page, filter.limit);

    if (filter.fechaDesde.has_value() && filter.fechaHasta.has_value()) {
        const auto desdeMs = hotel::validators::isoDateToUtcMidnightMs(*filter.fechaDesde);
        const auto hastaMs = hotel::validators::isoDateToUtcMidnightMs(*filter.fechaHasta);
        if (desdeMs > hastaMs) {
            throw hotel::core::ValidationException(
                "Rango de fechas de filtro invalido.",
                {{"fechaHasta", "fechaHasta debe ser mayor o igual a fechaDesde."}});
        }
    }

    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        document queryBuilder;
        if (filter.huespedId.has_value() && !filter.huespedId->empty()) {
            queryBuilder.append(kvp("huespedId", parseOid(*filter.huespedId, "huespedId")));
        }
        if (filter.habitacionId.has_value() && !filter.habitacionId->empty()) {
            queryBuilder.append(
                kvp("habitacionId", parseOid(*filter.habitacionId, "habitacionId")));
        }
        if (filter.estado.has_value()) {
            queryBuilder.append(kvp("estado", hotel::models::toString(*filter.estado)));
        }
        if (filter.fechaDesde.has_value() && !filter.fechaDesde->empty()) {
            queryBuilder.append(kvp(
                "fechaSalida",
                make_document(kvp("$gt", isoDateToBsonMidnight(*filter.fechaDesde)))));
        }
        if (filter.fechaHasta.has_value() && !filter.fechaHasta->empty()) {
            queryBuilder.append(kvp(
                "fechaEntrada",
                make_document(kvp("$lte", isoDateToBsonMidnight(*filter.fechaHasta)))));
        }

        const auto queryDoc = queryBuilder.extract();
        PagedResult<hotel::models::Reservacion> pageResult;
        pageResult.page = filter.page;
        pageResult.limit = filter.limit;
        pageResult.total = collection.count_documents(queryDoc.view());

        mongocxx::options::find findOpts;
        findOpts.sort(make_document(kvp("fechaEntrada", -1), kvp("creadoEn", -1)));
        findOpts.skip(static_cast<std::int64_t>(filter.page - 1) * filter.limit);
        findOpts.limit(static_cast<std::int64_t>(filter.limit));

        auto cursor = collection.find(queryDoc.view(), findOpts);
        for (const auto& doc : cursor) {
            pageResult.items.push_back(fromDocument(doc));
        }

        return pageResult;
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        handleMongoException(ex, "ReservacionRepository::findAll");
    }
}

std::optional<hotel::models::Reservacion> ReservacionRepository::findById(const std::string& id) {
    const auto oid = parseOid(id, "reservacionId");

    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        const auto filter = make_document(kvp("_id", oid));
        const auto found = collection.find_one(filter.view());
        if (!found.has_value()) {
            return std::nullopt;
        }
        return fromDocument(found->view());
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        handleMongoException(ex, "ReservacionRepository::findById");
    }
}

std::optional<hotel::models::Reservacion> ReservacionRepository::update(
    const hotel::models::Reservacion& reservacion) {
    const auto oid = parseOid(reservacion.id, "reservacionId");
    const auto huespedOid = parseOid(reservacion.huespedId, "huespedId");
    const auto habitacionOid = parseOid(reservacion.habitacionId, "habitacionId");
    const auto fechaEntradaBson = isoDateToBsonMidnight(reservacion.fechaEntrada);
    const auto fechaSalidaBson = isoDateToBsonMidnight(reservacion.fechaSalida);

    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        const auto filter = make_document(kvp("_id", oid));
        const auto updateDoc = make_document(kvp(
            "$set",
            make_document(
                kvp("huespedId", huespedOid),
                kvp("habitacionId", habitacionOid),
                kvp("fechaEntrada", fechaEntradaBson),
                kvp("fechaSalida", fechaSalidaBson),
                kvp("personas", reservacion.personas),
                kvp("estado", hotel::models::toString(reservacion.estado)),
                kvp("tarifaAplicada", hotel::validators::roundCurrency(reservacion.tarifaAplicada)),
                kvp("total", hotel::validators::roundCurrency(reservacion.total)),
                kvp("actualizadoEn", toBsonDate(reservacion.actualizadoEn))
            )));

        const auto result = collection.update_one(filter.view(), updateDoc.view());
        if (!result || result->matched_count() == 0) {
            return std::nullopt;
        }

        return reservacion;
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        handleMongoException(ex, "ReservacionRepository::update");
    }
}

bool ReservacionRepository::updateEstado(const std::string& id,
                                         hotel::models::EstadoReservacion estado,
                                         std::chrono::system_clock::time_point actualizadoEn) {
    const auto oid = parseOid(id, "reservacionId");

    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        const auto filter = make_document(kvp("_id", oid));
        const auto updateDoc = make_document(kvp(
            "$set",
            make_document(
                kvp("estado", hotel::models::toString(estado)),
                kvp("actualizadoEn", toBsonDate(actualizadoEn))
            )));

        const auto result = collection.update_one(filter.view(), updateDoc.view());
        return result.has_value() && result->matched_count() > 0;
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        handleMongoException(ex, "ReservacionRepository::updateEstado");
    }
}

bool ReservacionRepository::deleteById(const std::string& id) {
    const auto oid = parseOid(id, "reservacionId");

    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        const auto filter = make_document(kvp("_id", oid));
        const auto result = collection.delete_one(filter.view());
        return result.has_value() && result->deleted_count() > 0;
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        handleMongoException(ex, "ReservacionRepository::deleteById");
    }
}

bool ReservacionRepository::hasOverlappingBlockingReservation(
    const std::string& habitacionId,
    const std::string& fechaEntrada,
    const std::string& fechaSalida,
    const std::optional<std::string>& excludeReservacionId) {
    const auto habitacionOid = parseOid(habitacionId, "habitacionId");
    static_cast<void>(hotel::validators::calculateNights(fechaEntrada, fechaSalida));

    const auto entradaBson = isoDateToBsonMidnight(fechaEntrada);
    const auto salidaBson = isoDateToBsonMidnight(fechaSalida);

    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        // Mantener vivo blockingStates hasta finalizar la consulta MongoDB
        const auto blockingStates = makeBlockingStatesArray();

        // Alineado a ix_reservaciones_traslape: { habitacionId: 1, estado: 1, fechaEntrada: 1, fechaSalida: 1 }
        // Traslape semiabierto [E1, S1) con [E2, S2) <=> E1 < S2 && S1 > E2
        document queryBuilder;
        queryBuilder.append(kvp("habitacionId", habitacionOid));
        queryBuilder.append(kvp("estado", make_document(kvp("$in", blockingStates.view()))));
        queryBuilder.append(kvp("fechaEntrada", make_document(kvp("$lt", salidaBson))));
        queryBuilder.append(kvp("fechaSalida", make_document(kvp("$gt", entradaBson))));

        if (excludeReservacionId.has_value() && !excludeReservacionId->empty()) {
            const auto excludeOid = parseOid(*excludeReservacionId, "reservacionId");
            queryBuilder.append(kvp("_id", make_document(kvp("$ne", excludeOid))));
        }

        const auto queryDoc = queryBuilder.extract();
        mongocxx::options::find findOpts;
        findOpts.limit(1);
        findOpts.projection(make_document(kvp("_id", 1)));

        const auto found = collection.find_one(queryDoc.view(), findOpts);
        return found.has_value();
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        handleMongoException(ex, "ReservacionRepository::hasOverlappingBlockingReservation");
    }
}

std::unordered_set<std::string> ReservacionRepository::findBlockedHabitacionIds(
    const std::string& fechaEntrada,
    const std::string& fechaSalida) {
    static_cast<void>(hotel::validators::calculateNights(fechaEntrada, fechaSalida));

    const auto entradaBson = isoDateToBsonMidnight(fechaEntrada);
    const auto salidaBson = isoDateToBsonMidnight(fechaSalida);

    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        const auto blockingStates = makeBlockingStatesArray();
        const auto queryDoc = make_document(
            kvp("estado", make_document(kvp("$in", blockingStates.view()))),
            kvp("fechaEntrada", make_document(kvp("$lt", salidaBson))),
            kvp("fechaSalida", make_document(kvp("$gt", entradaBson)))
        );

        mongocxx::options::find findOpts;
        findOpts.projection(make_document(kvp("habitacionId", 1)));

        std::unordered_set<std::string> blockedIds;
        auto cursor = collection.find(queryDoc.view(), findOpts);
        for (const auto& doc : cursor) {
            if (const auto habElem = doc["habitacionId"];
                habElem && habElem.type() == bsoncxx::type::k_oid) {
                blockedIds.insert(habElem.get_oid().value.to_string());
            }
        }
        return blockedIds;
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        handleMongoException(ex, "ReservacionRepository::findBlockedHabitacionIds");
    }
}

std::vector<hotel::models::Reservacion> ReservacionRepository::findIntersectingPeriod(
    const std::string& fechaInicio,
    const std::string& fechaFin) {
    hotel::validators::validateReportDateRange(fechaInicio, fechaFin);

    const auto inicioBson = isoDateToBsonMidnight(fechaInicio);
    const auto finBson = isoDateToBsonMidnight(fechaFin);

    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        // Una estancia [E, S) intersecta el periodo inclusivo [fechaInicio, fechaFin]
        // (noches desde fechaInicio hasta fechaFin) si y solo si E <= fechaFin && S > fechaInicio.
        const auto queryDoc = make_document(
            kvp("fechaEntrada", make_document(kvp("$lte", finBson))),
            kvp("fechaSalida", make_document(kvp("$gt", inicioBson)))
        );

        mongocxx::options::find findOpts;
        findOpts.sort(make_document(kvp("fechaEntrada", 1)));

        std::vector<hotel::models::Reservacion> result;
        auto cursor = collection.find(queryDoc.view(), findOpts);
        for (const auto& doc : cursor) {
            result.push_back(fromDocument(doc));
        }
        return result;
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        handleMongoException(ex, "ReservacionRepository::findIntersectingPeriod");
    }
}

std::int64_t ReservacionRepository::countByHuespedId(const std::string& huespedId,
                                                     bool onlyBlocking) {
    const auto oid = parseOid(huespedId, "huespedId");

    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        const auto blockingStates = makeBlockingStatesArray();
        document queryBuilder;
        queryBuilder.append(kvp("huespedId", oid));
        if (onlyBlocking) {
            queryBuilder.append(kvp("estado", make_document(kvp("$in", blockingStates.view()))));
        }

        const auto queryDoc = queryBuilder.extract();
        return collection.count_documents(queryDoc.view());
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        handleMongoException(ex, "ReservacionRepository::countByHuespedId");
    }
}

std::int64_t ReservacionRepository::countByHabitacionId(const std::string& habitacionId,
                                                        bool onlyBlocking) {
    const auto oid = parseOid(habitacionId, "habitacionId");

    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        const auto blockingStates = makeBlockingStatesArray();
        document queryBuilder;
        queryBuilder.append(kvp("habitacionId", oid));
        if (onlyBlocking) {
            queryBuilder.append(kvp("estado", make_document(kvp("$in", blockingStates.view()))));
        }

        const auto queryDoc = queryBuilder.extract();
        return collection.count_documents(queryDoc.view());
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        handleMongoException(ex, "ReservacionRepository::countByHabitacionId");
    }
}

}  // namespace hotel::repositories