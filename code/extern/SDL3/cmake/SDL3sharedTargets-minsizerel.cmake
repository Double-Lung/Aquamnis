#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "SDL3::SDL3-shared" for configuration "MinSizeRel"
set_property(TARGET SDL3::SDL3-shared APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(SDL3::SDL3-shared PROPERTIES
  IMPORTED_IMPLIB_MINSIZEREL "${_IMPORT_PREFIX}/lib/SDL3.lib"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/bin/SDL3.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS SDL3::SDL3-shared )
list(APPEND _IMPORT_CHECK_FILES_FOR_SDL3::SDL3-shared "${_IMPORT_PREFIX}/lib/SDL3.lib" "${_IMPORT_PREFIX}/bin/SDL3.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
