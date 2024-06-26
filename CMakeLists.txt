#############################################################################
#Copyright (c) 2021,
#
#Distributed under the terms of the BSD 3-Clause License.
#
#The full license is in the file LICENSE, distributed with this software.
#############################################################################

cmake_minimum_required(VERSION 3.4.3)
project(xeus-wren)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

set(XEUS_WREN_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Versionning
# ===========

file(STRINGS "${XEUS_WREN_INCLUDE_DIR}/xeus-wren/xeus_wren_config.hpp" version_defines
     REGEX "#define XEUS_WREN_VERSION_(MAJOR|MINOR|PATCH)")
foreach (ver ${version_defines})
    if (ver MATCHES "#define XEUS_WREN_VERSION_(MAJOR|MINOR|PATCH) +([^ ]+)$")
        set(XEUS_WREN_VERSION_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE INTERNAL "")
    endif ()
endforeach ()
set(${PROJECT_NAME}_VERSION
    ${XEUS_WREN_VERSION_MAJOR}.${XEUS_WREN_VERSION_MINOR}.${XEUS_WREN_VERSION_PATCH})
message(STATUS "Building xeus-wren v${${PROJECT_NAME}_VERSION}")

# Configuration
# =============

include(GNUInstallDirs)

if (NOT DEFINED XEUS_WREN_KERNELSPEC_PATH)
    set(XEUS_WREN_KERNELSPEC_PATH "${CMAKE_INSTALL_FULL_BINDIR}/")
endif ()



configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels/xwren/kernel.json.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels/xwren/kernel.json"
)

# Build options
# =============

# Compilation options
option(XEUS_WREN_BUILD_STATIC "Build xeus-wren static library" OFF)
OPTION(XEUS_WREN_BUILD_SHARED "Split xwren build into executable and library" ON)
OPTION(XEUS_WREN_BUILD_EXECUTABLE "Build the xwren executable" ON)

OPTION(XEUS_WREN_USE_SHARED_XEUS "Link xwren  with the xeus shared library (instead of the static library)" ON)
OPTION(XEUS_WREN_USE_SHARED_XEUS_WREN "Link xwren  with the xeus shared library (instead of the static library)" ON)
OPTION(XEUS_WREN_EMSCRIPTEN_WASM_BUILD "Build for wasm with emscripten" OFF)

if(XEUS_WREN_EMSCRIPTEN_WASM_BUILD)
    add_compile_definitions(XEUS_WREN_EMSCRIPTEN_WASM_BUILD)
    message("Build with emscripten")
    SET(XEUS_WREN_BUILD_STATIC ON)
    SET(XEUS_WREN_BUILD_SHARED OFF)
    SET(XEUS_WREN_BUILD_EXECUTABLE OFF)
    SET(XEUS_WREN_USE_SHARED_XEUS OFF)
    SET(XEUS_WREN_USE_SHARED_XEUS_WREN OFF)
    SET(XEUS_WREN_BUILD_TESTS OFF)
endif()

# Dependencies
# ============


set(xeus_REQUIRED_VERSION 5.0.0)
find_package(xeus ${xeus_REQUIRED_VERSION} REQUIRED)


message(STATUS "Before searching for WREN")
find_package(WREN REQUIRED)
message(STATUS "After searching for WREN")

# Flags
# =====
include(CheckCXXCompilerFlag)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251 /wd4141")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4018 /wd4267 /wd4715 /wd4146 /wd4129")
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    if(NOT XEUS_WREN_EMSCRIPTEN_WASM_BUILD)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wunused-parameter -Wextra -Wreorder")
    endif()

    CHECK_CXX_COMPILER_FLAG("-std=c++17" HAS_CPP_17_FLAG)
    if (HAS_CPP_17_FLAG)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    else ()
        message(FATAL_ERROR "Unsupported compiler -- xeus requires C++17 support!")
    endif ()

endif ()

if(XEUS_WREN_EMSCRIPTEN_WASM_BUILD)
    # ENV (https://github.com/emscripten-core/emscripten/commit/6d9681ad04f60b41ef6345ab06c29bbc9eeb84e0)
    set(EMSCRIPTEN_FEATURES "${EMSCRIPTEN_FEATURES} -s \"EXTRA_EXPORTED_RUNTIME_METHODS=[ENV']\"")
endif()

# Source files
# ============

set(XEUS_WREN_HEADERS
    include/xeus-wren/xeus_wren_config.hpp
    include/xeus-wren/xinterpreter.hpp
)

set(XEUS_WREN_SRC
    src/xinterpreter.cpp 
    src/xio.cpp
    src/xcomplete.cpp
    src/xerror.cpp
    src/xjson.cpp
    src/xdisplay.cpp
)

set(XEUS_WREN_MAIN_SRC
    src/main.cpp
)


# Targets and link - Macros
# =========================

include(CheckCXXCompilerFlag)

string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib; ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

macro(xeus_wren_set_common_options target_name)
    if (MSVC)
        target_compile_options(${target_name} PUBLIC /wd4251 /wd4141)
        target_compile_options(${target_name} PUBLIC /wd4018 /wd4267 /wd4715 /wd4146 /wd4129)
    endif ()

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR
        CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR
        CMAKE_CXX_COMPILER_ID MATCHES "Intel")

        target_compile_options(${target_name} PUBLIC -Wunused-parameter -Wextra -Wreorder)

        # C++17 flag
        CHECK_CXX_COMPILER_FLAG("-std=c++17" HAS_CPP_17_FLAG)
        if (HAS_CPP_17_FLAG)
            target_compile_features(${target_name} PRIVATE cxx_std_17)
        else ()
            message(FATAL_ERROR "Unsupported compiler -- xeus-wren requires C++17 support!")
        endif ()
    endif ()

    if (APPLE)
        set_target_properties(${target_name} PROPERTIES
            MACOSX_RPATH ON
        )
    else ()
        set_target_properties(${target_name} PROPERTIES
            BUILD_WITH_INSTALL_RPATH 1
            SKIP_BUILD_RPATH FALSE
        )
    endif ()

    set_target_properties(${target_name} PROPERTIES
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
endmacro()

# Common macro kernels (xwren )
macro(xeus_wren_set_kernel_options target_name)
    if (XEUS_WREN_USE_SHARED_XEUS_WREN)
        target_link_libraries(${target_name} PRIVATE xeus-wren)
        if(CMAKE_DL_LIBS)
            target_link_libraries(${target_name} PRIVATE ${CMAKE_DL_LIBS} util)
        endif()
    else ()
        target_link_libraries(${target_name} PRIVATE xeus-wren-static)
    endif()
    if(NOT XEUS_WREN_EMSCRIPTEN_WASM_BUILD)
        find_package(Threads)
        target_link_libraries(${target_name} PRIVATE ${CMAKE_THREAD_LIBS_INIT} ${WREN_LIBRARIES})
    else()
        target_link_libraries(${target_name} PRIVATE ${WREN_LIBRARIES})
    endif()
endmacro()

# Common macro for shared and static library xeus-wren
macro(xeus_wren_create_target target_name linkage output_name)
    string(TOUPPER "${linkage}" linkage_upper)

    if (NOT ${linkage_upper} MATCHES "^(SHARED|STATIC)$")
        message(FATAL_ERROR "Invalid library linkage: ${linkage}")
    endif ()

    add_library(${target_name} ${linkage_upper} ${XEUS_WREN_SRC} ${XEUS_WREN_HEADERS})
    xeus_wren_set_common_options(${target_name})

    set_target_properties(${target_name} PROPERTIES
                          PUBLIC_HEADER "${XEUS_WREN_HEADERS}"
                          PREFIX ""
                          VERSION ${${PROJECT_NAME}_VERSION}
                          SOVERSION ${XEUS_WREN_VERSION_MAJOR}
                          OUTPUT_NAME "lib${output_name}")

    target_compile_definitions(${target_name} PUBLIC "XEUS_WREN_EXPORTS")

    target_compile_features(${target_name} PRIVATE cxx_std_17)

    target_include_directories(${target_name}
                               PUBLIC
                               ${WREN_INCLUDE_DIR}
                               $<BUILD_INTERFACE:${XEUS_WREN_INCLUDE_DIR}>
                               $<INSTALL_INTERFACE:include>)

    if (XEUS_WREN_USE_SHARED_XEUS)
        set(XEUS_WREN_XEUS_TARGET xeus)
    else ()
        set(XEUS_WREN_XEUS_TARGET xeus-static)
    endif ()

    target_link_libraries(${target_name} PUBLIC ${XEUS_WREN_XEUS_TARGET}  )
    if (WIN32 OR CYGWIN)
        #
    elseif (APPLE)
        target_link_libraries(${target_name} PRIVATE "-undefined dynamic_lookup")
    endif ()
    if(NOT XEUS_WREN_EMSCRIPTEN_WASM_BUILD)
        find_package(Threads) # TODO: add Threads as a dependence of xeus-static?
        target_link_libraries(${target_name} PRIVATE ${CMAKE_THREAD_LIBS_INIT} ${WREN_LIBRARY})
    else()
        target_link_libraries(${target_name} PRIVATE ${WREN_LIBRARY})
    endif()

endmacro()

# xeus-wren
# ===========

set(XEUS_WREN_TARGETS "")

if (XEUS_WREN_BUILD_SHARED)
    # Build libraries
    xeus_wren_create_target(xeus-wren SHARED xeus-wren)
    list(APPEND XEUS_WREN_TARGETS xeus-wren)
endif ()

if (XEUS_WREN_BUILD_STATIC)
    # On Windows, a static library should use a different output name
    # to avoid the conflict with the import library of a shared one.
    if (CMAKE_HOST_WIN32)
        xeus_wren_create_target(xeus-wren-static STATIC xeus-wren-static)
    else ()
        xeus_wren_create_target(xeus-wren-static STATIC xeus-wren)
    endif ()
    list(APPEND XEUS_WREN_TARGETS xeus-wren-static)
endif ()

# xeus-wren
# =======
if (XEUS_WREN_BUILD_EXECUTABLE)
    find_package(xeus-zmq 1.0.2 REQUIRED)
    add_executable(xwren ${XEUS_WREN_MAIN_SRC})
    target_compile_features(xwren PRIVATE cxx_std_17)
    xeus_wren_set_common_options(xwren)
    xeus_wren_set_kernel_options(xwren)
    target_link_libraries(xwren PRIVATE xeus-zmq)
endif()

if(XEUS_WREN_EMSCRIPTEN_WASM_BUILD)
    find_package(xeus-lite REQUIRED)
    add_executable(xeus_kernel src/main_emscripten_kernel.cpp )
    target_link_libraries(xeus_kernel PRIVATE xeus-wren-static xeus-static)
    target_compile_features(xeus_kernel PRIVATE cxx_std_17)
endif()

# Installation
# ============

include(CMakePackageConfigHelpers)

set(XEUS_WREN_CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}" CACHE STRING "install path for xeus-wrenConfig.cmake")

# Install xeus-wren and xeus-wren-static
if (XEUS_WREN_BUILD_SHARED)
    install(TARGETS ${XEUS_WREN_TARGETS}
            EXPORT ${PROJECT_NAME}-targets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/xeus-wren)

    # Makes the project importable from the build directory
    export(EXPORT ${PROJECT_NAME}-targets
           FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake")
endif ()

# Install xwren
if (XEUS_WREN_BUILD_EXECUTABLE)
    install(TARGETS xwren
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    # Configuration and data directories for jupyter and xeus-wren
    set(XJUPYTER_DATA_DIR "share/jupyter"    CACHE STRING "Jupyter data directory")

    # Install xwren Jupyter kernelspec
    set(KERNELSPEC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels)
    install(DIRECTORY ${KERNELSPEC_DIR}
            DESTINATION ${XJUPYTER_DATA_DIR}
            PATTERN "*.in" EXCLUDE)


    # Extra path for installing Jupyter kernelspec
    if (XEXTRA_JUPYTER_DATA_DIR)
        install(DIRECTORY ${KERNELSPEC_DIR}
                DESTINATION ${XEXTRA_JUPYTER_DATA_DIR}
                PATTERN "*.in" EXCLUDE)
    endif ()
endif ()

# Configure 'xeus-wrenConfig.cmake' for a build tree
set(XEUS_WREN_CONFIG_CODE "####### Expanded from \@XEUS_WREN_CONFIG_CODE\@ #######\n")
set(XEUS_WREN_CONFIG_CODE "${XEUS_WREN_CONFIG_CODE}set(CMAKE_MODULE_PATH \"${CMAKE_CURRENT_SOURCE_DIR}/cmake;\${CMAKE_MODULE_PATH}\")\n")
set(XEUS_WREN_CONFIG_CODE "${XEUS_WREN_CONFIG_CODE}##################################################")
configure_package_config_file(${PROJECT_NAME}Config.cmake.in
                              "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION ${PROJECT_BINARY_DIR})

# Configure 'xeus-wrenConfig.cmake.in for an install tree
set(XEUS_WREN_CONFIG_CODE "")
configure_package_config_file(${PROJECT_NAME}Config.cmake.in
                              "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION ${XEUS_WREN_CMAKECONFIG_INSTALL_DIR})

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
                                 VERSION ${${PROJECT_NAME}_VERSION}
                                 COMPATIBILITY AnyNewerVersion)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}Config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
              DESTINATION ${XEUS_WREN_CMAKECONFIG_INSTALL_DIR})

if (XEUS_WREN_BUILD_SHARED)
    install(EXPORT ${PROJECT_NAME}-targets
            FILE ${PROJECT_NAME}Targets.cmake
            DESTINATION ${XEUS_WREN_CMAKECONFIG_INSTALL_DIR})
endif ()
