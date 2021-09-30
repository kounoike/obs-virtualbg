@echo off
setlocal enabledelayedexpansion

cd %~dp0
cd ..

set ONNXRUNTIME_VERSION=1.9.0
set OPENCV_VERSION=4.4.0
set OBS_VERSION=27.0.1

set DEPS_DIR=%CD%\deps

set ONNXRUNTIME_URL=https://github.com/microsoft/onnxruntime/releases/download/v1.9.0/Microsoft.ML.OnnxRuntime.DirectML.%ONNXRUNTIME_VERSION%.zip
set ONNXRUNTIME_ZIP=%DEPS_DIR%\Microsoft.ML.OnnxRuntime.DirectML.%ONNXRUNTIME_VERSION%.zip
set ONNXRUNTIME_DIR=%DEPS_DIR%\onnxruntime

set OPENCV_URL=https://github.com/opencv/opencv/archive/refs/tags/%OPENCV_VERSION%.zip
set OPENCV_ZIP=%DEPS_DIR%\opencv-%OPENCV_VERSION%.zip
set OPENCV_SRC_DIR=%DEPS_DIR%\opencv-%OPENCV_VERSION%
set OPENCV_BUILD_DIR=%OPENCV_SRC_DIR%\build

set OBS_URL=https://github.com/obsproject/obs-studio/archive/refs/tags/%OBS_VERSION%.zip
set OBS_ZIP=%DEPS_DIR%\obs-studio-%OBS_VERSION%.zip
set OBS_SRC_DIR=%DEPS_DIR%\obs-studio-%OBS_VERSION%
set OBS_BUILD_DIR=%OBS_SRC_DIR%\build

set OBS_DEPS_URL=https://obsproject.com/downloads/dependencies2019.zip
set OBS_DEPS_ZIP=%DEPS_DIR%\dependencies2019.zip
set OBS_DEPS_DIR=%DEPS_DIR%\dependencies2019

set QT_VERSION=5.15.2
set QT_URL=https://cdn-fastly.obsproject.com/downloads/Qt_%QT_VERSION%.7z
set QT_7Z=%DEPS_DIR%\Qt_%QT_VERSION%.7z
set QT_DIR=%DEPS_DIR%\qt
set QT_VERSION_DIR=%DEPS_DIR%\qt\%QT_VERSION%


IF not exist deps mkdir deps
pushd deps

  IF not exist %OBS_DEPS_ZIP% curl -L -o %OBS_DEPS_ZIP% %OBS_DEPS_URL%
  IF not exist %OBS_DEPS_DIR% mkdir %OBS_DEPS_DIR%
  IF not exist %OBS_DEPS_DIR%\win64 7z x -o%OBS_DEPS_DIR% %OBS_DEPS_ZIP%

  IF not exist %OBS_ZIP% curl -L -o %OBS_ZIP% %OBS_URL%
  IF not exist %OBS_SRC_DIR% 7z x %OBS_ZIP%

  IF not exist %QT_7Z% curl -L -o %QT_7Z% %QT_URL%
  IF not exist %QT_DIR% mkdir %QT_DIR%
  IF not exist %QT_VERSION_DIR% 7z x -o%QT_DIR% %QT_7Z%

  IF exist %OBS_BUILD_DIR% goto OBS_BUILD_END
  @REM IF exist %OBS_BUILD_DIR% rmdir /s /q %OBS_BUILD_DIR%

  mkdir %OBS_BUILD_DIR%
  pushd %OBS_BUILD_DIR%
    cmake ^
      -G"Visual Studio 16 2019" -A"x64" ^
      -DDepsPath=%OBS_DEPS_DIR%\win64 ^
      -DQTDIR=%QT_VERSION_DIR% ^
      -DQt5Widgets_DIR=%QT_VERSION_DIR%\msvc2019_64\lib\cmake\Qt5Widgets ^
      -DQt5Svg_DIR=%QT_VERSION_DIR%\msvc2019_64\lib\cmake\Qt5Svg ^
      -DQt5Xml_DIR=%QT_VERSION_DIR%\msvc2019_64\lib\cmake\Qt5Xml ^
      -DBUILD_BROWSER=OFF ^
      -DBUILD_VST=OFF ^
      -DDISABLE_PLUGINS=ON ^
      -DDISABLE_LUA=ON ^
      -DDISABLE_PYTHON=ON ^
      -DCMAKE_SYSTEM_VERSION=10.0.18363.657 ^
      ..
    IF ERRORLEVEL 1 GOTO ERR
    cmake --build . --config Release
    IF ERRORLEVEL 1 GOTO ERR
  popd
  :OBS_BUILD_END

  IF not exist %OPENCV_ZIP% curl -L -o %OPENCV_ZIP% %OPENCV_URL%
  IF not exist %OPENCV_SRC_DIR% 7z x %OPENCV_ZIP%

  IF exist %DEPS_DIR%\opencv goto OPENCV_BUILD_END

  mkdir %OPENCV_BUILD_DIR%
  pushd %OPENCV_BUILD_DIR%
    cmake ^
      -G"Visual Studio 16 2019" -A"x64" ^
      -DBUILD_LIST=core,imgproc ^
      -DBUILD_SHARED_LIBS=OFF ^
      ..
    IF ERRORLEVEL 1 GOTO ERR

    cmake --build . --config Release
    IF ERRORLEVEL 1 GOTO ERR
    cmake --install . --prefix %DEPS_DIR%\opencv
    IF ERRORLEVEL 1 GOTO ERR
    cmake --build . --config Debug
    IF ERRORLEVEL 1 GOTO ERR
    cmake --install . --prefix %DEPS_DIR%\opencv
    IF ERRORLEVEL 1 GOTO ERR
  popd
  :OPENCV_BUILD_END

  IF not exist %ONNXRUNTIME_ZIP% curl -L -o %ONNXRUNTIME_ZIP% %ONNXRUNTIME_URL%
  IF not exist %ONNXRUNTIME_DIR% 7z x -o%ONNXRUNTIME_DIR% %ONNXRUNTIME_ZIP%
popd

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
