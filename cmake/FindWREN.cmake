# - Try to find wren
# Once done this will define
# WREN_FOUND - System has wren
# WREN_INCLUDE_DIRS - The wren include directories
# WREN_LIBRARIES - The libraries needed to use wren

find_package(PkgConfig)
find_path(WREN_INCLUDE_DIR wren.hpp)

find_library(WREN_LIBRARY NAMES wren)

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and setWREN_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(WREN  DEFAULT_MSG
                                 WREN_LIBRARY WREN_INCLUDE_DIR)

mark_as_advanced(WREN_INCLUDE_DIR WREN_LIBRARY )

set(WREN_LIBRARIES ${WREN_LIBRARY} )
set(WREN_INCLUDE_DIRS ${WREN_INCLUDE_DIR} )
