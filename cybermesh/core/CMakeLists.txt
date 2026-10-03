# ESP-IDF component for the portable floor core. Host builds use the Makefile.
idf_component_register(SRCS "src/cm_core.c" "src/cm_sha256.c"
                       INCLUDE_DIRS "include")
