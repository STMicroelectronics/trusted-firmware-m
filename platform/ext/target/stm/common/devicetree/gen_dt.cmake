#-------------------------------------------------------------------------------
# Copyright (c) 2023 STMicroelectronics. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#-------------------------------------------------------------------------------
#
# default config
set(CROSS_COMPILE arm-none-eabi CACHE STRING  "Cross-compilation triplet")

# requirement
find_package(Python3)
find_program(CMAKE_C_COMPILER ${CROSS_COMPILE}-gcc)

# internal variables
set(DEVICETREE_DIR ${CMAKE_CURRENT_LIST_DIR})
set(DT_DTS_DIR ${DEVICETREE_DIR}/dts)
set(DT_BINDINGS_DIR ${DEVICETREE_DIR}/bindings)
set(DT_INCLUDE_DIR ${DEVICETREE_DIR}/include)
set(DT_GEN_DEFINES_SCRIPT ${DEVICETREE_DIR}/gen_defines.py)
set(DT_VENDOR_PREFIXES ${DT_BINDINGS_DIR}/vendor-prefixes.txt)
set(DT_PYTHON_DEVICETREE_SRC ${DEVICETREE_DIR}/python-devicetree/src)

function(dt_preprocess)
    set(req_single_args "OUT_FILE")
    set(single_args "DEPS_FILE;WORKING_DIRECTORY")
    set(req_multi_args "SOURCE_FILES")
    set(multi_args "EXTRA_CPPFLAGS;INCLUDE_DIRECTORIES")
    cmake_parse_arguments(DT_PREPROCESS "" "${req_single_args};${single_args}" "${req_multi_args};${multi_args}" ${ARGN})

    foreach(arg ${req_single_args} ${req_multi_args})
      if(NOT DEFINED DT_PREPROCESS_${arg})
        message(FATAL_ERROR "dt_preprocess() missing required argument: ${arg}")
      endif()
    endforeach()

    set(include_opts)
    foreach(dir ${DT_PREPROCESS_INCLUDE_DIRECTORIES})
      list(APPEND include_opts -isystem ${dir})
    endforeach()

    set(source_opts)
    foreach(file ${DT_PREPROCESS_SOURCE_FILES})
      list(APPEND source_opts -include ${file})
    endforeach()

    set(deps_opts)
    if(DEFINED DT_PREPROCESS_DEPS_FILE)
      list(APPEND deps_opts -MD -MF ${DT_PREPROCESS_DEPS_FILE})
    endif()

    set(workdir_opts)
    if(DEFINED DT_PREPROCESS_WORKING_DIRECTORY)
      list(APPEND workdir_opts WORKING_DIRECTORY ${DT_PREPROCESS_WORKING_DIRECTORY})
    endif()

    if(CMAKE_C_COMPILER STREQUAL "CMAKE_C_COMPILER-NOTFOUND")
        message(FATAL_ERROR "Could not find compiler: '${CMAKE_C_COMPILER}'")
    endif()

    set(preprocess_cmd ${CMAKE_C_COMPILER}
        -x assembler-with-cpp
	-nostdinc
	${include_opts}
	${source_opts}
	${NOSYSDEF_CFLAG}
	-D__DTS__
	${DT_PREPROCESS_EXTRA_CPPFLAGS}
	-P   #linemarker
	-E   # Stop after preprocessing
	${deps_opts}
	-o ${DT_PREPROCESS_OUT_FILE}
	${DEVICETREE_DIR}/misc/empty_file.c
	${workdir_opts})

    execute_process(COMMAND ${preprocess_cmd} COMMAND_ECHO STDOUT ECHO_OUTPUT_VARIABLE ECHO_ERROR_VARIABLE RESULT_VARIABLE ret)
    if(NOT "${ret}" STREQUAL "0")
	    message(FATAL_ERROR "failed to preprocess devicetree files (error code: ${ret})")
    endif()

endfunction()

function(gen_devicetree DTS_DIR DTS_BOARD OUT_DIR)
	# Parse arguments
	set(options "")
	set(oneValueArgs DTS_DIR DTS_BOARD OUT_DIR)
	set(multiValueArgs DTC_FLAGS)
	cmake_parse_arguments(PARSE_ARGV 0 ARG "${options}" "${oneValueArgs}" "${multiValueArgs}")

	set(DTS_FILE ${ARG_DTS_DIR}/${ARG_DTS_BOARD})

	if(NOT EXISTS ${DTS_FILE})
		message(FATAL_ERROR "dts:${DTS_FILE} not found")
	endif()

	set(OUT_DTS_POST_CPP ${ARG_OUT_DIR}/${ARG_DTS_BOARD}.pre.tmp)
	set(OUT_DTS_DEPS ${ARG_OUT_DIR}/${ARG_DTS_BOARD}.pre.d)

	get_filename_component(DTS_FILENAME "${ARG_DTS_BOARD}" NAME)
	get_filename_component(DTS_DIRNAME "${ARG_DTS_BOARD}" DIRECTORY)

	execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${ARG_OUT_DIR}/${DTS_DIRNAME})

	list(APPEND DTS_INC ${ARG_DTS_DIR} ${ARG_DTS_DIR}/${DTS_DIRNAME} ${DT_INCLUDE_DIR})
	if(EXISTS ${DT_VENDOR_PREFIXES})
		list(APPEND EXTRA_GEN_DEFINES_ARGS --vendor-prefixes ${DT_VENDOR_PREFIXES})
	endif()

	dt_preprocess(
		SOURCE_FILES ${DTS_FILE}
		OUT_FILE ${OUT_DTS_POST_CPP}
		DEPS_FILE ${OUT_DTS_DEPS}
		EXTRA_CPPFLAGS ${DTS_EXTRA_CPPFLAGS}
		INCLUDE_DIRECTORIES ${DTS_INC}
		WORKING_DIRECTORY ${ARG_OUT_DIR})

	set(ENV{PYTHONPATH} ${DT_PYTHON_DEVICETREE_SRC})

	set(CMD_GEN_DEFINES ${Python3_EXECUTABLE} ${DT_GEN_DEFINES_SCRIPT}
		--dts ${OUT_DTS_POST_CPP}
		--dtc-flags '${ARG_DTC_FLAGS}'
		--bindings-dirs ${DT_BINDINGS_DIR}
		--header-out ${ARG_OUT_DIR}/devicetree_generated.h
		--dts-out ${ARG_OUT_DIR}/out.dts # for debugging and dtc
		--edt-pickle-out ${ARG_OUT_DIR}/edt.pickle
		${EXTRA_GEN_DEFINES_ARGS})

	execute_process(COMMAND ${CMD_GEN_DEFINES} COMMAND_ECHO STDOUT ECHO_OUTPUT_VARIABLE ECHO_ERROR_VARIABLE RESULT_VARIABLE ret)
	if(NOT "${ret}" STREQUAL "0")
		message(FATAL_ERROR "failed to generated define files (error code: ${ret})")
	endif()

endfunction()

if(DEFINED DTS_BOARD AND DEFINED OUT_DIR)
	if (NOT EXISTS ${DTS_DIR})
		set(DTS_DIR ${DT_DTS_DIR})
	endif()

	gen_devicetree(
		DTS_DIR ${DTS_DIR}
		DTS_BOARD ${DTS_BOARD}
		OUT_DIR ${OUT_DIR}
		DTC_FLAGS ${DTC_FLAGS})
endif()
