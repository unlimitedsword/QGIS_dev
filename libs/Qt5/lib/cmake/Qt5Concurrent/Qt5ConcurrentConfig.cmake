if (CMAKE_VERSION VERSION_LESS 3.1.0)
    message(FATAL_ERROR "Qt 5 Concurrent module requires at least CMake version 3.1.0")
endif()

get_filename_component(_qt5Concurrent_install_prefix "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

# For backwards compatibility only. Use Qt5Concurrent_VERSION instead.
set(Qt5Concurrent_VERSION_STRING 5.15.13)

set(Qt5Concurrent_LIBRARIES Qt5::Concurrent)

macro(_qt5_Concurrent_check_file_exists file)
    if(NOT EXISTS "${file}" )
        message(FATAL_ERROR "The imported target \"Qt5::Concurrent\" references the file
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


macro(_populate_Concurrent_target_properties Configuration LIB_LOCATION IMPLIB_LOCATION
      IsDebugAndRelease)
    set_property(TARGET Qt5::Concurrent APPEND PROPERTY IMPORTED_CONFIGURATIONS ${Configuration})

    set(imported_location "${_qt5Concurrent_install_prefix}/bin/${LIB_LOCATION}")
    _qt5_Concurrent_check_file_exists(${imported_location})
    set(_deps
        ${_Qt5Concurrent_LIB_DEPENDENCIES}
    )
    set(_static_deps
    )

    set_target_properties(Qt5::Concurrent PROPERTIES
        "IMPORTED_LOCATION_${Configuration}" ${imported_location}
        # For backward compatibility with CMake < 2.8.12
        "IMPORTED_LINK_INTERFACE_LIBRARIES_${Configuration}" "${_deps};${_static_deps}"
    )
    set_property(TARGET Qt5::Concurrent APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                 "${_deps}"
    )


    set(imported_implib "${_qt5Concurrent_install_prefix}/lib/${IMPLIB_LOCATION}")
    _qt5_Concurrent_check_file_exists(${imported_implib})
    if(NOT "${IMPLIB_LOCATION}" STREQUAL "")
        set_target_properties(Qt5::Concurrent PROPERTIES
        "IMPORTED_IMPLIB_${Configuration}" ${imported_implib}
        )
    endif()
endmacro()

if (NOT TARGET Qt5::Concurrent)

    set(_Qt5Concurrent_OWN_INCLUDE_DIRS "${_qt5Concurrent_install_prefix}/include/" "${_qt5Concurrent_install_prefix}/include/QtConcurrent")
    set(Qt5Concurrent_PRIVATE_INCLUDE_DIRS "")

    foreach(_dir ${_Qt5Concurrent_OWN_INCLUDE_DIRS})
        _qt5_Concurrent_check_file_exists(${_dir})
    endforeach()

    # Only check existence of private includes if the Private component is
    # specified.
    list(FIND Qt5Concurrent_FIND_COMPONENTS Private _check_private)
    if (NOT _check_private STREQUAL -1)
        foreach(_dir ${Qt5Concurrent_PRIVATE_INCLUDE_DIRS})
            _qt5_Concurrent_check_file_exists(${_dir})
        endforeach()
    endif()

    set(Qt5Concurrent_INCLUDE_DIRS ${_Qt5Concurrent_OWN_INCLUDE_DIRS})

    set(Qt5Concurrent_DEFINITIONS -DQT_CONCURRENT_LIB)
    set(Qt5Concurrent_COMPILE_DEFINITIONS QT_CONCURRENT_LIB)
    set(_Qt5Concurrent_MODULE_DEPENDENCIES "Core")


    set(Qt5Concurrent_OWN_PRIVATE_INCLUDE_DIRS ${Qt5Concurrent_PRIVATE_INCLUDE_DIRS})

    set(_Qt5Concurrent_FIND_DEPENDENCIES_REQUIRED)
    if (Qt5Concurrent_FIND_REQUIRED)
        set(_Qt5Concurrent_FIND_DEPENDENCIES_REQUIRED REQUIRED)
    endif()
    set(_Qt5Concurrent_FIND_DEPENDENCIES_QUIET)
    if (Qt5Concurrent_FIND_QUIETLY)
        set(_Qt5Concurrent_DEPENDENCIES_FIND_QUIET QUIET)
    endif()
    set(_Qt5Concurrent_FIND_VERSION_EXACT)
    if (Qt5Concurrent_FIND_VERSION_EXACT)
        set(_Qt5Concurrent_FIND_VERSION_EXACT EXACT)
    endif()

    set(Qt5Concurrent_EXECUTABLE_COMPILE_FLAGS "")

    foreach(_module_dep ${_Qt5Concurrent_MODULE_DEPENDENCIES})
        if (NOT Qt5${_module_dep}_FOUND)
            find_package(Qt5${_module_dep}
                5.15.13 ${_Qt5Concurrent_FIND_VERSION_EXACT}
                ${_Qt5Concurrent_DEPENDENCIES_FIND_QUIET}
                ${_Qt5Concurrent_FIND_DEPENDENCIES_REQUIRED}
                PATHS "${CMAKE_CURRENT_LIST_DIR}/.." NO_DEFAULT_PATH
            )
        endif()

        if (NOT Qt5${_module_dep}_FOUND)
            set(Qt5Concurrent_FOUND False)
            return()
        endif()

        list(APPEND Qt5Concurrent_INCLUDE_DIRS "${Qt5${_module_dep}_INCLUDE_DIRS}")
        list(APPEND Qt5Concurrent_PRIVATE_INCLUDE_DIRS "${Qt5${_module_dep}_PRIVATE_INCLUDE_DIRS}")
        list(APPEND Qt5Concurrent_DEFINITIONS ${Qt5${_module_dep}_DEFINITIONS})
        list(APPEND Qt5Concurrent_COMPILE_DEFINITIONS ${Qt5${_module_dep}_COMPILE_DEFINITIONS})
        list(APPEND Qt5Concurrent_EXECUTABLE_COMPILE_FLAGS ${Qt5${_module_dep}_EXECUTABLE_COMPILE_FLAGS})
    endforeach()
    list(REMOVE_DUPLICATES Qt5Concurrent_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES Qt5Concurrent_PRIVATE_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES Qt5Concurrent_DEFINITIONS)
    list(REMOVE_DUPLICATES Qt5Concurrent_COMPILE_DEFINITIONS)
    list(REMOVE_DUPLICATES Qt5Concurrent_EXECUTABLE_COMPILE_FLAGS)

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
    if(TARGET Qt5::Concurrent)
        return()
    endif()

    set(_Qt5Concurrent_LIB_DEPENDENCIES "Qt5::Core")


    add_library(Qt5::Concurrent SHARED IMPORTED)


    set_property(TARGET Qt5::Concurrent PROPERTY
      INTERFACE_INCLUDE_DIRECTORIES ${_Qt5Concurrent_OWN_INCLUDE_DIRS})
    set_property(TARGET Qt5::Concurrent PROPERTY
      INTERFACE_COMPILE_DEFINITIONS QT_CONCURRENT_LIB)

    set_property(TARGET Qt5::Concurrent PROPERTY INTERFACE_QT_ENABLED_FEATURES )
    set_property(TARGET Qt5::Concurrent PROPERTY INTERFACE_QT_DISABLED_FEATURES )

    # Qt 6 forward compatible properties.
    set_property(TARGET Qt5::Concurrent
                 PROPERTY QT_ENABLED_PUBLIC_FEATURES
                 )
    set_property(TARGET Qt5::Concurrent
                 PROPERTY QT_DISABLED_PUBLIC_FEATURES
                 )
    set_property(TARGET Qt5::Concurrent
                 PROPERTY QT_ENABLED_PRIVATE_FEATURES
                 )
    set_property(TARGET Qt5::Concurrent
                 PROPERTY QT_DISABLED_PRIVATE_FEATURES
                 )

    set_property(TARGET Qt5::Concurrent PROPERTY INTERFACE_QT_PLUGIN_TYPES "")

    set(_Qt5Concurrent_PRIVATE_DIRS_EXIST TRUE)
    foreach (_Qt5Concurrent_PRIVATE_DIR ${Qt5Concurrent_OWN_PRIVATE_INCLUDE_DIRS})
        if (NOT EXISTS ${_Qt5Concurrent_PRIVATE_DIR})
            set(_Qt5Concurrent_PRIVATE_DIRS_EXIST FALSE)
        endif()
    endforeach()

    if (_Qt5Concurrent_PRIVATE_DIRS_EXIST)
        add_library(Qt5::ConcurrentPrivate INTERFACE IMPORTED)
        set_property(TARGET Qt5::ConcurrentPrivate PROPERTY
            INTERFACE_INCLUDE_DIRECTORIES ${Qt5Concurrent_OWN_PRIVATE_INCLUDE_DIRS}
        )
        set(_Qt5Concurrent_PRIVATEDEPS)
        foreach(dep ${_Qt5Concurrent_LIB_DEPENDENCIES})
            if (TARGET ${dep}Private)
                list(APPEND _Qt5Concurrent_PRIVATEDEPS ${dep}Private)
            endif()
        endforeach()
        set_property(TARGET Qt5::ConcurrentPrivate PROPERTY
            INTERFACE_LINK_LIBRARIES Qt5::Concurrent ${_Qt5Concurrent_PRIVATEDEPS}
        )

        # Add a versionless target, for compatibility with Qt6.
        if(NOT "${QT_NO_CREATE_VERSIONLESS_TARGETS}" AND NOT TARGET Qt::ConcurrentPrivate)
            add_library(Qt::ConcurrentPrivate INTERFACE IMPORTED)
            set_target_properties(Qt::ConcurrentPrivate PROPERTIES
                INTERFACE_LINK_LIBRARIES "Qt5::ConcurrentPrivate"
            )
        endif()
    endif()

    _populate_Concurrent_target_properties(RELEASE "Qt5Concurrent.dll" "Qt5Concurrent.lib" FALSE)

    if (EXISTS
        "${_qt5Concurrent_install_prefix}/bin/Qt5Concurrentd.dll"
      AND EXISTS
        "${_qt5Concurrent_install_prefix}/lib/Qt5Concurrentd.lib" )
        _populate_Concurrent_target_properties(DEBUG "Qt5Concurrentd.dll" "Qt5Concurrentd.lib" FALSE)
    endif()



    # In Qt 5.15 the glob pattern was relaxed to also catch plugins not literally named Plugin.
    # Define QT5_STRICT_PLUGIN_GLOB or ModuleName_STRICT_PLUGIN_GLOB to revert to old behavior.
    if (QT5_STRICT_PLUGIN_GLOB OR Qt5Concurrent_STRICT_PLUGIN_GLOB)
        file(GLOB pluginTargets "${CMAKE_CURRENT_LIST_DIR}/Qt5Concurrent_*Plugin.cmake")
    else()
        file(GLOB pluginTargets "${CMAKE_CURRENT_LIST_DIR}/Qt5Concurrent_*.cmake")
    endif()

    macro(_populate_Concurrent_plugin_properties Plugin Configuration PLUGIN_LOCATION
          IsDebugAndRelease)
        set_property(TARGET Qt5::${Plugin} APPEND PROPERTY IMPORTED_CONFIGURATIONS ${Configuration})

        set(imported_location "${_qt5Concurrent_install_prefix}/plugins/${PLUGIN_LOCATION}")
        _qt5_Concurrent_check_file_exists(${imported_location})
        set_target_properties(Qt5::${Plugin} PROPERTIES
            "IMPORTED_LOCATION_${Configuration}" ${imported_location}
        )

    endmacro()

    if (pluginTargets)
        foreach(pluginTarget ${pluginTargets})
            include(${pluginTarget})
        endforeach()
    endif()



    _qt5_Concurrent_check_file_exists("${CMAKE_CURRENT_LIST_DIR}/Qt5ConcurrentConfigVersion.cmake")
endif()

# Add a versionless target, for compatibility with Qt6.
if(NOT "${QT_NO_CREATE_VERSIONLESS_TARGETS}" AND TARGET Qt5::Concurrent AND NOT TARGET Qt::Concurrent)
    add_library(Qt::Concurrent INTERFACE IMPORTED)
    set_target_properties(Qt::Concurrent PROPERTIES
        INTERFACE_LINK_LIBRARIES "Qt5::Concurrent"
    )
endif()
