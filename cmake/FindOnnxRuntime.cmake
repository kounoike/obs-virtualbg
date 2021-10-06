set(OnnxRuntime_DEPS_DIR ${CMAKE_SOURCE_DIR}/deps/onnxruntime)
if(EXISTS ${OnnxRuntime_DEPS_DIR})
    file(GLOB OnnxRuntime_LIBRARIES_EX ${OnnxRuntime_DEPS_DIR}/lib/lib*.a ${OnnxRuntime_DEPS_DIR}/lib/lib*.so)
    set(OnnxRuntime_LIBRARIES ${OnnxRuntime_LIBRARIES_EX})
    set(OnnxRuntime_INCLUDE_DIR ${OnnxRuntime_DEPS_DIR}/include)
    message("OnnxRuntime_DEPS_DIR Found!!!!!! ${OnnxRuntime_DEPS_DIR} ${OnnxRuntime_LIBRARIES}")
else()
  find_path(OnnxRuntime_INCLUDE_DIR
      NAMES "onnxruntime_c_api.h"
      "onnxruntime/core/session/onnxruntime_c_api.h"
      PATHS
          ENV OnnxRuntimePath
          ${OnnxRuntimePath}
          ${OnnxRuntimePath}/include
          ${OnnxRuntimePath}/build/native/include
      PATH_SUFFIXES include
      DOC "OnnxRuntime Include Directory"
  )

  find_library(OnnxRuntime_LIBRARIES
      NAMES onnxruntime.lib onnxruntime_providers_shared.lib libonnxruntime.dylib
      PATHS
          ENV OnnxRuntimePath
          ${OnnxRuntimePath}
          ${OnnxRuntimePath}/lib
          ${OnnxRuntimePath}/runtimes/win-x64/native
      PATH_SUFFIXES lib
  )
endif()

mark_as_advanced(OnnxRuntime_INCLUDE_DIR OnnxRuntime_LIBRARIES)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OnnxRuntime
  REQUIRED_VARS
    OnnxRuntime_INCLUDE_DIR
    OnnxRuntime_LIBRARIES
  )
