#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Qt5::WebKit" for configuration "RelWithDebInfo"
set_property(TARGET Qt5::WebKit APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(Qt5::WebKit PROPERTIES
  IMPORTED_IMPLIB_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/Qt5WebKit.lib"
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELWITHDEBINFO "Qt5::Quick;Qt5::WebChannel;Qt5::Positioning"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/bin/Qt5WebKit.dll"
  )

list(APPEND _cmake_import_check_targets Qt5::WebKit )
list(APPEND _cmake_import_check_files_for_Qt5::WebKit "${_IMPORT_PREFIX}/lib/Qt5WebKit.lib" "${_IMPORT_PREFIX}/bin/Qt5WebKit.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
