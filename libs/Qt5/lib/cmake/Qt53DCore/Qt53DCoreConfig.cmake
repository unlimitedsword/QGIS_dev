if (CMAKE_VERSION VERSION_LESS 3.1.0)
    message(FATAL_ERROR "Qt 5 3DCore module requires at least CMake version 3.1.0")
endif()

get_filename_component(_qt53DCore_install_prefix "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

# For backwards compatibility only. Use Qt53DCore_VERSION instead.
set(Qt53DCore_VERSION_STRING 5.15.13)

set(Qt53DCore_LIBRARIES Qt5::3DCore)

macro(_qt5_3DCore_check_file_exists file)
    if(NOT EXISTS "${file}" )
        message(FATAL_ERROR "The imported target \"Qt5::3DCore\" references the file
   \"${file}\"
but this file does not exist.  Possible reasons include:
* The file was deleted, renamed, or moved to another location.
* An install or uninstall procedure did not complete successfully.
* The installation package was faulty and contained
   \"${CMAKE_CURRENT_LIST_FILE}\"
but not all the files it references.
")
    endif()
endmacro()


macro(_populate_3DCore_target_properties Configuration LIB_LOCATION IMPLIB_LOCATION
      IsDebugAndRelease)
    set_property(TARGET Qt5::3DCore APPEND PROPERTY IMPORTED_CONFIGURATIONS ${Configuration})

    set(imported_location "${_qt53DCore_install_prefix}/bin/${LIB_LOCATION}")
    _qt5_3DCore_check_file_exists(${imported_location})
    set(_deps
        ${_Qt53DCore_LIB_DEPENDENCIES}
    )
    set(_static_deps
    )

    set_target_properties(Qt5::3DCore PROPERTIES
        "IMPORTED_LOCATION_${Configuration}" ${imported_location}
        # For backward compatibility with CMake < 2.8.12
        "IMPORTED_LINK_INTERFACE_LIBRARIES_${Configuration}" "${_deps};${_static_deps}"
    )
    set_property(TARGET Qt5::3DCore APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                 "${_deps}"
    )


    set(imported_implib "${_qt53DCore_install_prefix}/lib/${IMPLIB_LOCATION}")
    _qt5_3DCore_check_file_exists(${imported_implib})
    if(NOT "${IMPLIB_LOCATION}" STREQUAL "")
        set_target_properties(Qt5::3DCore PROPERTIES
        "IMPORTED_IMPLIB_${Configuration}" ${imported_implib}
        )
    endif()
endmacro()

if (NOT TARGET Qt5::3DCore)

    set(_Qt53DCore_OWN_INCLUDE_DIRS "${_qt53DCore_install_prefix}/include/" "${_qt53DCore_install_prefix}/include/Qt3DCore")
    set(Qt53DCore_PRIVATE_INCLUDE_DIRS
        "${_qt53DCore_install_prefix}/include/Qt3DCore/5.15.13"
        "${_qt53DCore_install_prefix}/include/Qt3DCore/5.15.13/Qt3DCore"
    )

    foreach(_dir ${_Qt53DCore_OWN_INCLUDE_DIRS})
        _qt5_3DCore_check_file_exists(${_dir})
    endforeach()

    # Only check existence of private includes if the Private component is
    # specified.
    list(FIND Qt53DCore_FIND_COMPONENTS Private _check_private)
    if (NOT _check_private STREQUAL -1)
        foreach(_dir ${Qt53DCore_PRIVATE_INCLUDE_DIRS})
            _qt5_3DCore_check_file_exists(${_dir})
        endforeach()
    endif()

    set(Qt53DCore_INCLUDE_DIRS ${_Qt53DCore_OWN_INCLUDE_DIRS})

    set(Qt53DCore_DEFINITIONS -DQT_3DCORE_LIB)
    set(Qt53DCore_COMPILE_DEFINITIONS QT_3DCORE_LIB)
    set(_Qt53DCore_MODULE_DEPENDENCIES "Gui;Network;Core")


    set(Qt53DCore_OWN_PRIVATE_INCLUDE_DIRS ${Qt53DCore_PRIVATE_INCLUDE_DIRS})

    set(_Qt53DCore_FIND_DEPENDENCIES_REQUIRED)
    if (Qt53DCore_FIND_REQUIRED)
        set(_Qt53DCore_FIND_DEPENDENCIES_REQUIRED REQUIRED)
    endif()
    set(_Qt53DCore_FIND_DEPENDENCIES_QUIET)
    if (Qt53DCore_FIND_QUIETLY)
        set(_Qt53DCore_DEPENDENCIES_FIND_QUIET QUIET)
    endif()
    set(_Qt53DCore_FIND_VERSION_EXACT)
    if (Qt53DCore_FIND_VERSION_EXACT)
        set(_Qt53DCore_FIND_VERSION_EXACT EXACT)
    endif()

    set(Qt53DCore_EXECUTABLE_COMPILE_FLAGS "")

    foreach(_module_dep ${_Qt53DCore_MODULE_DEPENDENCIES})
        if (NOT Qt5${_module_dep}_FOUND)
            find_package(Qt5${_module_dep}
                5.15.13 ${_Qt53DCore_FIND_VERSION_EXACT}
                ${_Qt53DCore_DEPENDENCIES_FIND_QUIET}
                ${_Qt53DCore_FIND_DEPENDENCIES_REQUIRED}
                PATHS "${CMAKE_CURRENT_LIST_DIR}/.." NO_DEFAULT_PATH
            )
        endif()

        if (NOT Qt5${_module_dep}_FOUND)
            set(Qt53DCore_FOUND False)
            return()
        endif()

        list(APPEND Qt53DCore_INCLUDE_DIRS "${Qt5${_module_dep}_INCLUDE_DIRS}")
        list(APPEND Qt53DCore_PRIVATE_INCLUDE_DIRS "${Qt5${_module_dep}_PRIVATE_INCLUDE_DIRS}")
        list(APPEND Qt53DCore_DEFINITIONS ${Qt5${_module_dep}_DEFINITIONS})
        list(APPEND Qt53DCore_COMPILE_DEFINITIONS ${Qt5${_module_dep}_COMPILE_DEFINITIONS})
        list(APPEND Qt53DCore_EXECUTABLE_COMPILE_FLAGS ${Qt5${_module_dep}_EXECUTABLE_COMPILE_FLAGS})
    endforeach()
    list(REMOVE_DUPLICATES Qt53DCore_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES Qt53DCore_PRIVATE_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES Qt53DCore_DEFINITIONS)
    list(REMOVE_DUPLICATES Qt53DCore_COMPILE_DEFINITIONS)
    list(REMOVE_DUPLICATES Qt53DCore_EXECUTABLE_COMPILE_FLAGS)

    # It can happen that the same FooConfig.cmake file is included when calling find_package()
    # on some Qt component. An example of that is when using a Qt static build with auto inclusion
    # of plugins:
    #
    # Qt5WidgetsConfig.cmake -> Qt5GuiConfig.cmake -> Qt5Gui_QSvgIconPlugin.cmake ->
    # Qt5SvgConfig.cmake -> Qt5WidgetsConfig.cmake ->
    # finish processing of second Qt5WidgetsConfig.cmake ->
    # return to first Qt5WidgetsConfig.cmake ->
    # add_library cannot create imported target Qt5::Widgets.
    #
    # Make sure to return early in the original Config inclusion, because the target has already
    # been defined as part of the second inclusion.
    if(TARGET Qt5::3DCore)
        return()
    endif()

    set(_Qt53DCore_LIB_DEPENDENCIES "Qt5::Gui;Qt5::Network;Qt5::Core")


    add_library(Qt5::3DCore SHARED IMPORTED)


    set_property(TARGET Qt5::3DCore PROPERTY
      INTERFACE_INCLUDE_DIRECTORIES ${_Qt53DCore_OWN_INCLUDE_DIRS})
    set_property(TARGET Qt5::3DCore PROPERTY
      INTERFACE_COMPILE_DEFINITIONS QT_3DCORE_LIB)

    set_property(TARGET Qt5::3DCore PROPERTY INTERFACE_QT_ENABLED_FEATURES qt3d-render;qt3d-animation;qt3d-input;qt3d-logic;qt3d-extras)
    set_property(TARGET Qt5::3DCore PROPERTY INTERFACE_QT_DISABLED_FEATURES )

    # Qt 6 forward compatible properties.
    set_property(TARGET Qt5::3DCore
                 PROPERTY QT_ENABLED_PUBLIC_FEATURES
                 qt3d-render;qt3d-animation;qt3d-input;qt3d-logic;qt3d-extras)
    set_property(TARGET Qt5::3DCore
                 PROPERTY QT_DISABLED_PUBLIC_FEATURES
                 )
    set_property(TARGET Qt5::3DCore
                 PROPERTY QT_ENABLED_PRIVATE_FEATURES
                 assimp;qt3d-simd-sse2)
    set_property(TARGET Qt5::3DCore
                 PROPERTY QT_DISABLED_PRIVATE_FEATURES
                 qt3d-profile-gl;qt3d-profile-jobs;qt3d-simd-avx2;system-assimp)

    set_property(TARGET Qt5::3DCore PROPERTY INTERFACE_QT_PLUGIN_TYPES "")

    set(_Qt53DCore_PRIVATE_DIRS_EXIST TRUE)
    foreach (_Qt53DCore_PRIVATE_DIR ${Qt53DCore_OWN_PRIVATE_INCLUDE_DIRS})
        if (NOT EXISTS ${_Qt53DCore_PRIVATE_DIR})
            set(_Qt53DCore_PRIVATE_DIRS_EXIST FALSE)
        endif()
    endforeach()

    if (_Qt53DCore_PRIVATE_DIRS_EXIST)
        add_library(Qt5::3DCorePrivate INTERFACE IMPORTED)
        set_property(TARGET Qt5::3DCorePrivate PROPERTY
            INTERFACE_INCLUDE_DIRECTORIES ${Qt53DCore_OWN_PRIVATE_INCLUDE_DIRS}
        )
        set(_Qt53DCore_PRIVATEDEPS)
        foreach(dep ${_Qt53DCore_LIB_DEPENDENCIES})
            if (TARGET ${dep}Private)
                list(APPEND _Qt53DCore_PRIVATEDEPS ${dep}Private)
            endif()
        endforeach()
        set_property(TARGET Qt5::3DCorePrivate PROPERTY
            INTERFACE_LINK_LIBRARIES Qt5::3DCore ${_Qt53DCore_PRIVATEDEPS}
        )

        # Add a versionless target, for compatibility with Qt6.
        if(NOT "${QT_NO_CREATE_VERSIONLESS_TARGETS}" AND NOT TARGET Qt::3DCorePrivate)
            add_library(Qt::3DCorePrivate INTERFACE IMPORTED)
            set_target_properties(Qt::3DCorePrivate PROPERTIES
                INTERFACE_LINK_LIBRARIES "Qt5::3DCorePrivate"
            )
        endif()
    endif()

    _populate_3DCore_target_properties(RELEASE "Qt53DCore.dll" "Qt53DCore.lib" FALSE)

    if (EXISTS
        "${_qt53DCore_install_prefix}/bin/Qt53DCored.dll"
      AND EXISTS
        "${_qt53DCore_install_prefix}/lib/Qt53DCored.lib" )
        _populate_3DCore_target_properties(DEBUG "Qt53DCored.dll" "Qt53DCored.lib" FALSE)
    endif()



    # In Qt 5.15 the glob pattern was relaxed to also catch plugins not literally named Plugin.
    # Define QT5_STRICT_PLUGIN_GLOB or ModuleName_STRICT_PLUGIN_GLOB to revert to old behavior.
    if (QT5_STRICT_PLUGIN_GLOB OR Qt53DCore_STRICT_PLUGIN_GLOB)
        file(GLOB pluginTargets "${CMAKE_CURRENT_LIST_DIR}/Qt53DCore_*Plugin.cmake")
    else()
        file(GLOB pluginTargets "${CMAKE_CURRENT_LIST_DIR}/Qt53DCore_*.cmake")
    endif()

    macro(_populate_3DCore_plugin_properties Plugin Configuration PLUGIN_LOCATION
          IsDebugAndRelease)
        set_property(TARGET Qt5::${Plugin} APPEND PROPERTY IMPORTED_CONFIGURATIONS ${Configuration})

        set(imported_location "${_qt53DCore_install_prefix}/plugins/${PLUGIN_LOCATION}")
        _qt5_3DCore_check_file_exists(${imported_location})
        set_target_properties(Qt5::${Plugin} PROPERTIES
            "IMPORTED_LOCATION_${Configuration}" ${imported_location}
        )

    endmacro()

    if (pluginTargets)
        foreach(pluginTarget ${pluginTargets})
            include(${pluginTarget})
        endforeach()
    endif()



    _qt5_3DCore_check_file_exists("${CMAKE_CURRENT_LIST_DIR}/Qt53DCoreConfigVersion.cmake")
endif()

# Add a versionless target, for compatibility with Qt6.
if(NOT "${QT_NO_CREATE_VERSIONLESS_TARGETS}" AND TARGET Qt5::3DCore AND NOT TARGET Qt::3DCore)
    add_library(Qt::3DCore INTERFACE IMPORTED)
    set_target_properties(Qt::3DCore PROPERTIES
        INTERFACE_LINK_LIBRARIES "Qt5::3DCore"
    )
endif()
