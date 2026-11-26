#include "hotel/repositories/HuespedRepository.hpp"

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

bsoncxx::array::value toBsonArray(const std::vector<std::string>& items) {
    array arr;
    for (const auto& item : items) {
        arr.append(item);
    }
    return arr.extract();
}

}  // namespace

HuespedRepository::HuespedRepository(hotel::database::MongoConnectionManager& connectionManager)
    : connectionManager_(connectionManager) {}

hotel::models::Huesped HuespedRepository::fromDocument(const bsoncxx::document::view& doc) {
    hotel::models::Huesped h;
    if (const auto idElem = doc["_id"]; idElem && idElem.type() == bsoncxx::type::k_oid) {
        h.id = idElem.get_oid().value.to_string();
    }
    h.nombre = readStringOrEmpty(doc, "nombre");
    h.correo = readStringOrEmpty(doc, "correo");
    h.telefono = readStringOrEmpty(doc, "telefono");
    h.fechaRegistro = readDateOrNow(doc, "fechaRegistro");
    h.preferencias = readStringArray(doc, "preferencias");
    h.activo = readBoolOrDefault(doc, "activo", true);
    h.actualizadoEn = readDateOrNow(doc, "actualizadoEn");
    return h;
}

hotel::models::Huesped HuespedRepository::create(const hotel::models::Huesped& huesped) {
    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        const auto prefsArray = toBsonArray(huesped.preferencias);
        const auto doc = make_document(
            kvp("nombre", huesped.nombre),
            kvp("correo", huesped.correo),
            kvp("telefono", huesped.telefono),
            kvp("fechaRegistro", toBsonDate(huesped.fechaRegistro)),
            kvp("preferencias", prefsArray.view()),
            kvp("activo", huesped.activo),
            kvp("actualizadoEn", toBsonDate(huesped.actualizadoEn))
        );

        const auto result = collection.insert_one(doc.view());
        if (!result || result->inserted_id().type() != bsoncxx::type::k_oid) {
            throw hotel::core::InternalServerException(
                "No se recibio identificador tras insertar el huesped.");
        }

        hotel::models::Huesped created = huesped;
        created.id = result->inserted_id().get_oid().value.to_string();
        return created;
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        handleMongoException(
            ex,
            "HuespedRepository::create",
            "Ya existe un huesped registrado con ese correo electronico.",
            "DUPLICATE_EMAIL");
    }
}

PagedResult<hotel::models::Huesped> HuespedRepository::findAll(const HuespedFilter& filter) {
    hotel::validators::validatePagination(filter.page, filter.limit);

    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        document queryBuilder;
        std::string nombrePattern;
        std::string correoPattern;

        if (filter.nombre.has_value()) {
            const std::string cleaned = hotel::validators::trim(*filter.nombre);
            if (!cleaned.empty()) {
                nombrePattern = escapeRegex(cleaned);
                queryBuilder.append(kvp("nombre", bsoncxx::types::b_regex{nombrePattern, "i"}));
            }
        }
        if (filter.correo.has_value()) {
            const std::string cleaned = hotel::validators::normalizeEmail(*filter.correo);
            if (!cleaned.empty()) {
                correoPattern = escapeRegex(cleaned);
                queryBuilder.append(kvp("correo", bsoncxx::types::b_regex{correoPattern, "i"}));
            }
        }
        if (filter.activo.has_value()) {
            queryBuilder.append(kvp("activo", *filter.activo));
        }

        const auto queryDoc = queryBuilder.extract();
        PagedResult<hotel::models::Huesped> pageResult;
        pageResult.page = filter.page;
        pageResult.limit = filter.limit;
        pageResult.total = collection.count_documents(queryDoc.view());

        mongocxx::options::find findOpts;
        findOpts.sort(make_document(kvp("nombre", 1)));
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
        handleMongoException(ex, "HuespedRepository::findAll");
    }
}

std::optional<hotel::models::Huesped> HuespedRepository::findById(const std::string& id) {
    const auto oid = parseOid(id, "huespedId");

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
        handleMongoException(ex, "HuespedRepository::findById");
    }
}

std::optional<hotel::models::Huesped> HuespedRepository::findByEmail(const std::string& correo) {
    const std::string normalized = hotel::validators::normalizeEmail(correo);

    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        const auto filter = make_document(kvp("correo", normalized));
        const auto found = collection.find_one(filter.view());
        if (!found.has_value()) {
            return std::nullopt;
        }
        return fromDocument(found->view());
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        handleMongoException(ex, "HuespedRepository::findByEmail");
    }
}

std::optional<hotel::models::Huesped> HuespedRepository::update(
    const hotel::models::Huesped& huesped) {
    const auto oid = parseOid(huesped.id, "huespedId");

    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        const auto prefsArray = toBsonArray(huesped.preferencias);
        const auto filter = make_document(kvp("_id", oid));
        const auto updateDoc = make_document(kvp(
            "$set",
            make_document(
                kvp("nombre", huesped.nombre),
                kvp("correo", huesped.correo),
                kvp("telefono", huesped.telefono),
                kvp("preferencias", prefsArray.view()),
                kvp("activo", huesped.activo),
                kvp("actualizadoEn", toBsonDate(huesped.actualizadoEn))
            )));

        const auto result = collection.update_one(filter.view(), updateDoc.view());
        if (!result || result->matched_count() == 0) {
            return std::nullopt;
        }

        return huesped;
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        handleMongoException(
            ex,
            "HuespedRepository::update",
            "Ya existe otro huesped registrado con ese correo electronico.",
            "DUPLICATE_EMAIL");
    }
}

bool HuespedRepository::setActive(const std::string& id,
                                  bool activo,
                                  std::chrono::system_clock::time_point actualizadoEn) {
    const auto oid = parseOid(id, "huespedId");

    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        const auto filter = make_document(kvp("_id", oid));
        const auto updateDoc = make_document(kvp(
            "$set",
            make_document(
                kvp("activo", activo),
                kvp("actualizadoEn", toBsonDate(actualizadoEn))
            )));

        const auto result = collection.update_one(filter.view(), updateDoc.view());
        return result.has_value() && result->matched_count() > 0;
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        handleMongoException(ex, "HuespedRepository::setActive");
    }
}

bool HuespedRepository::deleteById(const std::string& id) {
    const auto oid = parseOid(id, "huespedId");

    try {
        auto client = connectionManager_.acquireClient();
        auto collection = connectionManager_.database(*client)[kCollectionName];

        const auto filter = make_document(kvp("_id", oid));
        const auto result = collection.delete_one(filter.view());
        return result.has_value() && result->deleted_count() > 0;
    } catch (const hotel::core::AppException&) {
        throw;
    } catch (const mongocxx::exception& ex) {
        handleMongoException(ex, "HuespedRepository::deleteById");
    }
}

}  // namespace hotel::repositories