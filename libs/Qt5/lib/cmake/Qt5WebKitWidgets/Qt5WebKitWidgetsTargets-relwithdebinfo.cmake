#----------------------------------------------------------------
# Generated CMake target import file for configuration "RelWithDebInfo".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "Qt5::WebKitWidgets" for configuration "RelWithDebInfo"
set_property(TARGET Qt5::WebKitWidgets APPEND PROPERTY IMPORTED_CONFIGURATIONS RELWITHDEBINFO)
set_target_properties(Qt5::WebKitWidgets PROPERTIES
  IMPORTED_IMPLIB_RELWITHDEBINFO "${_IMPORT_PREFIX}/lib/Qt5WebKitWidgets.lib"
  IMPORTED_LINK_DEPENDENT_LIBRARIES_RELWITHDEBINFO "Qt5::PrintSupport"
  IMPORTED_LOCATION_RELWITHDEBINFO "${_IMPORT_PREFIX}/bin/Qt5WebKitWidgets.dll"
  )

list(APPEND _cmake_import_check_targets Qt5::WebKitWidgets )
list(APPEND _cmake_import_check_files_for_Qt5::WebKitWidgets "${_IMPORT_PREFIX}/lib/Qt5WebKitWidgets.lib" "${_IMPORT_PREFIX}/bin/Qt5WebKitWidgets.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
