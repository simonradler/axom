
##------------------------------------------------------------------------------
## uncrustify_shroud( YAML_INPUT_FILE file
##                    CFILES file
##                    CFG_FILE file
## )
##
## Runs uncrustify over the files generated by Shroud.
##
##  YAML_INPUT_FILE - YAML input file to shroud. Required.
##  CFILES          - File with list of generated C/C++ files to format.
##                    Optional, defaults to value used by add_shroud macro
##  CFG_FILE        - Uncrustify configuration file.
##                    Optional, defaults to ${PROJECT_SOURCE_DIR}/uncrustify.cfg.
##
## Add a target generate_${basename}_uncrustify where basename is generated from
## YAML_INPUT_FILE.  It is then added as a dependency to the generate target.
##
##------------------------------------------------------------------------------
macro(uncrustify_shroud)
    # Must be after a call to add_shroud macro
    # Only run uncrustify if shroud has just run

    set(options)
    set(singleValueArgs
        YAML_INPUT_FILE
        CFILES
        CFG_FILE
    )
    set(multiValueArgs "" )
    cmake_parse_arguments(arg
        "${options}" "${singleValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT arg_YAML_INPUT_FILE)
      message(FATAL_ERROR "uncrustify_shroud macro must define YAML_INPUT_FILE")
    endif()
    get_filename_component(_basename ${arg_YAML_INPUT_FILE} NAME_WE)

    if(NOT arg_CFILES)
      # This is the default location used by add_shroud
      set(arg_CFILES ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.cfiles)
    endif()

    if(NOT arg_CFG_FILE)
      set(arg_CFG_FILE ${PROJECT_SOURCE_DIR}/uncrustify.cfg)
    endif()

    if(UNCRUSTIFY_FOUND AND (EXISTS ${arg_CFG_FILE}))
        # _timestamp is created by add_shroud and is updated after Shroud is run
        set(_timestamp  ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.time)
        set(_uncrustify ${CMAKE_CURRENT_BINARY_DIR}/${_basename}.uncrustify)
        add_custom_command(
            OUTPUT ${_uncrustify}
            DEPENDS  ${_timestamp}
            COMMAND ${UNCRUSTIFY_EXECUTABLE}
                    -c ${arg_CFG_FILE} --no-backup `cat ${arg_CFILES}`
            COMMAND touch ${_uncrustify}
            COMMENT "Running uncrustify for ${arg_YAML_INPUT_FILE}."
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
        add_custom_target(generate_${_basename}_uncrustify
            DEPENDS ${_uncrustify}
        )
        if (TARGET generate)
            add_dependencies(generate generate_${_basename}_uncrustify)
        endif()
    endif()
endmacro(uncrustify_shroud)