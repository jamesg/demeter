#include "demeter/db.hpp"

#include "atlas/log/log.hpp"
#include "hades/crud.ipp"
#include "hades/join.hpp"
#include "hades/transaction.hpp"
#include "styx/serialise_json.hpp"

const char demeter::attr::base_ingredient_id[] = "base_ingredient_id";
const char demeter::attr::result_ingredient_id[] = "result_ingredient_id";
const char demeter::attr::ingredient_id[] = "ingredient_id";
const char demeter::attr::ingredient_instruction[] = "ingredient_instruction";
const char demeter::attr::ingredient_name[] = "ingredient_name";
const char demeter::attr::recipe_id[] = "recipe_id";
const char demeter::attr::recipe_cook_time[] = "recipe_cook_time";
const char demeter::attr::recipe_prep_time[] = "recipe_prep_time";
const char demeter::attr::recipe_quantity[] = "recipe_quantity";
const char demeter::attr::recipe_root_ingredient_id[] = "recipe_root_ingredient_id";
const char demeter::attr::recipe_title[] = "recipe_title";
const char demeter::attr::recipe_created[] = "recipe_created";
const char demeter::attr::recipe_updated[] = "recipe_updated";
const char demeter::attr::recipe_notes[] = "recipe_notes";
const char demeter::attr::category_id[] = "category_id";
const char demeter::attr::category_name[] = "category_name";
const char demeter::attr::category_description[] = "category_description";

const char demeter::relvar::component_of[] = "demeter_component_of";
const char demeter::relvar::ingredient[] = "demeter_ingredient";
const char demeter::relvar::recipe[] = "demeter_recipe";
const char demeter::relvar::recipe_created[] = "demeter_recipe_created";
const char demeter::relvar::recipe_last_updated[] = "demeter_recipe_last_updated";
const char demeter::relvar::recipe_notes[] = "demeter_recipe_notes";
const char demeter::relvar::category[] = "demeter_category";

namespace
{
    // Get an ingredient and its components recursively.
    demeter::ingredient get_ingredient_(hades::connection& conn, demeter::ingredient::id_type id)
    {
        demeter::ingredient out = hades::get_by_id<demeter::ingredient>(conn, id);
        styx::list components = hades::join<demeter::ingredient, demeter::component_of>(
            conn,
            hades::where(
                "ingredient.ingredient_id = component_of.base_ingredient_id "
                "AND component_of.result_ingredient_id = ?",
                hades::row<styx::int_type>(id.get_int<demeter::attr::ingredient_id>())
                )
            );
        for(styx::element& e : components)
        {
            demeter::ingredient c(e);
            auto id = c.id();
            demeter::ingredient sub = get_ingredient_(conn, id);
            styx::element sub_e = sub;
            out.components().append(sub_e);
        }
        return out;
    }

    // Save an ingredient and its components recursively.
    void save_(hades::connection& conn, demeter::ingredient& ingredient)
    {
        ingredient.save(conn);
        for(styx::element& e : ingredient.components())
        {
            demeter::ingredient component(e);
            save_(conn, component);
            demeter::component_of(component.id(), ingredient.id()).save(conn);
            e = component;
        }
    }
}

demeter::ingredient demeter::db::get_ingredient(hades::connection& conn, ingredient::id_type id)
{
    hades::transaction transaction(conn, "demeter_db_get_ingredient");
    auto in = get_ingredient_(conn, id);
    transaction.rollback();
    return in;
}

void demeter::db::save(hades::connection& conn, ingredient& ingredient)
{
    hades::transaction transaction(conn, "demeter_db_save_ingredient");
    save_(conn, ingredient);
    transaction.commit();
}

void demeter::db::save(hades::connection& conn, demeter::recipe& recipe)
{
    hades::transaction transaction(conn, "demeter_db_save_recipe");

    if(recipe.has_key("root_ingredient"))
    {
        ingredient root_ingredient(recipe.root_ingredient());
        atlas::log::information("demeter::db::save") <<
            "root ingredient " <<
            styx::serialise_json(recipe.root_ingredient());
        db::save(conn, root_ingredient);
        recipe.root_ingredient() = root_ingredient;

        recipe.get_int<attr::recipe_root_ingredient_id>() =
            root_ingredient.id().copy_int<attr::ingredient_id>();
        atlas::log::information("demeter::db::save") <<
            "recipe root ingredient id " <<
            recipe.get_int<attr::recipe_root_ingredient_id>();
    }

    if(recipe.save(conn))
        recipe_created(recipe.id()).update(conn);
    recipe_last_updated(recipe.id()).update(conn);
    transaction.commit();
}

demeter::recipe demeter::db::get_recipe(
        hades::connection& conn,
        const demeter::recipe::id_type id
        )
{
    hades::transaction transaction(conn, "demeter_db_get_recipe");

    demeter::recipe recipe = hades::get_by_id<demeter::recipe>(conn, id);
    if(recipe.has_key("root_ingredient_id"))
    {
        demeter::ingredient ingredient =
            db::get_ingredient(
                conn,
                demeter::ingredient::id_type{
                    recipe.copy_int<attr::recipe_root_ingredient_id>()
                }
            );
        recipe.root_ingredient() = ingredient;
    }
    return recipe;
}

void demeter::db::create(hades::connection& conn)
{
    hades::devoid(
        "CREATE TABLE IF NOT EXISTS demeter_ingredient ( "
        " ingredient_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        " ingredient_instruction VARCHAR, "
        " ingredient_name VARCHAR "
        ") ",
        conn
    );
    hades::devoid(
        "CREATE TABLE IF NOT EXISTS demeter_component_of ( "
        " base_ingredient_id INTEGER "
        "  REFERENCES demeter_ingredient(ingredient_id) ON DELETE CASCADE, "
        " result_ingredient_id INTEGER "
        "  REFERENCES demeter_ingredient(ingredient_id) ON DELETE CASCADE, "
        " UNIQUE(base_ingredient_id, result_ingredient_id) "
        ") ",
        conn
    );
    hades::devoid(
        "CREATE TABLE IF NOT EXISTS demeter_recipe ( "
        " recipe_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        " recipe_title VARCHAR, "
        " recipe_cook_time INTEGER, "
        " recipe_prep_time INTEGER, "
        " recipe_quantity INTEGER, "
        " root_ingredient_id REFERENCES demeter_ingredient(ingredient_id) "
        ") ",
        conn
    );
    hades::devoid(
        "CREATE TABLE IF NOT EXISTS demeter_category ( "
        " category_id INTEGER PRIMARY KEY AUTOINCREMENT, "
        " category_name VARCHAR, "
        " category_description VARCHAR "
        ") ",
        conn
    );
    hades::devoid(
        "CREATE TABLE IF NOT EXISTS demeter_recipe_created ( "
        " recipe_id INTEGER REFERENCES demeter_recipe(recipe_id), "
        " recipe_created INTEGER "
        ") ",
        conn
    );
    hades::devoid(
        "CREATE TABLE IF NOT EXISTS demeter_recipe_last_updated ( "
        " recipe_id INTEGER REFERENCES demeter_recipe(recipe_id), "
        " recipe_updated INTEGER "
        ") ",
        conn
    );
}
