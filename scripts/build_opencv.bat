@echo off
setlocal enabledelayedexpansion

cd %~dp0
cd ..
set DEPS_DIR=%CD%\deps
cd %DEPS_DIR%

set OPENCV_VERSION=4.4.0

set OPENCV_URL=https://github.com/opencv/opencv/archive/refs/tags/%OPENCV_VERSION%.zip
set OPENCV_ZIP=%DEPS_DIR%\opencv-%OPENCV_VERSION%.zip
set OPENCV_SRC_DIR=%DEPS_DIR%\opencv-%OPENCV_VERSION%
set OPENCV_BUILD_DIR=%OPENCV_SRC_DIR%\build

IF exist %DEPS_DIR%\opencv goto OPENCV_BUILD_END

IF not exist %OPENCV_ZIP% curl -L -o %OPENCV_ZIP% %OPENCV_URL%
IF not exist %OPENCV_SRC_DIR% 7z x %OPENCV_ZIP%

mkdir %OPENCV_BUILD_DIR%
pushd %OPENCV_BUILD_DIR%
  cmake ^
    -G"Visual Studio 16 2019" -A"x64" ^
    -DBUILD_LIST=core,imgproc ^
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

exit /b 0

:ERR
exit /b 1

endlocal
