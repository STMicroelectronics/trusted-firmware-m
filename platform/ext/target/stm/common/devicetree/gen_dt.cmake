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

# output variables
set(GENERATED_DT_DIR ${CMAKE_BINARY_DIR}/generated/devicetree)

# toolchain_parse_make_rule: is a function that parses the output of
# 'gcc -M'.
#
# input_file: is in input parameter with the path to the file with
# the dependency information.
#
# include_files: is an output parameter with the result of parsing
# the include files.
function(toolchain_parse_make_rule input_file include_files)
	file(STRINGS ${input_file} input)

	# The file is formatted like this:
	# empty_file.o: misc/empty_file.c \
	# <dts file> \
	# <dtsi file> \
	# <h file> \

	# The dep file will contain `\` for line continuation.
	# This results in `\;` which is then treated a the char `;` instead of
	# the element separator, so let's get the pure `;` back.
	string(REPLACE "\;" ";" input_as_list ${input})

	# Pop the first line and treat it specially
	list(POP_FRONT input_as_list first_input_line)
	string(FIND ${first_input_line} ": " index)
	math(EXPR j "${index} + 2")
	string(SUBSTRING ${first_input_line} ${j} -1 first_include_file)

	# Remove whitespace before and after filename and convert to CMake path.
	string(STRIP "${first_include_file}" first_include_file)
	file(TO_CMAKE_PATH "${first_include_file}" first_include_file)
	set(result "${first_include_file}")

	# Remove whitespace before and after filename and convert to CMake path.
	foreach(file ${input_as_list})
		string(STRIP "${file}" file)
		file(TO_CMAKE_PATH "${file}" file)
		list(APPEND result "${file}")
	endforeach()

	set(${include_files} ${result} PARENT_SCOPE)

endfunction()

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

    unset(ret)

    #for debug add COMMAND_ECHO STDOUT ECHO_OUTPUT_VARIABLE
    execute_process(COMMAND ${preprocess_cmd} ECHO_ERROR_VARIABLE RESULT_VARIABLE ret)
    if(NOT "${ret}" STREQUAL "0")
	    message(FATAL_ERROR "failed to preprocess devicetree files (error code: ${ret})")
    endif()

endfunction()

macro(gen_devicetree_h)
	# Parse arguments
	set(options "")
	set(oneValueArgs TARGET DT_OUT_DIR DTS_BOARD)
	set(multiValueArgs DTS_DIR DTC_FLAGS)
	cmake_parse_arguments(A "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	#prepare
	file(REMOVE ${A_DT_OUT_DIR})
	file(MAKE_DIRECTORY ${A_DT_OUT_DIR})
	get_filename_component(${A_TARGET}_DTS_FILENAME "${A_DTS_BOARD}" NAME)
	set(${A_TARGET}_DTS_POST_CPP ${A_DT_OUT_DIR}/${${A_TARGET}_DTS_FILENAME}.pre.tmp)
	set(${A_TARGET}_DTS_DEPS ${A_DT_OUT_DIR}/${${A_TARGET}_DTS_FILENAME}.pre.d)

	#preprocess
	dt_preprocess(
		SOURCE_FILES ${A_DTS_BOARD}
		OUT_FILE ${${A_TARGET}_DTS_POST_CPP}
		DEPS_FILE ${${A_TARGET}_DTS_DEPS}
		EXTRA_CPPFLAGS ${DTS_EXTRA_CPPFLAGS}
		INCLUDE_DIRECTORIES ${A_DTS_DIR}
		WORKING_DIRECTORY ${A_DT_OUT_DIR})

	# Parse the generated dependency file to find the DT sources that
	# were included, including any transitive includes.
	toolchain_parse_make_rule(${${A_TARGET}_DTS_DEPS} ${A_TARGET}_DTS_INCLUDE_FILES)

	set(ENV{PYTHONPATH} ${DT_PYTHON_DEVICETREE_SRC})

	set(${A_TARGET}_CMD_GEN_DEFINES ${Python3_EXECUTABLE} ${DT_GEN_DEFINES_SCRIPT}
		--dts ${${A_TARGET}_DTS_POST_CPP}
		--dtc-flags '${A_DTC_FLAGS}'
		--bindings-dirs ${DT_BINDINGS_DIR}
		--header-out ${A_DT_OUT_DIR}/devicetree_generated.h
		--dts-out ${A_DT_OUT_DIR}/out.dts # for debugging and dtc
		--edt-pickle-out ${A_DT_OUT_DIR}/edt.pickle
		--vendor-prefixes ${DT_VENDOR_PREFIXES})

	#for debug add COMMAND_ECHO STDOUT ECHO_OUTPUT_VARIABLE
	execute_process(COMMAND ${${A_TARGET}_CMD_GEN_DEFINES} ECHO_ERROR_VARIABLE RESULT_VARIABLE ret)
	if(NOT "${ret}" STREQUAL "0")
		message(FATAL_ERROR "failed to generated define files (error code: ${ret})")
	endif()

	if (EXISTS ${A_DT_OUT_DIR}/devicetree_generated.h)
		message(STATUS "DEVICETREE: ${ARG_TARGET}: generation done")
	else()
		message(FATAL_ERROR "DEVICETREE: ${ARG_TARGET}: fail")
	endif()

endmacro()


macro(add_devicetree_target)
	# Parse arguments
	set(options "")
	set(oneValueArgs TARGET DTS_BOARD)
	set(multiValueArgs DTS_DIR DTC_FLAGS)
	cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	#check device tree source directory
	list(APPEND ${ARG_TARGET}_DTS_DIR ${DT_DTS_DIR} ${DT_INCLUDE_DIR})

	foreach(dir ${ARG_DTS_DIR})
		file(REAL_PATH ${dir} ${ARG_TARGET}_EXTRA_DIR EXPAND_TILDE)
		if (EXISTS ${${ARG_TARGET}_EXTRA_DIR})
			list(APPEND ${ARG_TARGET}_DTS_DIR ${${ARG_TARGET}_EXTRA_DIR})
		endif()
	endforeach()

	#check board file
	find_file(${ARG_TARGET}_DTS_BOARD NAME ${ARG_DTS_BOARD} PATHS ${${ARG_TARGET}_DTS_DIR})
	if(${ARG_TARGET}_DTS_BOARD STREQUAL ${ARG_TARGET}_DTS_BOARD-NOTFOUND)
		message(FATAL_ERROR "devicetree: board ${ARG_DTS_BOARD} not found in:\n ${${ARG_TARGET}_DTS_DIR}")
	endif()

	#prepare flags
	set(${ARG_TARGET}_DTC_FLAGS "")
	foreach(flags ${ARG_DTC_FLAGS})
		list(APPEND ${ARG_TARGET}_DTC_FLAGS ${flags})
	endforeach()

	#prepare out
	set(${ARG_TARGET}_DT_OUT_DIR ${GENERATED_DT_DIR}/${ARG_TARGET})
	set(${ARG_TARGET}_DT_GEN_H ${${ARG_TARGET}_DT_OUT_DIR}/devicetree_generated.h)

	gen_devicetree_h(
		TARGET ${ARG_TARGET}
		DTS_BOARD ${${ARG_TARGET}_DTS_BOARD}
		DTS_DIR ${${ARG_TARGET}_DTS_DIR}
		DTC_FLAGS ${${ARG_TARGET}_DTC_FLAGS}
		DT_OUT_DIR ${${ARG_TARGET}_DT_OUT_DIR}
	)

	set(GEN_DT_OPT
		-DTARGET=${ARG_TARGET}
		-DDTS_BOARD=${${ARG_TARGET}_DTS_BOARD}
		-DDTS_DIR:STRING="${${ARG_TARGET}_DTS_DIR}"
		-DDTC_FLAGS:STRING="${${ARG_TARGET}_DTC_FLAGS}"
		-DDT_OUT_DIR=${${ARG_TARGET}_DT_OUT_DIR}
	)

	add_custom_command(
		OUTPUT ${${ARG_TARGET}_DT_GEN_H}
		DEPENDS ${${ARG_TARGET}_DTS_INCLUDE_FILES}		
		COMMENT "DEVICETREE: ${ARG_TARGET}: preprocess and header re-generation"
		COMMAND ${CMAKE_COMMAND} ${GEN_DT_OPT} -P ${DEVICETREE_DIR}/gen_dt.cmake
	)

	add_custom_target(dt_${ARG_TARGET}_gen_h DEPENDS
		${${ARG_TARGET}_DT_GEN_H}
	)

	add_library(dt_${ARG_TARGET}_defs INTERFACE)
	target_include_directories(dt_${ARG_TARGET}_defs
		INTERFACE
			${DT_INCLUDE_DIR}
			${${ARG_TARGET}_DT_OUT_DIR}
	)

	# allow to add devicetree include directory at target with
	# devicetree link.
	target_link_libraries(${ARG_TARGET}
		PUBLIC
			dt_${ARG_TARGET}_defs
	)

	add_dependencies(${ARG_TARGET} dt_${ARG_TARGET}_gen_h)
endmacro()

if(DEFINED TARGET)
	string(REPLACE " " ";" LIST_DTS_DIR "${DTS_DIR}")
	string(REPLACE " " ";" LIST_DTC_FLAGS "${DTC_FLAGS}")

	gen_devicetree_h(
		TARGET ${TARGET}
		DTS_BOARD ${DTS_BOARD}
		DTS_DIR ${LIST_DTS_DIR}
		DTC_FLAGS ${LIST_DTC_FLAGS}
		DT_OUT_DIR ${DT_OUT_DIR}
	)

endif()
