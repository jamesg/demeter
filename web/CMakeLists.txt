atlas_add_web_library(
    PREFIX demeter
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/static.a
    TARGET demeter-web
    SOURCES
        index.html
        index.js
)
