project("Demeter")

include_directories("src")
include_directories("include")

# Demeter components
add_subdirectory(src)

# Web components
add_subdirectory(web)

add_dependencies(demeter-server-lib demeter-web)
