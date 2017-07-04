cmake_minimum_required(VERSION 2.8.12)

project(ddb_copy_info)

# -Wstrict-prototypes is also supported in clang 4.0 and newer,
# but since clang, unlike gcc, ignores unknown -W options, we
# don't need to do anything special to support older versions.
set(COMPILE_OPTIONS -std=c99 -Wall -Wextra -pedantic -Wstrict-prototypes)

set(DEADBEEF_INC "" CACHE STRING "Semicolon-separated list of additional paths to search for DeaDBeeF API headers")


find_package(PkgConfig REQUIRED)

set(HAS_TARGETS FALSE)
foreach(GTK_VERSION 2 3)
    pkg_check_modules(GTK${GTK_VERSION} gtk+-${GTK_VERSION}.0)

    if (NOT GTK${GTK_VERSION}_FOUND)
        message("Target for GTK ${GTK_VERSION} version of the plugin was disabled since dependencies are not satisfied")
    else()
        set(HAS_TARGETS TRUE)
        set(TARGET gtk${GTK_VERSION})

        add_library(${TARGET} SHARED main.c)
        set_target_properties(
            ${TARGET} PROPERTIES
            PREFIX ""
            OUTPUT_NAME "ddb_copy_info_gtk${GTK_VERSION}")
        target_compile_options(${TARGET} PRIVATE ${COMPILE_OPTIONS})
        target_include_directories(${TARGET} PRIVATE ${DEADBEEF_INC})

        target_include_directories(
            ${TARGET} PRIVATE ${GTK${GTK_VERSION}_INCLUDE_DIRS})
        target_link_libraries(${TARGET} ${GTK${GTK_VERSION}_LIBRARIES})

        unset(TARGET)
    endif()
endforeach(GTK_VERSION)

if(NOT HAS_TARGETS)
    message(FATAL_ERROR "Please install development packages for GTK 2 and/or 3")
endif()
