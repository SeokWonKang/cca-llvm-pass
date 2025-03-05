cmake_minimum_required( VERSION 3.15 )

# Project CCA-LLVM-PASS
project( "CCA-LLVM-PASS" LANGUAGES C CXX VERSION 0.1 )

# Dependencies

# Dependency: LLVM (recommended llvm-12.0.0)
find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})

# CMake Options
set( CMAKE_BUILD_TYPE Debug )
set( CMAKE_VERBOSE_MAKEFILE false )
# set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BUILD_TYPE}/bin )
# set( CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BUILD_TYPE}/lib )
# set( CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BUILD_TYPE}/lib )
set( EXPORT_NAME ${PROJECT_NAME}Targets )
set( CMAKE_EXPORT_COMPILE_COMMANDS on ) 

# Compile Options
set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
add_compile_options(-W -Wall -fPIC -fno-rtti -Wno-unused-parameter)

# Passes
add_subdirectory(Instrumentation)

#------------------------------
# Packages
#------------------------------
# Export
install( 
    EXPORT ${EXPORT_NAME}
    FILE ${EXPORT_NAME}.cmake
	NAMESPACE CCALLVMPass::
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/${PROJECT_NAME} )

# Include Cmake Package Config Helper
include( CMakePackageConfigHelpers )

# Config File
configure_package_config_file(
    ${CMAKE_SOURCE_DIR}/Config.cmake.in
    ${CMAKE_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${CMAKE_CURRENT_BINARY_DIR} )

# Version File
write_basic_package_version_file( 
    ${CMAKE_BINARY_DIR}/cmake/${PROJECT_NAME}ConfigVersion.cmake
    VERSION		    ${PROJECT_VERSION}
    COMPATIBILITY	SameMajorVersion )

# Install Version & Config Files
install( FILES 
    ${CMAKE_BINARY_DIR}/cmake/${PROJECT_NAME}Config.cmake
    ${CMAKE_BINARY_DIR}/cmake/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/cmake/${PROJECT_NAME} )
