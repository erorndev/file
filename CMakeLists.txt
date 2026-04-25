cmake_minimum_required(VERSION 3.15)
project(file-windows C)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
    foreach(flag_var
            CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
            CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
        if(${flag_var} MATCHES "/MD")
            string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
        endif()
    endforeach()
endif()

include(FetchContent)
FetchContent_Declare(
        pcre2
        GIT_REPOSITORY https://github.com/PCRE2Project/pcre2.git
        GIT_TAG        pcre2-10.44
)
set(PCRE2_BUILD_PCRE2GREP  OFF CACHE BOOL "" FORCE)
set(PCRE2_BUILD_TESTS      OFF CACHE BOOL "" FORCE)
set(PCRE2_SUPPORT_UNICODE  ON  CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS      OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(pcre2)

file(GLOB MAGIC_FRAGMENT_FILES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/magic/Magdir/*
)

set(MAGIC_STAGE_DIR ${CMAKE_CURRENT_BINARY_DIR}/magic)
set(MAGIC_OUTPUT    ${CMAKE_CURRENT_BINARY_DIR}/magic.mgc)

set(COMMON_DEFINES
        HAVE_CONFIG_H
        WIN32
        _CRT_SECURE_NO_WARNINGS
        _CRT_NONSTDC_NO_WARNINGS
        S_IFBLK=0x6000
        S_IFIFO=0x1000
)

set(LIB_SOURCES
        upstream/src/magic.c upstream/src/apprentice.c upstream/src/softmagic.c upstream/src/apptype.c
        upstream/src/ascmagic.c upstream/src/buffer.c upstream/src/compress.c upstream/src/encoding.c
        upstream/src/fsmagic.c upstream/src/funcs.c upstream/src/print.c upstream/src/readcdf.c
        upstream/src/readelf.c upstream/src/cdf_time.c upstream/src/is_tar.c upstream/src/cdf.c
        upstream/src/der.c upstream/src/is_csv.c upstream/src/is_json.c upstream/src/is_simh.c upstream/src/swap.c
        upstream/src/fmtcheck.c upstream/src/asprintf.c upstream/src/getline.c upstream/src/getopt_long.c
        upstream/src/strlcpy.c upstream/src/strlcat.c upstream/src/vasprintf.c upstream/src/pread.c
        upstream/src/strcasestr.c upstream/src/gmtime_r.c upstream/src/localtime_r.c
        upstream/src/asctime_r.c upstream/src/ctime_r.c win/symlink.c
)

add_library(magic STATIC ${LIB_SOURCES})

target_include_directories(magic
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/win
        ${CMAKE_CURRENT_SOURCE_DIR}/upstream/src
        INTERFACE
        ${pcre2_BINARY_DIR}
        ${pcre2_SOURCE_DIR}/src
)

target_compile_definitions(magic PRIVATE
        ${COMMON_DEFINES}
        PCRE2_CODE_UNIT_WIDTH=8
        HAVE_LIBPCRE2
)

target_link_libraries(magic PRIVATE pcre2-posix-static pcre2-8-static)

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(magic PRIVATE
            -Wno-error=incompatible-pointer-types
            -Wno-macro-redefined
    )
endif()

add_executable(file upstream/src/file.c)

target_include_directories(file PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/win
        ${CMAKE_CURRENT_SOURCE_DIR}/upstream/src
)

target_compile_definitions(file PRIVATE ${COMMON_DEFINES})

target_link_libraries(file PRIVATE magic shlwapi)

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    target_compile_options(file PRIVATE
            -Wno-error=incompatible-pointer-types
            -Wno-macro-redefined
    )
endif()

add_custom_command(
        OUTPUT ${MAGIC_OUTPUT}
        COMMAND ${CMAKE_COMMAND} -E rm -rf ${MAGIC_STAGE_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${MAGIC_STAGE_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/magic/Header
        ${MAGIC_STAGE_DIR}/Header
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/magic/Localstuff
        ${MAGIC_STAGE_DIR}/Localstuff
        COMMAND powershell -ExecutionPolicy Bypass -Command
        "Copy-Item -Path '${CMAKE_CURRENT_SOURCE_DIR}/magic/Magdir/*' -Destination '${MAGIC_STAGE_DIR}' -Force"
        COMMAND $<TARGET_FILE:file> -C -m magic
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        DEPENDS
        file
        ${CMAKE_CURRENT_SOURCE_DIR}/magic/Header
        ${CMAKE_CURRENT_SOURCE_DIR}/magic/Localstuff
        ${MAGIC_FRAGMENT_FILES}
        VERBATIM
)

add_custom_target(magic_database ALL
        DEPENDS ${MAGIC_OUTPUT}
)