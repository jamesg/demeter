//
// Models.
//

var restUri = function(fragment) {
    return fragment;
};

var Ingredient = Backbone.Model.extend({
    defaults: {
        ingredient_name: '',
        ingredient_instruction: ''
    },
    idAttribute: 'ingredient_id',
    constructor: function() {
        this._components = new IngredientCollection();
        Backbone.Model.apply(this, arguments);
    },
    sync: function() {
        // Ingredients are transferred with the recipe.
    },
    parse: function(data) {
        this._components.reset(data['components'], { parse: true });
        this._components.each((function(model) { model.collection = this._components; }).bind(this));
        console.log('parse components', data['components']);
        return _.omit(data, 'components');
    },
    toJSON: function() {
        var out = Backbone.Model.prototype.toJSON.apply(this, arguments);
        out.components = this._components.map(
            function(component) {
                return component.toJSON();
            }
        );
        return out;
    },
    components: function() {
        return this._components;
    }
})

var IngredientCollection = Backbone.Collection.extend({
    model: Ingredient
});

var Recipe = RestModel.extend({
    defaults: {
        recipe_title: '',
        recipe_quantity: 0,
        recipe_cook_time: 0,
        recipe_prep_time: 0
    },
    idAttribute: 'recipe_id',
    url: function() {
        return restUri(this.isNew() ? 'recipe' : 'recipe/' + this.id);
    },
    initialize: function() {
        RestModel.prototype.initialize.apply(this, arguments);
        this._rootIngredient = new Ingredient;
    },
    parse: function() {
        var out = RestModel.prototype.parse.apply(this, arguments);
        this._rootIngredient = new Ingredient(
            out['root_ingredient'],
            { parse: true }
        );
        return out;
    },
    toJSON: function() {
        var out = RestModel.prototype.toJSON.apply(this, arguments);
        out.root_ingredient = this._rootIngredient.toJSON();
        return out;
    },
    rootIngredient: function() {
        return this._rootIngredient;
    }
});

var RecipeCollection = RestCollection.extend({
    model: Recipe,
    url: restUri('recipe')
})

//
// Pages.
//

var HomePage = PageView.extend({
    pageTitle: 'Home',
    initialize: function() {
        PageView.prototype.initialize.apply(this, arguments);
        PageView.prototype.render.apply(this);

        var recipes = new RecipeCollection;
        recipes.fetch();
        (new CollectionView({
            model: recipes,
            el: this.$('ul[name=recipes]'),
            view: StaticView.extend({
                tagName: 'li',
                template: '<%-recipe_title%>',
                events: {
                    click: function() {
                        gApplication.pushPage(
                            new RecipePage({
                                model: this.model
                            })
                        )
                    }
                }
            })
        })).render();
    },
    events: {
        'click button[name=new-recipe]': function() {
            var recipe = new Recipe;
            var m = new Modal({
                model: recipe,
                view: StaticView.extend({
                    initialize: function() {
                        StaticView.prototype.initialize.apply(this, arguments);
                        this.on('create', this.save.bind(this));
                        this.on('save', this.save.bind(this));
                    },
                    template: '\
                    <div class="aligned">\
                        <div class="group">\
                            <label>\
                            Recipe Title\
                            <input type="text" name="recipe_title"></input>\
                            </label>\
                        </div>\
                        <div class="group">\
                            <label>\
                            Preparation Time\
                            <input type="text" name="recipe_prep_time"></input>\
                            </label>\
                        </div>\
                        <div class="group">\
                            <label>\
                            Cooking Time\
                            <input type="text" name="recipe_cook_time"></input>\
                            </label>\
                        </div>\
                    </div>\
                    ',
                    save: function() {
                        this.model.set({
                            recipe_title: this.$('input[name=recipe_title]').val(),
                            recipe_prep_time: this.$('input[name=recipe_prep_time]').val(),
                            recipe_cook_time: this.$('input[name=recipe_cook_time]').val()
                        });
                    }
                }),
                buttons: [
                    StandardButton.cancel(),
                    StandardButton.create()
                ]
            });
            this.listenTo(
                m,
                'create',
                function() {
                    recipe.save({
                        success: function() {
                            m.finish();
                            gApplication.pushPage(
                                new RecipePage({ model: recipe })
                            );
                        }
                    })
                }
            );
            gApplication.modal(m);
        }
    },
    render: function() {},
    template: $('#homepage-template').html()
});

var IngredientDetailView = StaticView.extend({
    template: '\
    <div class="aligned">\
        <div class="group">\
            <label>\
                Ingredient\
                <input type="text" name="ingredient_name" value="<%-ingredient_name%>"></input>\
            </label>\
        </div>\
        <div class="group">\
            <label>\
                Instruction\
                <input type="text" name="ingredient_instruction" value="<%-ingredient_instruction%>"></input>\
            </label>\
        </div>\
    </div>\
    <div class="button-box">\
        <button type="button" name="new-component" class="button-create">\
            <span class="oi" data-glyph="plus" title="New Component" aria-hidden="true"></span>\
        </button>\
        <%if(!root) {%>\
        <button type="button" name="delete-ingredient" class="button-delete">\
            <span class="oi" data-glyph="trash" title="Delete Ingredient" aria-hidden="true"></span>\
        </button>\
        <%}%>\
        <button type="button" name="insert" class="button-primary">\
            <span class="oi" data-glyph="collapse-right" title="Insert Component" aria-hidden="true"></span>\
        </button>\
    </div>\
    ',
    templateParams: function() {
        var params = this.model.attributes;
        _.extend(
            params,
            { root: this.model.collection == null }
        );
        return params;
    },
    events: {
        'click button[name=new-component]': function() {
            this.model.components().push(new Ingredient);
        },
        'click button[name=delete-ingredient]': function() {
            this.model.components().each(
                (function(component) {
                    this.model.collection.add(component);
                }).bind(this)
            );
            this.model.components().reset();
            this.model.destroy();
        },
        'click button[name=insert]': function() {
            var ingredient = new Ingredient;
            this.model.components().each(
                function(component) { ingredient.components().add(component);  }
            );
            this.model.components().reset();
            this.model.components().add(ingredient);
        }
    },
    save: function() {
        this.model.set({
            ingredient_name: this.$('input[name=ingredient_name]').val(),
            ingredient_instruction: this.$('input[name=ingredient_instruction]').val()
        });
    }
});

var IngredientView = StaticView.extend({
    tagName: 'div',
    className: 'ingredient',
    initialize: function() {
        StaticView.prototype.initialize.apply(this, arguments);
        StaticView.prototype.render.apply(this);

        this._detailView = new IngredientDetailView({
            el: this.$('div[name=details]'),
            model: this.model
        });

        this._componentsView = new CollectionView({
            el: this.$('div[name=components]'),
            view: IngredientView,
            emptyView: StaticView.extend({ template: '&nbsp;' }),
            model: this.model.components()
        });

        this.render();
    },
    template: '\
        <div class="ingredient-components" name="components"></div>\
        <div class="ingredient-details" name="details"></div>\
        ',
    render: function() {
        this._detailView.render();
        this._componentsView.render();
    },
    save: function() {
        this._detailView.save();
        this._componentsView.each(function(view) { view.save(); });
    },
    delegateEvents: function() {
        StaticView.prototype.delegateEvents.apply(this);
        this._detailView.delegateEvents();
        this._componentsView.delegateEvents();
    }
});

var RecipePage = PageView.extend({
    pageTitle: function() { return this.model.get('recipe_title'); },
    initialize: function() {
        PageView.prototype.initialize.apply(this, arguments);
        PageView.prototype.render.apply(this);

        (new StaticView({
            el: this.$('div[name=recipe-details]'),
            model: this.model,
            template: '<h2><%-recipe_title%></h2>'
        })).render();

        (new StaticView({
            el: this.$('div[name=root-ingredient]'),
            template: '<p>Loading...</p>'
        })).render();

        this.model.fetch({
            success: (function() {
                this._rootIngredientView = new IngredientView({
                    el: this.$('div[name=root-ingredient]'),
                    model: this.model.rootIngredient()
                });
                this._rootIngredientView.render();
            }).bind(this)
        });
    },
    template: '\
        <div class="recipe-details" name="recipe-details"></div>\
        <div class="ingredient recipe-root-ingredient" name="root-ingredient"></div>\
        <div class="button-box">\
            <button type="button" name="save" class="button-primary">\
                <span class="oi" data-glyph="data-transfer-download" aria-hidden="true"></span>\
                Save\
            </button>\
        </div>\
    ',
    events: {
        'click button[name=save]': function() {
            this._rootIngredientView.save();
            this.model.save();
            console.log(this.model.toJSON());
        }
    },
    render: function() {}
})
