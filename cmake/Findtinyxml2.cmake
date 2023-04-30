
include(FindPackageHandleStandardArgs)

message("Searching tinyxml2 library...")

if(DEFINED ENV{<TINYXML2_DIR>})
  message("TINYXML2_DIR = $ENV{TINYXML2_DIR}")
endif()

find_library(TINYXML2_LIBRARY NAMES tinyxml2 PATHS ENV TINYXML2_DIR PATH_SUFFIXES "bin" "lib" REQUIRED)
find_path(TINYXML2_INCLUDE NAMES "tinyxml2.h" PATHS ENV TINYXML2_DIR PATH_SUFFIXES "include" REQUIRED)
get_filename_component(TINYXML2_DIR ${TINYXML2_INCLUDE} DIRECTORY)

find_package_handle_standard_args(tinyxml2 REQUIRED_VARS TINYXML2_LIBRARY TINYXML2_INCLUDE)

if (TINYXML2_FOUND)
  mark_as_advanced(TINYXML2_LIBRARY)
  message("tinyxml2 found: ${TINYXML2_DIR}")
else()
  message("Could not find tinyxml2, please specify TINYXML2_DIR env variable.")
endif()

if (TINYXML2_FOUND AND NOT TARGET tinyxml2)
  message("TINYXML2_INCLUDE: ${TINYXML2_INCLUDE}")

  add_library(tinyxml2 SHARED IMPORTED GLOBAL)
  
  if (WIN32)
    set_target_properties(tinyxml2 PROPERTIES
      IMPORTED_IMPLIB ${TINYXML2_LIBRARY}
      IMPORTED_LOCATION "${TINYXML2_DIR}/bin/tinyxml2.dll"
      INTERFACE_INCLUDE_DIRECTORIES ${TINYXML2_INCLUDE})
  else()
    message("tinyxml2 library: ${TINYXML2_DIR}/lib/libtinyxml2.so")
    set_target_properties(tinyxml2 PROPERTIES
      IMPORTED_LOCATION "${TINYXML2_DIR}/lib/libtinyxml2.so"
      INTERFACE_INCLUDE_DIRECTORIES ${TINYXML2_INCLUDE})
  endif()
endif()
