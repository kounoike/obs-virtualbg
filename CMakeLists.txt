cmake_minimum_required(VERSION 3.18)

include(cmake/MunkeiVersionFromGit.cmake)
version_from_git()

project(obs-virtualbg VERSION ${VERSION})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
find_package(Threads REQUIRED)
find_package(LibObs REQUIRED)
find_package(OnnxRuntime REQUIRED)

#find_package(HalideHelpers REQUIRED)
find_package(Halide REQUIRED COMPONENTS shared)
# find_package(Halide REQUIRED)

set(RELEASE_DIR "${PROJECT_SOURCE_DIR}/Release" CACHE STRING "Directory for building release packages")

set(SOURCES
    src/plugin.cpp
    src/obs-virtualbg-detector.cpp
    src/obs-virtualbg-render.cpp
)

add_executable(blur_generator src/blur_gen.cpp)
target_link_libraries(blur_generator PRIVATE Halide::Generator)
add_halide_library(blur FROM blur_generator GENERATOR blur_generator)

add_library(${CMAKE_PROJECT_NAME} MODULE
    ${SOURCES})

include_directories(
    ${LIBOBS_INCLUDE_DIR}
    ${LIBOBS_INCLUDE_DIRS}
    ${OnnxRuntime_INCLUDE_DIR}
)
    
target_link_libraries(${CMAKE_PROJECT_NAME}
	${LIBOBS_LIBRARIES}
  ${OnnxRuntime_LIBRARIES}
  blur
  Halide::RunGenMain
)


if(WIN32)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_NAME "64bit")
    set(OBS_BUILDDIR_ARCH "build64")
  else()
    set(ARCH_NAME "32bit")
    set(OBS_BUILDDIR_ARCH "build32")
  endif()

  include_directories(
    "${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/UI"
  )

  target_link_libraries(${CMAKE_PROJECT_NAME}
    "${OBS_FRONTEND_LIB}"
  )

# --- Release package helper ---
	# The "release" folder has a structure similar OBS' one on Windows
	set(RELEASE_DIR "${PROJECT_SOURCE_DIR}/release")

    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
        # If config is Release or RelWithDebInfo, package release files
        COMMAND if $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>==1 (
            "${CMAKE_COMMAND}" -E make_directory
                "${RELEASE_DIR}/data/obs-plugins/${CMAKE_PROJECT_NAME}"
                "${RELEASE_DIR}/obs-plugins/${ARCH_NAME}"
        )

        COMMAND if $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>==1 (
            "${CMAKE_COMMAND}" -E copy_directory
                "${PROJECT_SOURCE_DIR}/data"
                "${RELEASE_DIR}/data/obs-plugins/${CMAKE_PROJECT_NAME}"
        )

        COMMAND if $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>==1 (
            "${CMAKE_COMMAND}" -E copy
                "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
                "${RELEASE_DIR}/obs-plugins/${ARCH_NAME}"
        )

        # If config is RelWithDebInfo, copy the pdb file
        COMMAND if $<CONFIG:RelWithDebInfo>==1 (
            "${CMAKE_COMMAND}" -E copy
                "$<TARGET_PDB_FILE:${CMAKE_PROJECT_NAME}>"
                "${RELEASE_DIR}/obs-plugins/${ARCH_NAME}"
        )

        # Copy to obs-studio dev environment for immediate testing
        COMMAND if $<CONFIG:Debug>==1 (
            "${CMAKE_COMMAND}" -E copy
                "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
                "${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/obs-plugins/${ARCH_NAME}"
        )

        COMMAND if $<CONFIG:Debug>==1 (
            "${CMAKE_COMMAND}" -E copy
                "$<TARGET_PDB_FILE:${CMAKE_PROJECT_NAME}>"
                "${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/obs-plugins/${ARCH_NAME}"
        )

        COMMAND if $<CONFIG:Debug>==1 (
            "${CMAKE_COMMAND}" -E make_directory
                "${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/data/obs-plugins/${CMAKE_PROJECT_NAME}"
        )

        COMMAND if $<CONFIG:Debug>==1 (
            "${CMAKE_COMMAND}" -E copy_directory
                "${PROJECT_SOURCE_DIR}/data"
                "${LIBOBS_INCLUDE_DIR}/../${OBS_BUILDDIR_ARCH}/rundir/$<CONFIG>/data/obs-plugins/${CMAKE_PROJECT_NAME}"
        )
    )
  install(DIRECTORY "${RELEASE_DIR}/" DESTINATION .)
  set(CPACK_GENERATOR "ZIP")
  set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
  set(CPACK_VERBATIM_VARIABLES YES)
  set(CPACK_PACKAGE_VERSION v${SEMVER})
  include(CPack)
endif()


include(ObsHelpers)
#include(ObsCpack)
#install_obs_plugin_with_data(obs-virtualbg data)
