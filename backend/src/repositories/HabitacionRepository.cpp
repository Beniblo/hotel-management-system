#include "hotel/repositories/HabitacionRepository.hpp"

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
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

bsoncxx::array::value toBsonArray(const std::vector<std::string>& items) {
    array arr;
    for (const auto& item : items) {
        arr.append(item);
    }
    return arr.extract();
}

}  // namespace

HabitacionRepository::HabitacionRepository(
    hotel::database::MongoConnectionManager& connectionManager)
    : connectionManager_(connectionManager) {}

hotel::models::Habitacion HabitacionRepository::fromDocument(const bsoncxx::document::view& doc) {
    hotel::models::Habitacion hab;
    if (const auto idElem = doc["_id"]; idElem && idElem.type() == bsoncxx::type::k_oid) {
        hab.id = idElem.get_oid().value.to_string();
    }
    hab.numero = readStringOrEmpty(doc, "numero");
    hab.tipo = readStringOrEmpty(doc, "tipo");
    hab.capacidad = readInt32OrDefault(doc, "capacidad", 1);
    hab.tarifa = hotel::validators::roundCurrency(readDoubleOrDefault(doc, "tarifa", 0.0));

    const auto parsedEstado =
        hotel::models::parseEstadoHabitacion(readStringOrEmpty(doc, "estado"));
    hab.estado = parsedEstado.value_or(hotel::models::EstadoHabitacion::Disponible);

    hab.servicios = readStringArray(doc, "servicios");
    hab.actualizadoEn = readDateOrNow(doc, "actualizadoEn");
    return hab;
}

hotel::models::Habitacion HabitacionRepository::create(
    const hotel::models::Habitacion& habitacion) {
    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        const auto serviciosArray = toBsonArray(habitacion.servicios);
        const auto doc = make_document(
            kvp("numero", habitacion.numero),
            kvp("tipo", habitacion.tipo),
            kvp("capacidad", habitacion.capacidad),
            kvp("tarifa", hotel::validators::roundCurrency(habitacion.tarifa)),
            kvp("estado", hotel::models::toString(habitacion.estado)),
            kvp("servicios", serviciosArray.view()),
            kvp("actualizadoEn", toBsonDate(habitacion.actualizadoEn))
        );

        const auto result = collection.insert_one(doc.view());
        if (!result || result->inserted_id().type() != bsoncxx::type::k_oid) {
            throw hotel::core::InternalServerException(
                "No se recibio identificador tras insertar la habitacion.");
        }

        hotel::models::Habitacion created = habitacion;
        created.id = result->inserted_id().get_oid().value.to_string();
        return created;
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        handleMongoException(
            ex,
            "HabitacionRepository::create",
            "Ya existe una habitacion registrada con ese numero.",
            "DUPLICATE_ROOM_NUMBER");
    }
}

PagedResult<hotel::models::Habitacion> HabitacionRepository::findAll(
    const HabitacionFilter& filter) {
    hotel::validators::validatePagination(filter.page, filter.limit);

    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        document queryBuilder;
        if (filter.estado.has_value()) {
            queryBuilder.append(kvp("estado", hotel::models::toString(*filter.estado)));
        }
        if (filter.tipo.has_value()) {
            const std::string normalizedTipo = hotel::validators::toLower(
                hotel::validators::trim(*filter.tipo));
            if (!normalizedTipo.empty()) {
                queryBuilder.append(kvp("tipo", normalizedTipo));
            }
        }
        if (filter.capacidadMin.has_value()) {
            queryBuilder.append(kvp("capacidad", make_document(kvp("$gte", *filter.capacidadMin))));
        }
        if (filter.tarifaMax.has_value()) {
            queryBuilder.append(kvp("tarifa", make_document(kvp("$lte", *filter.tarifaMax))));
        }

        const auto queryDoc = queryBuilder.extract();
        PagedResult<hotel::models::Habitacion> pageResult;
        pageResult.page = filter.page;
        pageResult.limit = filter.limit;
        pageResult.total = collection.count_documents(queryDoc.view());

        mongocxx::options::find findOpts;
        findOpts.sort(make_document(kvp("numero", 1)));
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
        handleMongoException(ex, "HabitacionRepository::findAll");
    }
}

std::vector<hotel::models::Habitacion> HabitacionRepository::findAllUnpaged(
    const std::optional<hotel::models::EstadoHabitacion>& estado,
    const std::optional<std::string>& tipo,
    const std::optional<std::int32_t>& capacidadMin) {
    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        document queryBuilder;
        if (estado.has_value()) {
            queryBuilder.append(kvp("estado", hotel::models::toString(*estado)));
        }
        if (tipo.has_value()) {
            const std::string normalizedTipo = hotel::validators::toLower(
                hotel::validators::trim(*tipo));
            if (!normalizedTipo.empty()) {
                queryBuilder.append(kvp("tipo", normalizedTipo));
            }
        }
        if (capacidadMin.has_value()) {
            queryBuilder.append(kvp("capacidad", make_document(kvp("$gte", *capacidadMin))));
        }

        const auto queryDoc = queryBuilder.extract();
        mongocxx::options::find findOpts;
        findOpts.sort(make_document(kvp("numero", 1)));

        std::vector<hotel::models::Habitacion> result;
        auto cursor = collection.find(queryDoc.view(), findOpts);
        for (const auto& doc : cursor) {
            result.push_back(fromDocument(doc));
        }
        return result;
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        handleMongoException(ex, "HabitacionRepository::findAllUnpaged");
    }
}

std::optional<hotel::models::Habitacion> HabitacionRepository::findById(const std::string& id) {
    const auto oid = parseOid(id, "habitacionId");

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
        handleMongoException(ex, "HabitacionRepository::findById");
    }
}

std::optional<hotel::models::Habitacion> HabitacionRepository::findByNumero(
    const std::string& numero) {
    const std::string normalized = hotel::validators::normalizeRoomNumber(numero);

    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        const auto filter = make_document(kvp("numero", normalized));
        const auto found = collection.find_one(filter.view());
        if (!found.has_value()) {
            return std::nullopt;
        }
        return fromDocument(found->view());
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        handleMongoException(ex, "HabitacionRepository::findByNumero");
    }
}

std::optional<hotel::models::Habitacion> HabitacionRepository::update(
    const hotel::models::Habitacion& habitacion) {
    const auto oid = parseOid(habitacion.id, "habitacionId");

    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        const auto serviciosArray = toBsonArray(habitacion.servicios);
        const auto filter = make_document(kvp("_id", oid));
        const auto updateDoc = make_document(kvp(
            "$set",
            make_document(
                kvp("numero", habitacion.numero),
                kvp("tipo", habitacion.tipo),
                kvp("capacidad", habitacion.capacidad),
                kvp("tarifa", hotel::validators::roundCurrency(habitacion.tarifa)),
                kvp("estado", hotel::models::toString(habitacion.estado)),
                kvp("servicios", serviciosArray.view()),
                kvp("actualizadoEn", toBsonDate(habitacion.actualizadoEn))
            )));

        const auto result = collection.update_one(filter.view(), updateDoc.view());
        if (!result || result->matched_count() == 0) {
            return std::nullopt;
        }

        return habitacion;
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        handleMongoException(
            ex,
            "HabitacionRepository::update",
            "Ya existe otra habitacion registrada con ese numero.",
            "DUPLICATE_ROOM_NUMBER");
    }
}

bool HabitacionRepository::updateEstado(const std::string& id,
                                        hotel::models::EstadoHabitacion estado,
                                        std::chrono::system_clock::time_point actualizadoEn) {
    const auto oid = parseOid(id, "habitacionId");

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
        handleMongoException(ex, "HabitacionRepository::updateEstado");
    }
}

bool HabitacionRepository::deleteById(const std::string& id) {
    const auto oid = parseOid(id, "habitacionId");

    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        const auto filter = make_document(kvp("_id", oid));
        const auto result = collection.delete_one(filter.view());
        return result.has_value() && result->deleted_count() > 0;
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        handleMongoException(ex, "HabitacionRepository::deleteById");
    }
}

}  // namespace hotel::repositories