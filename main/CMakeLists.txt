file(GLOB_RECURSE SOURCES ./*.c)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS .)

idf_component_get_property(LVGL_LIB lvgl__lvgl COMPONENT_LIB)
target_compile_options(
    ${LVGL_LIB} 
    PRIVATE
        -DLV_LVGL_H_INCLUDE_SIMPLE
        -DLV_USE_DEMO_MUSIC
)
