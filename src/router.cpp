#include "demeter/router.hpp"

#include <boost/bind.hpp>

#include "atlas/http/server/mimetypes.hpp"
#include "atlas/http/server/response.hpp"
#include "atlas/http/server/static_string.hpp"
#include "atlas/http/server/static_text.hpp"
#include "demeter/db.hpp"
#include "hades/crud.ipp"

#define DEMETER_DECLARE_STATIC_STRING(NAME) ATLAS_DECLARE_STATIC_STRING(demeter, NAME)
#define DEMETER_STATIC_STD_STRING(NAME) ATLAS_STATIC_STD_STRING(demeter, NAME)

DEMETER_DECLARE_STATIC_STRING(index_html)
DEMETER_DECLARE_STATIC_STRING(index_js)

demeter::router::router(
    boost::shared_ptr<boost::asio::io_service> io,
    hades::connection& conn
) :
    application_router(io)
{
    install_static_text("/", "html", DEMETER_STATIC_STD_STRING(index_html));
    install_static_text("/index.html", DEMETER_STATIC_STD_STRING(index_html));
    install_static_text("/index.js", DEMETER_STATIC_STD_STRING(index_js));

    //
    // Recipe.
    //

    install<>(
        atlas::http::matcher("/recipe", "GET"),
        [&conn]() {
            return atlas::http::json_response(recipe::get_collection(conn));
        }
    );
    install_json<recipe>(
        atlas::http::matcher("/recipe", "POST"),
        [&conn](recipe r) {
            db::save(conn, r);
            return atlas::http::json_response(r);
        }
    );
    install<styx::int_type>(
        atlas::http::matcher("/recipe/([0-9]+)", "GET"),
        [&conn](styx::int_type recipe_id) {
            return atlas::http::json_response(
                db::get_recipe(conn, recipe::id_type{recipe_id})
            );
        }
    );
    install_json<recipe, styx::int_type>(
        atlas::http::matcher("/recipe/([0-9]+)", "PUT"),
        [&conn](recipe r, styx::int_type recipe_id) {
            db::save(conn, r);
            return atlas::http::json_response(r);
        }
    );
    install<styx::int_type>(
        atlas::http::matcher("/recipe/([0-9]+)", "DELETE"),
        [&conn](styx::int_type recipe_id) {
            recipe r;
            r.set_id(recipe::id_type{recipe_id});
            r.destroy(conn);
            return atlas::http::json_response(true);
        }
    );
}
