#ifndef DEMETER_DB_HPP
#define DEMETER_DB_HPP

#include <set>

#include "atlas/db/temporal.hpp"
#include "hades/crud.hpp"
#include "hades/has_candidate_key.hpp"
#include "hades/relation.hpp"
#include "hades/tuple.hpp"

namespace demeter
{
    namespace attr
    {
        extern const char base_ingredient_id[];
        extern const char result_ingredient_id[];
        extern const char ingredient_id[];
        extern const char ingredient_instruction[];
        extern const char ingredient_name[];
        extern const char recipe_id[];
        extern const char recipe_cook_time[];
        extern const char recipe_prep_time[];
        extern const char recipe_quantity[];
        extern const char recipe_root_ingredient_id[];
        extern const char recipe_title[];
        extern const char recipe_created[];
        extern const char recipe_updated[];
        extern const char recipe_notes[];
        extern const char category_id[];
        extern const char category_name[];
        extern const char category_description[];
    }
    namespace relvar
    {
        extern const char component_of[];
        extern const char ingredient[];
        extern const char recipe[];
        extern const char recipe_created[];
        extern const char recipe_last_updated[];
        extern const char recipe_notes[];
        extern const char category[];
    }

    // An ingredient with a name and instruction on how to prepare it.
    class ingredient :
        public hades::tuple<
            attr::ingredient_id,
            attr::ingredient_instruction,
            attr::ingredient_name>,
        public hades::relation<relvar::ingredient>,
        public hades::has_candidate_key<attr::ingredient_id>,
        public hades::crud<ingredient>
    {
    public:
        ingredient()
        {
        }
        explicit ingredient(const styx::element& e) :
            styx::object(e)
        {
        }
        styx::list& components()
        {
            return get_list("components");
        }
    };

    // Record that an ingredient is a component of another.
    class component_of :
        public hades::tuple<
            attr::base_ingredient_id,
            attr::result_ingredient_id>,
        public hades::relation<relvar::component_of>,
        public hades::has_candidate_key<
            attr::base_ingredient_id,
            attr::result_ingredient_id>,
        public hades::crud<component_of>
    {
    public:
        component_of()
        {
        }
        explicit component_of(const styx::element& e) :
            styx::object(e)
        {
        }
        component_of(
            const ingredient::id_type& base_ingredient_id,
            const ingredient::id_type& result_ingredient_id
        )
        {
            get_int<attr::base_ingredient_id>() =
                base_ingredient_id.copy_int<attr::ingredient_id>();
            get_int<attr::result_ingredient_id>() =
                result_ingredient_id.copy_int<attr::ingredient_id>();
        }
        component_of(int base_ingredient_id, int result_ingredient_id)
        {
            get_int<attr::base_ingredient_id>() =
                base_ingredient_id;
            get_int<attr::result_ingredient_id>() =
                result_ingredient_id;
        }
    };

    class recipe :
        public hades::crud<recipe>,
        public hades::relation<relvar::recipe>,
        public hades::has_candidate_key<attr::recipe_id>,
        public hades::tuple<
            attr::recipe_id,
            attr::recipe_cook_time,
            attr::recipe_prep_time,
            attr::recipe_quantity,
            attr::recipe_root_ingredient_id,
            attr::recipe_title>
    {
    public:
        recipe()
        {
        }
        explicit recipe(const styx::element& e) :
            styx::object(e)
        {
        }
        styx::object& root_ingredient()
        {
            return get_object("root_ingredient");
        }
    };

    class category :
        public hades::crud<category>,
        public hades::relation<relvar::category>,
        public hades::has_candidate_key<attr::category_id>,
        public hades::tuple<
            attr::category_id,
            attr::category_name,
            attr::category_description>
    {
    public:
        category()
        {
        }
        explicit category(const styx::element& e) :
            styx::object(e)
        {
        }
    };

    typedef atlas::db::semi_temporal<
        recipe::id_type,
        relvar::recipe_created,
        attr::recipe_created> recipe_created;
    typedef atlas::db::semi_temporal<
        recipe::id_type,
        relvar::recipe_last_updated,
        attr::recipe_updated> recipe_last_updated;

    namespace db
    {
        /*!
         * \brief Retrieve an ingredient and all component ingredients
         * recursively.
         *
         * \note Retrieving the ingredient and all components recursively is
         * equivalent to querying for the transitive closure of the ingredient,
         * which is a second order function.
         */
        ingredient get_ingredient(hades::connection&, ingredient::id_type);
        /*!
         * \brief Save an ingredient and its components recursively.
         *
         * \note Saving the ingredient and all its components recursively is a
         * second order function.
         */
        void save(hades::connection&, ingredient&);
        /*!
         * \brief Save a recipe and all its components recursively.
         */
        void save(hades::connection&, recipe&);
        /*!
         * \brief Get a recipe, its root ingredient, and all components.
         */
        recipe get_recipe(hades::connection&, const recipe::id_type);
        /*!
         * \brief Create database tables for the recipe and related types.
         */
        void create(hades::connection&);
        /*!
         * \brief Get ingredients which have been 'orphaned' (are not part
         * of any recipe).
         */
        std::set<ingredient::id_type> orphaned(hades::connection&);
        /*!
         * \brief Get all ingredients of a recipe.
         */
        std::set<ingredient::id_type> ingredients(demeter::recipe&);
        /*!
         * \brief Get all ingredients of a recipe.
         */
        std::set<ingredient::id_type> ingredients(
                hades::connection&,
                demeter::recipe::id_type
                );
    }
}

#endif
