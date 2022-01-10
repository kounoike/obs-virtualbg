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
add_halide_library(blur FROM blur_generator GENERATOR blur_generator TARGETS cmake AUTOSCHEDULER Halide::Adams2019 SCHEDULE blur.schedule STMT_HTML blur_schedule.html)

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
  MESSAGE("*******************ONNXRUNTIME: ${OnnxRuntimePath}/runtimes/win-x64/native/onnxruntime.dll **********")


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

        # package the onnxruntime DLL with the plugin DLL
        COMMAND if $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>==1 (
            "${CMAKE_COMMAND}" -E copy
                "${OnnxRuntimePath}/runtimes/win-x64/native/onnxruntime.dll"
                "${RELEASE_DIR}/obs-plugins/${ARCH_NAME}"
        )

        # package the DirectML runtime DLL
        COMMAND if $<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>==1 (
            "${CMAKE_COMMAND}" -E copy
                "${PROJECT_SOURCE_DIR}/deps/directml/bin/x64-win/DirectML.dll"
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
elseif(APPLE)
  # set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13)
  set(CMAKE_OSX_DEPLOYMENT_TARGET 11.0)
  # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libstdc++ -fvisibility=default -framework Foundation")
	set(RELEASE_DIR "${PROJECT_SOURCE_DIR}/release")

  set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -fvisibility=default")
  set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES LINK_FLAGS "${LD_FLAGS} -stdlib=libc++ -fvisibility=default -framework Foundation")
  set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES PREFIX "")
  target_link_libraries(${CMAKE_PROJECT_NAME} "${OBS_FRONTEND_LIB}")

  add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND
      "${CMAKE_COMMAND}" -E rm -rf "${RELEASE_DIR}"
    COMMAND
      "${CMAKE_COMMAND}" -E make_directory
        "${RELEASE_DIR}/${CMAKE_PROJECT_NAME}/data"
        "${RELEASE_DIR}/${CMAKE_PROJECT_NAME}/bin"
    COMMAND
      "${CMAKE_COMMAND}" -E copy_directory
        "${PROJECT_SOURCE_DIR}/data"
        "${RELEASE_DIR}/${CMAKE_PROJECT_NAME}/data"
    COMMAND
      "${CMAKE_COMMAND}" -E copy
        "$<TARGET_FILE:${CMAKE_PROJECT_NAME}>"
        "${RELEASE_DIR}/${CMAKE_PROJECT_NAME}/bin"
    COMMAND
      "${CMAKE_COMMAND}" -E rm -f
        "${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}-v${SEMVER}-macosx.zip"
    )

  install(DIRECTORY "${RELEASE_DIR}/" DESTINATION .)

  set(CPACK_GENERATOR "ZIP")
  set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
  set(CPACK_VERBATIM_VARIABLES YES)
  set(CPACK_PACKAGE_VERSION v${SEMVER})
  include(CPack)
elseif(UNIX AND NOT APPLE)
  include(GNUInstallDirs)
  set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES PREFIX "")
  target_link_libraries(${CMAKE_PROJECT_NAME} "${OBS_FRONTEND_LIB}")

  file(GLOB locales data/locale/*.ini)
  file(GLOB models data/*.onnx)
  file(GLOB effects data/*.effect)
  file(GLOB OnnxRuntime_INSTALL ${OnnxRuntime_DEPS_DIR}/lib/lib*.so*)

  if("${OBS_PLUGIN_DESTINATION}" STREQUAL "")
    set(OBS_PLUGIN_DESTINATION ${CMAKE_INSTALL_FULL_LIBDIR}/obs-plugins)
  endif()

  install(TARGETS ${CMAKE_PROJECT_NAME} LIBRARY DESTINATION ${OBS_PLUGIN_DESTINATION}/)
  install(FILES ${locales} DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/obs/obs-plugins/${CMAKE_PROJECT_NAME}/locale)
  install(FILES ${models} DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/obs/obs-plugins/${CMAKE_PROJECT_NAME})
  install(FILES ${effects} DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/obs/obs-plugins/${CMAKE_PROJECT_NAME})
  install(FILES ${OnnxRuntime_INSTALL} DESTINATION ${OBS_PLUGIN_DESTINATION}/../obs-virtualbg)
endif()

#include(ObsHelpers)
#include(ObsCpack)
#install_obs_plugin_with_data(obs-virtualbg data)
