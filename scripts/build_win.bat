@echo off
setlocal enabledelayedexpansion

cd %~dp0
cd ..

set ONNXRUNTIME_VERSION=1.9.0

set DEPS_DIR=%CD%\deps

set ONNXRUNTIME_URL=https://github.com/microsoft/onnxruntime/releases/download/v1.9.0/Microsoft.ML.OnnxRuntime.DirectML.%ONNXRUNTIME_VERSION%.zip
set ONNXRUNTIME_ZIP=%DEPS_DIR%\Microsoft.ML.OnnxRuntime.DirectML.%ONNXRUNTIME_VERSION%.zip
set ONNXRUNTIME_DIR=%DEPS_DIR%\onnxruntime

IF not exist deps mkdir deps

pushd %DEPS_DIR%
  IF not exist %ONNXRUNTIME_ZIP% curl -L -o %ONNXRUNTIME_ZIP% %ONNXRUNTIME_URL%
  IF not exist %ONNXRUNTIME_DIR% 7z x -o%ONNXRUNTIME_DIR% %ONNXRUNTIME_ZIP%
popd


call %~dp0\build_obs.bat
IF ERRORLEVEL 1 goto ERR
call %~dp0\build_opencv.bat
IF ERRORLEVEL 1 goto ERR


IF exist build rmdir /s /q build
mkdir build
pushd build
  cmake ^
    -DobsPath=%OBS_SRC_DIR% ^
    -DOnnxRuntimePath=%ONNXRUNTIME_DIR% ^
    -DOpenCV_DIR=%OPENCV_BUILD_DIR% ^
    -DCMAKE_SYSTEM_VERSION=10.0.18363.657 ^
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
