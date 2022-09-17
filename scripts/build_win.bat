@echo on
setlocal enabledelayedexpansion

cd %~dp0
cd ..
set DEPS_DIR=%CD%\deps

set ONNXRUNTIME_VERSION=1.10.0
set DIRECTML_VERSION=1.8.0
set HALIDE_VERSION=14.0.0

set ONNXRUNTIME_URL=https://github.com/microsoft/onnxruntime/releases/download/v%ONNXRUNTIME_VERSION%/Microsoft.ML.OnnxRuntime.DirectML.%ONNXRUNTIME_VERSION%.zip
set ONNXRUNTIME_ZIP=%DEPS_DIR%\Microsoft.ML.OnnxRuntime.DirectML.%ONNXRUNTIME_VERSION%.zip
set ONNXRUNTIME_DIR=%DEPS_DIR%\onnxruntime

set ONNXRUNTIME_GPU_URL=https://github.com/microsoft/onnxruntime/releases/download/v%ONNXRUNTIME_VERSION%/onnxruntime-win-x64-gpu-%ONNXRUNTIME_VERSION%.zip
set ONNXRUNTIME_GPU_ZIP=%DEPS_DIR%\onnxruntime-win-x64-gpu-%ONNXRUNTIME_VERSION%.zip
set ONNXRUNTIME_GPU_DIR=%DEPS_DIR%\onnxruntime-win-x64-gpu-%ONNXRUNTIME_VERSION%

set DIRECTML_URL=https://www.nuget.org/api/v2/package/Microsoft.AI.DirectML/%DIRECTML_VERSION%
set DIRECTML_ZIP=%DEPS_DIR%\Microsoft.AI.DirectML-%DIRECTML_VERSION%.zip
set DIRECTML_DIR=%DEPS_DIR%\directml

set HALIDE_URL=https://github.com/halide/Halide/releases/download/v14.0.0/Halide-14.0.0-x86-64-windows-6b9ed2afd1d6d0badf04986602c943e287d44e46.zip
set HALIDE_ZIP=Halide-%HALIDE_VERSION%.zip
set HALIDE_DIR=%DEPS_DIR%\Halide-%HALIDE_VERSION%-x86-64-windows


IF not exist deps mkdir deps

pushd %DEPS_DIR%
  IF not exist %ONNXRUNTIME_ZIP% curl -L -o %ONNXRUNTIME_ZIP% %ONNXRUNTIME_URL%
  IF not exist %ONNXRUNTIME_DIR% 7z x -o%ONNXRUNTIME_DIR% %ONNXRUNTIME_ZIP%

  IF not exist %ONNXRUNTIME_GPU_ZIP% curl -L -o %ONNXRUNTIME_GPU_ZIP% %ONNXRUNTIME_GPU_URL%
  IF not exist %ONNXRUNTIME_GPU_DIR% 7z x -o%DEPS_DIR% %ONNXRUNTIME_GPU_ZIP%

  IF not exist %DIRECTML_ZIP% curl -L -o %DIRECTML_ZIP% %DIRECTML_URL%
  IF not exist %DIRECTML_DIR% 7z x -o%DIRECTML_DIR% %DIRECTML_ZIP%

  IF not exist %HALIDE_ZIP% curl -L -o %HALIDE_ZIP% %HALIDE_URL%
  IF not exist %HALIDE_DIR% 7z x %HALIDE_ZIP%
popd


call %~dp0\build_obs.bat
IF ERRORLEVEL 1 goto ERR

IF exist build rmdir /s /q build
mkdir build
pushd build
  cmake ^
    -DobsPath=%DEPS_DIR%\obs-studio ^
    -DOnnxRuntimePath=%ONNXRUNTIME_DIR% ^
    -DCMAKE_SYSTEM_VERSION=10.0.18363.657 ^
    -DHalide_DIR=%HALIDE_DIR%\lib\cmake\Halide ^
    -DHalideHelpers_DIR=%HALIDE_DIR%\lib\cmake\HalideHelpers ^
    ..
  IF ERRORLEVEL 1 GOTO ERR
  cmake --build . --config RelWithDebInfo
  IF ERRORLEVEL 1 GOTO ERR
  cpack
  IF ERRORLEVEL 1 GOTO ERR
popd

IF exist build_gpu rmdir /s /q build_gpu
mkdir build_gpu
pushd build_gpu
  cmake ^
    -DOBS_VIRTUALBG_USE_CUDA=ON ^
    -DobsPath=%DEPS_DIR%\obs-studio ^
    -DOnnxRuntimePath=%ONNXRUNTIME_GPU_DIR% ^
    -DCMAKE_SYSTEM_VERSION=10.0.18363.657 ^
    -DHalide_DIR=%HALIDE_DIR%\lib\cmake\Halide ^
    -DHalideHelpers_DIR=%HALIDE_DIR%\lib\cmake\HalideHelpers ^
    ..
  IF ERRORLEVEL 1 GOTO ERR
  cmake --build . --config RelWithDebInfo
  IF ERRORLEVEL 1 GOTO ERR
  cpack
  IF ERRORLEVEL 1 GOTO ERR
popd

goto END
:ERR
echo BUILD ERROR %ERRORLEVEL%
exit /b 1


:END
endlocal
