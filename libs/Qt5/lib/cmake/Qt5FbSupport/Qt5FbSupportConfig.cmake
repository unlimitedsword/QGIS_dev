if (CMAKE_VERSION VERSION_LESS 3.1.0)
    message(FATAL_ERROR "Qt 5 FbSupport module requires at least CMake version 3.1.0")
endif()

get_filename_component(_qt5FbSupport_install_prefix "${CMAKE_CURRENT_LIST_DIR}/../../../" ABSOLUTE)

# For backwards compatibility only. Use Qt5FbSupport_VERSION instead.
set(Qt5FbSupport_VERSION_STRING 5.15.13)

set(Qt5FbSupport_LIBRARIES Qt5::FbSupport)

macro(_qt5_FbSupport_check_file_exists file)
    if(NOT EXISTS "${file}" )
        message(FATAL_ERROR "The imported target \"Qt5::FbSupport\" references the file
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

function(_qt5_FbSupport_process_prl_file prl_file_location Configuration lib_deps link_flags)
    set(_lib_deps)
    set(_link_flags)

    set(_qt5_install_libs "${_qt5FbSupport_install_prefix}/lib/")

    if(EXISTS "${prl_file_location}")
        file(STRINGS "${prl_file_location}" _prl_strings REGEX "QMAKE_PRL_LIBS_FOR_CMAKE[ \t]*=")

        # file(STRINGS) replaces all semicolons read from the file with backslash semicolons.
        # We need to do a reverse transformation in CMake. For that we replace all backslash
        # semicolons with just semicolons, but due to the qmake substitution feature
        # creating this file, we need to double the amount of backslashes, so the final file
        # should have three backslashes and one semicolon.
        string(REGEX REPLACE "\\\;" ";" _prl_strings "${_prl_strings}")

        string(REGEX REPLACE "QMAKE_PRL_LIBS_FOR_CMAKE[ \t]*=[ \t]*([^\n]*)" "\\1" _static_depends "${_prl_strings}")
        string(REGEX REPLACE "[ \t]+" ";" _standard_libraries "${CMAKE_CXX_STANDARD_LIBRARIES}")
        set(_search_paths)
        set(_fw_search_paths)
        set(_framework_flag)
        string(REPLACE "\$\$[QT_INSTALL_LIBS]" "${_qt5_install_libs}" _static_depends "${_static_depends}")
        foreach(_flag ${_static_depends})
            string(REPLACE "\"" "" _flag ${_flag})
            if(_flag MATCHES "^-framework$")
                # Handle the next flag as framework name
                set(_framework_flag 1)
            elseif(_flag MATCHES "^-F(.*)$")
                # Handle -F/foo/bar flags by recording the framework search paths to be used
                # by find_library.
                list(APPEND _fw_search_paths "${CMAKE_MATCH_1}")
            elseif(_framework_flag OR _flag MATCHES "^-l(.*)$")
                if(_framework_flag)
                    # Handle Darwin framework bundles passed as -framework Foo
                    set(_lib ${_flag})
                else()
                    # Handle normal libraries passed as -lfoo
                    set(_lib "${CMAKE_MATCH_1}")
                    foreach(_standard_library ${_standard_libraries})
                        if(_standard_library MATCHES "^${_lib}(\\.lib)?$")
                            set(_lib_is_default_linked TRUE)
                            break()
                        endif()
                    endforeach()
                endif()
                if (_lib_is_default_linked)
                    unset(_lib_is_default_linked)
                elseif(_lib MATCHES "^pthread$")
                    find_package(Threads REQUIRED)
                    list(APPEND _lib_deps Threads::Threads)
                else()
                    set(current_search_paths "${_search_paths}")
                    if(_framework_flag)
                        set(current_search_paths "${_fw_search_paths}")
                    endif()
                    if(current_search_paths)
                        find_library(_Qt5FbSupport_${Configuration}_${_lib}_PATH ${_lib} HINTS ${current_search_paths} NO_DEFAULT_PATH)
                    endif()
                    find_library(_Qt5FbSupport_${Configuration}_${_lib}_PATH ${_lib} HINTS ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES})
                    mark_as_advanced(_Qt5FbSupport_${Configuration}_${_lib}_PATH)
                    if(_Qt5FbSupport_${Configuration}_${_lib}_PATH)
                        list(APPEND _lib_deps
                            ${_Qt5FbSupport_${Configuration}_${_lib}_PATH}
                        )
                    else()
                        message(FATAL_ERROR "Library not found: ${_lib}")
                    endif()
                    unset(_framework_flag)
                endif()
            elseif(EXISTS "${_flag}")
                # The flag is an absolute path to an existing library
                list(APPEND _lib_deps "${_flag}")
            elseif(_flag MATCHES "^-L(.*)$")
                # Handle -Lfoo flags by putting their paths in the search path used by find_library above
                list(APPEND _search_paths "${CMAKE_MATCH_1}")
            else()
                # Handle all remaining flags by simply passing them to the linker
                list(APPEND _link_flags ${_flag})
            endif()
        endforeach()
    endif()

    string(REPLACE ";" " " _link_flags "${_link_flags}")
    set(${lib_deps} ${_lib_deps} PARENT_SCOPE)
    set(${link_flags} "SHELL:${_link_flags}" PARENT_SCOPE)
endfunction()

macro(_populate_FbSupport_target_properties Configuration LIB_LOCATION IMPLIB_LOCATION
      IsDebugAndRelease)
    set_property(TARGET Qt5::FbSupport APPEND PROPERTY IMPORTED_CONFIGURATIONS ${Configuration})

    set(imported_location "${_qt5FbSupport_install_prefix}/lib/${LIB_LOCATION}")
    _qt5_FbSupport_check_file_exists(${imported_location})
    set(_deps
        ${_Qt5FbSupport_LIB_DEPENDENCIES}
    )
    set(_static_deps
        ${_Qt5FbSupport_STATIC_${Configuration}_LIB_DEPENDENCIES}
    )

    set_target_properties(Qt5::FbSupport PROPERTIES
        "IMPORTED_LOCATION_${Configuration}" ${imported_location}
        # For backward compatibility with CMake < 2.8.12
        "IMPORTED_LINK_INTERFACE_LIBRARIES_${Configuration}" "${_deps};${_static_deps}"
    )
    set_property(TARGET Qt5::FbSupport APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                 "${_deps}"
    )

    if(NOT ${IsDebugAndRelease})
        set(_genex_condition "1")
    else()
        if(${Configuration} STREQUAL DEBUG)
            set(_genex_condition "$<CONFIG:Debug>")
        else()
            set(_genex_condition "$<NOT:$<CONFIG:Debug>>")
        endif()
    endif()

    if(_static_deps)
        set(_static_deps_genex "$<${_genex_condition}:${_static_deps}>")
        set_property(TARGET Qt5::FbSupport APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     "${_static_deps_genex}"
        )
    endif()

    set(_static_link_flags "${_Qt5FbSupport_STATIC_${Configuration}_LINK_FLAGS}")
    if(_static_link_flags)
        set(_static_link_flags_genex "$<${_genex_condition}:${_static_link_flags}>")
        if(NOT CMAKE_VERSION VERSION_LESS "3.13")
            set_property(TARGET Qt5::FbSupport APPEND PROPERTY INTERFACE_LINK_OPTIONS
                "${_static_link_flags_genex}"
            )
        else()
            # Abuse INTERFACE_LINK_LIBRARIES to add link flags when CMake version is too low.
            # Strip out SHELL:, because it is not supported in this property. And hope for the best.
            string(REPLACE "SHELL:" "" _static_link_flags_genex "${_static_link_flags_genex}")
            set_property(TARGET Qt5::FbSupport APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                "${_static_link_flags_genex}"
            )
        endif()
    endif()

    set(imported_implib "${_qt5FbSupport_install_prefix}/lib/${IMPLIB_LOCATION}")
    _qt5_FbSupport_check_file_exists(${imported_implib})
    if(NOT "${IMPLIB_LOCATION}" STREQUAL "")
        set_target_properties(Qt5::FbSupport PROPERTIES
        "IMPORTED_IMPLIB_${Configuration}" ${imported_implib}
        )
    endif()
endmacro()

if (NOT TARGET Qt5::FbSupport)

    set(_Qt5FbSupport_OWN_INCLUDE_DIRS "${_qt5FbSupport_install_prefix}/include/" "${_qt5FbSupport_install_prefix}/include/QtFbSupport")
    set(Qt5FbSupport_PRIVATE_INCLUDE_DIRS
        "${_qt5FbSupport_install_prefix}/include/QtFbSupport/5.15.13"
        "${_qt5FbSupport_install_prefix}/include/QtFbSupport/5.15.13/QtFbSupport"
    )

    foreach(_dir ${_Qt5FbSupport_OWN_INCLUDE_DIRS})
        _qt5_FbSupport_check_file_exists(${_dir})
    endforeach()

    # Only check existence of private includes if the Private component is
    # specified.
    list(FIND Qt5FbSupport_FIND_COMPONENTS Private _check_private)
    if (NOT _check_private STREQUAL -1)
        foreach(_dir ${Qt5FbSupport_PRIVATE_INCLUDE_DIRS})
            _qt5_FbSupport_check_file_exists(${_dir})
        endforeach()
    endif()

    set(Qt5FbSupport_INCLUDE_DIRS ${_Qt5FbSupport_OWN_INCLUDE_DIRS})

    set(Qt5FbSupport_DEFINITIONS -DQT_FB_SUPPORT_LIB)
    set(Qt5FbSupport_COMPILE_DEFINITIONS QT_FB_SUPPORT_LIB)
    set(_Qt5FbSupport_MODULE_DEPENDENCIES "Gui;Core")


    set(Qt5FbSupport_OWN_PRIVATE_INCLUDE_DIRS ${Qt5FbSupport_PRIVATE_INCLUDE_DIRS})

    set(_Qt5FbSupport_FIND_DEPENDENCIES_REQUIRED)
    if (Qt5FbSupport_FIND_REQUIRED)
        set(_Qt5FbSupport_FIND_DEPENDENCIES_REQUIRED REQUIRED)
    endif()
    set(_Qt5FbSupport_FIND_DEPENDENCIES_QUIET)
    if (Qt5FbSupport_FIND_QUIETLY)
        set(_Qt5FbSupport_DEPENDENCIES_FIND_QUIET QUIET)
    endif()
    set(_Qt5FbSupport_FIND_VERSION_EXACT)
    if (Qt5FbSupport_FIND_VERSION_EXACT)
        set(_Qt5FbSupport_FIND_VERSION_EXACT EXACT)
    endif()

    set(Qt5FbSupport_EXECUTABLE_COMPILE_FLAGS "")

    foreach(_module_dep ${_Qt5FbSupport_MODULE_DEPENDENCIES})
        if (NOT Qt5${_module_dep}_FOUND)
            find_package(Qt5${_module_dep}
                5.15.13 ${_Qt5FbSupport_FIND_VERSION_EXACT}
                ${_Qt5FbSupport_DEPENDENCIES_FIND_QUIET}
                ${_Qt5FbSupport_FIND_DEPENDENCIES_REQUIRED}
                PATHS "${CMAKE_CURRENT_LIST_DIR}/.." NO_DEFAULT_PATH
            )
        endif()

        if (NOT Qt5${_module_dep}_FOUND)
            set(Qt5FbSupport_FOUND False)
            return()
        endif()

        list(APPEND Qt5FbSupport_INCLUDE_DIRS "${Qt5${_module_dep}_INCLUDE_DIRS}")
        list(APPEND Qt5FbSupport_PRIVATE_INCLUDE_DIRS "${Qt5${_module_dep}_PRIVATE_INCLUDE_DIRS}")
        list(APPEND Qt5FbSupport_DEFINITIONS ${Qt5${_module_dep}_DEFINITIONS})
        list(APPEND Qt5FbSupport_COMPILE_DEFINITIONS ${Qt5${_module_dep}_COMPILE_DEFINITIONS})
        list(APPEND Qt5FbSupport_EXECUTABLE_COMPILE_FLAGS ${Qt5${_module_dep}_EXECUTABLE_COMPILE_FLAGS})
    endforeach()
    list(REMOVE_DUPLICATES Qt5FbSupport_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES Qt5FbSupport_PRIVATE_INCLUDE_DIRS)
    list(REMOVE_DUPLICATES Qt5FbSupport_DEFINITIONS)
    list(REMOVE_DUPLICATES Qt5FbSupport_COMPILE_DEFINITIONS)
    list(REMOVE_DUPLICATES Qt5FbSupport_EXECUTABLE_COMPILE_FLAGS)

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
    if(TARGET Qt5::FbSupport)
        return()
    endif()

    set(_Qt5FbSupport_LIB_DEPENDENCIES "Qt5::Gui;Qt5::Core")


    if(NOT Qt5_EXCLUDE_STATIC_DEPENDENCIES)

        _qt5_FbSupport_process_prl_file(
            "${_qt5FbSupport_install_prefix}/lib/Qt5FbSupport.prl" RELEASE
            _Qt5FbSupport_STATIC_RELEASE_LIB_DEPENDENCIES
            _Qt5FbSupport_STATIC_RELEASE_LINK_FLAGS
        )
    endif()

    add_library(Qt5::FbSupport STATIC IMPORTED)
    set_property(TARGET Qt5::FbSupport PROPERTY IMPORTED_LINK_INTERFACE_LANGUAGES CXX)


    set_property(TARGET Qt5::FbSupport PROPERTY
      INTERFACE_INCLUDE_DIRECTORIES ${_Qt5FbSupport_OWN_INCLUDE_DIRS})
    set_property(TARGET Qt5::FbSupport PROPERTY
      INTERFACE_COMPILE_DEFINITIONS QT_FB_SUPPORT_LIB)

    set_property(TARGET Qt5::FbSupport PROPERTY INTERFACE_QT_ENABLED_FEATURES )
    set_property(TARGET Qt5::FbSupport PROPERTY INTERFACE_QT_DISABLED_FEATURES )

    # Qt 6 forward compatible properties.
    set_property(TARGET Qt5::FbSupport
                 PROPERTY QT_ENABLED_PUBLIC_FEATURES
                 )
    set_property(TARGET Qt5::FbSupport
                 PROPERTY QT_DISABLED_PUBLIC_FEATURES
                 )
    set_property(TARGET Qt5::FbSupport
                 PROPERTY QT_ENABLED_PRIVATE_FEATURES
                 )
    set_property(TARGET Qt5::FbSupport
                 PROPERTY QT_DISABLED_PRIVATE_FEATURES
                 )

    set_property(TARGET Qt5::FbSupport PROPERTY INTERFACE_QT_PLUGIN_TYPES "")

    set(_Qt5FbSupport_PRIVATE_DIRS_EXIST TRUE)
    foreach (_Qt5FbSupport_PRIVATE_DIR ${Qt5FbSupport_OWN_PRIVATE_INCLUDE_DIRS})
        if (NOT EXISTS ${_Qt5FbSupport_PRIVATE_DIR})
            set(_Qt5FbSupport_PRIVATE_DIRS_EXIST FALSE)
        endif()
    endforeach()

    if (_Qt5FbSupport_PRIVATE_DIRS_EXIST)
        add_library(Qt5::FbSupportPrivate INTERFACE IMPORTED)
        set_property(TARGET Qt5::FbSupportPrivate PROPERTY
            INTERFACE_INCLUDE_DIRECTORIES ${Qt5FbSupport_OWN_PRIVATE_INCLUDE_DIRS}
        )
        set(_Qt5FbSupport_PRIVATEDEPS)
        foreach(dep ${_Qt5FbSupport_LIB_DEPENDENCIES})
            if (TARGET ${dep}Private)
                list(APPEND _Qt5FbSupport_PRIVATEDEPS ${dep}Private)
            endif()
        endforeach()
        set_property(TARGET Qt5::FbSupportPrivate PROPERTY
            INTERFACE_LINK_LIBRARIES Qt5::FbSupport ${_Qt5FbSupport_PRIVATEDEPS}
        )

        # Add a versionless target, for compatibility with Qt6.
        if(NOT "${QT_NO_CREATE_VERSIONLESS_TARGETS}" AND NOT TARGET Qt::FbSupportPrivate)
            add_library(Qt::FbSupportPrivate INTERFACE IMPORTED)
            set_target_properties(Qt::FbSupportPrivate PROPERTIES
                INTERFACE_LINK_LIBRARIES "Qt5::FbSupportPrivate"
            )
        endif()
    endif()

    _populate_FbSupport_target_properties(RELEASE "Qt5FbSupport.lib" "" FALSE)

    if (EXISTS "${_qt5FbSupport_install_prefix}/lib/Qt5FbSupportd.lib" )
        _populate_FbSupport_target_properties(DEBUG "Qt5FbSupportd.lib" "" FALSE)
    endif()






    _qt5_FbSupport_check_file_exists("${CMAKE_CURRENT_LIST_DIR}/Qt5FbSupportConfigVersion.cmake")
endif()

# Add a versionless target, for compatibility with Qt6.
if(NOT "${QT_NO_CREATE_VERSIONLESS_TARGETS}" AND TARGET Qt5::FbSupport AND NOT TARGET Qt::FbSupport)
    add_library(Qt::FbSupport INTERFACE IMPORTED)
    set_target_properties(Qt::FbSupport PROPERTIES
        INTERFACE_LINK_LIBRARIES "Qt5::FbSupport"
    )
endif()
