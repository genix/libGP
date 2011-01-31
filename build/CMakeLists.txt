#========================
# Build script for libGP 
#========================

cmake_minimum_required(VERSION 2.6)

project(libGP C CXX)

set(LIBGP_VERSION_MAJOR 1)
set(LIBGP_VERSION_MINOR 0)
set(LIBGP_VERSION_PATCH 0)
set(PROJECT_VERSION ${LIBGP_VERSION_MAJOR}.${LIBGP_VERSION_MINOR}.${LIBGP_VERSION_PATCH})

# Search in the 'cmake' directory for additional CMake modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR})

set(CMAKE_DEBUG_POSTFIX  "_d")

#===================================
# Environment tests ================
#===================================

include(TestForANSIForScope)
include(TestForANSIStreamHeaders)
include(TestForSTDNamespace)

#===================================
# Build options ====================
#===================================

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING
        "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
        FORCE)
endif()

option(BUILD_AUXILIARY "Build auxiliary code" ON)
option(BUILD_SAMPLES "Build samples" ON)

#===================================
# Setup paths ======================
#===================================

set(PROJECT_SOURCE_DIR ${PROJECT_SOURCE_DIR}/..)

include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# Include list of source files
include(FileList)

#===================================
# Build libraries ==================
#===================================

list(APPEND SOURCE ${Core_SOURCE_FILES})
list(APPEND HEADERS ${Core_SOURCE_FILES})

if (BUILD_AUXILIARY)
	list(APPEND SOURCE ${Aux_SOURCE_FILES})
	list(APPEND HEADERS ${Aux_HEADER_FILES})
endif()

add_library(GP STATIC ${SOURCE} ${HEADERS})

# On Apple build 64bit and 32bit architectures
if(APPLE)
    	set_target_properties(${NAME} PROPERTIES
    					OSX_ARCHITECTURES "i386;x86_64;"
    	)
endif()

#===================================
# Build samples ====================
#===================================

if(BUILD_SAMPLES)    
	set(SAMPLES Example1 Example2)
    
    set(sample_LIBRARIES GP)
    
    include_directories(
    	${PROJECT_SOURCE_DIR}/auxiliary
	)

    # Build and install the basic samples
    foreach(SAMPLE ${SAMPLES})
        add_executable(${SAMPLE} WIN32 ${${SAMPLE}_SOURCE_FILES} ${${SAMPLE}_HEADER_FILES} )
	    target_link_libraries(${SAMPLE} GP)
    endforeach()
endif()
