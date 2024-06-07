set(BLEND_DIR ../../../src/draw/sw/blend)
file(GLOB_RECURSE DRAW_SOURCES ${BLEND_DIR}/lv_draw_sw_blend*.c)
file(GLOB_RECURSE ASM_SOURCES ${BLEND_DIR}/esp32/src/*.S)

set(STDLIB_DIR ../../../src/stdlib)
file(GLOB_RECURSE STDLIB_SOURCES ${STDLIB_DIR}/*.c)


idf_component_register(SRCS "test_app.c" "test_file.c" "../../../src/misc/lv_color.c" ${DRAW_SOURCES} ${ASM_SOURCES} ${STDLIB_SOURCES}
                    INCLUDE_DIRS ""  ${BLEND_DIR} ${BLEND_DIR}/esp32 ${STDLIB_DIR}
                    REQUIRES unity
                    WHOLE_ARCHIVE)

set_source_files_properties(
    ${DRAW_SOURCES}
    PROPERTIES COMPILE_OPTIONS
    "-DLV_LVGL_H_INCLUDE_SIMPLE;-Wno-format;"
    )