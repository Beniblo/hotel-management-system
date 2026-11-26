#include <iostream>

#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/json.hpp>
#include <drogon/drogon.h>
#include <mongocxx/instance.hpp>

int main() {
    // Inicializacion unica obligatoria del driver oficial MongoDB C++
    mongocxx::instance instance{};

    using bsoncxx::builder::basic::kvp;
    using bsoncxx::builder::basic::make_document;

    const auto doc = make_document(
        kvp("sistema", "Gestion Hotelera"),
        kvp("estado", "smoke_build_ok")
    );

    std::cout << "[OK] Drogon version: " << drogon::getVersion() << '\n';
    std::cout << "[OK] BSON/mongocxx operativo: " << bsoncxx::to_json(doc.view()) << '\n';

    return 0;
}