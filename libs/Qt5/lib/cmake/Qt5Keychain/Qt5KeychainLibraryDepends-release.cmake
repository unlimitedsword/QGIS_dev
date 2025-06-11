#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "qt5keychain" for configuration "Release"
set_property(TARGET qt5keychain APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(qt5keychain PROPERTIES
  IMPORTED_IMPLIB_RELEASE "${_IMPORT_PREFIX}/lib/qt5keychain.lib"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/qt5keychain.dll"
  )

list(APPEND _cmake_import_check_targets qt5keychain )
list(APPEND _cmake_import_check_files_for_qt5keychain "${_IMPORT_PREFIX}/lib/qt5keychain.lib" "${_IMPORT_PREFIX}/bin/qt5keychain.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
