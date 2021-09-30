find_path(OnnxRuntime_INCLUDE_DIR
    NAMES onnxruntime_c_api.h
    PATHS
        ENV OnnxRuntimePath
        ${OnnxRuntimePath}
        ${OnnxRuntimePath}/include
        ${OnnxRuntimePath}/build/native/include
    PATH_SUFFIXES include
    DOC "OnnxRuntime Include Directory"
)

find_library(OnnxRuntime_LIBRARIES
    NAMES onnxruntime.lib onnxruntime_providers_shared.lib
    PATHS
        ENV OnnxRuntimePath
        ${OnnxRuntimePath}
        ${OnnxRuntimePath}/lib
        ${OnnxRuntimePath}/runtimes/win-x64/native
    PATH_SUFFIXES lib
)

mark_as_advanced(OnnxRuntime_INCLUDE_DIR OnnxRuntime_LIBRARIES)
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OnnxRuntime
  REQUIRED_VARS
    OnnxRuntime_INCLUDE_DIR
    OnnxRuntime_LIBRARIES
  )
