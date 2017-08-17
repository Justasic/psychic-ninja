# A macro to recursively get source files, however if the source folder has
# a CMakeLists.txt file, include it as part of the build instead.
macro(GetSources SRC CPPSOURCES)
	if(NOT ${SRC} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR} AND EXISTS "${SRC}/CMakeLists.txt")
		# If we have a CMakeLists.txt file, include it instead of trying to compile
		# sources as part of main project.
		add_subdirectory("${SRC}")
	else(NOT ${SRC} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR} AND EXISTS "${SRC}/CMakeLists.txt")
		# get a list of all files in the directory.
		file(GLOB SRC_FLDR "${SRC}/*")
		foreach(SOURCEFILE ${SRC_FLDR})
			# If it's a directory, recursively call this function.
			if (IS_DIRECTORY "${SOURCEFILE}")
				GetSources(${SOURCEFILE} ${CPPSOURCES})
			else (IS_DIRECTORY "${SOURCEFILE}")
				# Otherwise look for source files and append them.
				# Look for C++ files.
				string(REGEX MATCH "\\.cpp$" CPP ${SOURCEFILE})
				if (CPP)
					list(APPEND ${CPPSOURCES} ${SOURCEFILE})
				endif (CPP)

				# Look for C files.
				string(REGEX MATCH "\\.c$" C ${SOURCEFILE})
				if (C)
					list(APPEND ${CPPSOURCES} ${SOURCEFILE})
				endif (C)
			endif (IS_DIRECTORY "${SOURCEFILE}")
		endforeach(SOURCEFILE ${SRC_FLDR})
	endif(NOT ${SRC} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR} AND EXISTS "${SRC}/CMakeLists.txt")
endmacro(GetSources)

function(GetGitRevision)
# Get the git revision location for the branch we're on
    if(EXISTS "${CMAKE_SOURCE_DIR}/.git/HEAD")
            file(READ ${CMAKE_SOURCE_DIR}/.git/HEAD GIT_HEAD_LOC)
            string(LENGTH ${GIT_HEAD_LOC} HEAD_LEN)
            math(EXPR LEN "${HEAD_LEN} - 5")
            string(SUBSTRING ${GIT_HEAD_LOC} 5 ${LEN} GIT_HEAD)
            # Weird nastery to remove newlines which screw up the if statement below.
            set(GIT_SHA_PATH "${CMAKE_SOURCE_DIR}/.git/${GIT_HEAD}")
            string(REGEX REPLACE "(\r?\n)+$" "" GIT_SHA_PATH "${GIT_SHA_PATH}")
    endif(EXISTS "${CMAKE_SOURCE_DIR}/.git/HEAD")

# Get the git revision we're on for the version string
    if(EXISTS "${GIT_SHA_PATH}")
            file(READ "${GIT_SHA_PATH}" VERSION_STR)
            string(REGEX REPLACE "(\r?\n)+$" "" VERSION_STR "${VERSION_STR}")
            # Get the length of the string
            string(LENGTH ${VERSION_STR} VERSION_LEN)
            # Subtract 7 from the string's length
            math(EXPR VERSION_NUM_LEN "${VERSION_LEN} - ${VERSION_LEN} + 7")
            # Extract the value from the string
            string(SUBSTRING ${VERSION_STR} 0 ${VERSION_NUM_LEN} VERSION_GIT)
    endif(EXISTS "${GIT_SHA_PATH}")

    # Set our variables
    set(GIT_REVISION_LONG ${VERSION_STR} PARENT_SCOPE)
    set(GIT_REVISION_SHORT ${VERSION_GIT} PARENT_SCOPE)
endfunction(GetGitRevision)
