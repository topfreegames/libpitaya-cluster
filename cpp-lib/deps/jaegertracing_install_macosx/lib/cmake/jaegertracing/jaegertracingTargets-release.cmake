#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "jaegertracing::jaegertracing" for configuration "Release"
set_property(TARGET jaegertracing::jaegertracing APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(jaegertracing::jaegertracing PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libjaegertracing.dylib"
  IMPORTED_SONAME_RELEASE "libjaegertracing.dylib"
  )

list(APPEND _IMPORT_CHECK_TARGETS jaegertracing::jaegertracing )
list(APPEND _IMPORT_CHECK_FILES_FOR_jaegertracing::jaegertracing "${_IMPORT_PREFIX}/lib/libjaegertracing.dylib" )

# Import target "jaegertracing::jaegertracing-static" for configuration "Release"
set_property(TARGET jaegertracing::jaegertracing-static APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(jaegertracing::jaegertracing-static PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/libjaegertracing.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS jaegertracing::jaegertracing-static )
list(APPEND _IMPORT_CHECK_FILES_FOR_jaegertracing::jaegertracing-static "${_IMPORT_PREFIX}/lib/libjaegertracing.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
