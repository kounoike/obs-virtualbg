@echo on
setlocal enabledelayedexpansion

cd %~dp0
cd ..
set DEPS_DIR=%CD%\deps
cd %DEPS_DIR%

set OBS_VERSION=28.0.1

set OBS_URL=https://github.com/obsproject/obs-studio/archive/refs/tags/%OBS_VERSION%.zip
set OBS_ZIP=%DEPS_DIR%\obs-studio-%OBS_VERSION%.zip
set OBS_SRC_DIR=%DEPS_DIR%\obs-studio-%OBS_VERSION%
set OBS_INSTALL_DIR=%DEPS_DIR%\obs-studio
set OBS_BUILD_DIR=%OBS_INSTALL_DIR%\build

set OBS_DEPS_URL=https://github.com/obsproject/obs-deps/releases/download/2022-08-02/windows-deps-2022-08-02-x64.zip
set OBS_DEPS_ZIP=%DEPS_DIR%\dependencies2022.zip
set OBS_DEPS_DIR=%DEPS_DIR%\dependencies2022

set QT_VERSION=6.3.1
set QT_URL=https://github.com/obsproject/obs-deps/releases/download/2022-08-02/windows-deps-qt6-2022-08-02-x64.zip
set QT_ZIP=%DEPS_DIR%\windows-deps-qt6-2022-08-02-x64.zip
rem extract to OBS_DEPS_DIR

set CEF_VERSION=5060
set CEF_URL=https://cdn-fastly.obsproject.com/downloads/cef_binary_%CEF_VERSION%_windows_x64.zip
set CEF_ZIP=%DEPS_DIR%\cef_binary_%CEF_VERSION%_windows_x64.zip
set CEF_DIR=%DEPS_DIR%\cef
set CEF_VERSION_DIR=%DEPS_DIR%\cef\cef_binary_%CEF_VERSION%_windows_x64

IF exist %OBS_BUILD_DIR% goto OBS_BUILD_END

IF not exist %DEPS_DIR% mkdir %DEPS_DIR%

IF not exist %OBS_DEPS_ZIP% curl -L -o %OBS_DEPS_ZIP% %OBS_DEPS_URL%
IF not exist %OBS_DEPS_DIR% mkdir %OBS_DEPS_DIR%
IF not exist %OBS_DEPS_DIR%\win64 7z x -o%OBS_DEPS_DIR% %OBS_DEPS_ZIP%

IF not exist %QT_ZIP% curl -L -o %QT_ZIP% %QT_URL%
7z x -o%OBS_DEPS_DIR% %QT_ZIP%

IF not exist %CEF_ZIP% curl -L -o %CEF_ZIP% %CEF_URL%
IF not exist %CEF_DIR% mkdir %CEF_DIR%
IF not exist %CEF_VERSION_DIR% 7z x -o%CEF_DIR% %CEF_ZIP%

IF not exist %OBS_ZIP% curl -L -o %OBS_ZIP% %OBS_URL%
IF not exist %OBS_SRC_DIR% 7z x -y %OBS_ZIP%
IF exist %OBS_INSTALL_DIR% rmdir /s /q %OBS_INSTALL_DIR%
move %OBS_SRC_DIR% %OBS_INSTALL_DIR%

mkdir %OBS_BUILD_DIR%
pushd %OBS_BUILD_DIR%
  cmake ^
    -G"Visual Studio 17 2022" -A"x64" ^
    -DDepsPath=%OBS_DEPS_DIR%\win64 ^
    -DCMAKE_PREFIX_PATH=%OBS_DEPS_DIR% ^
    -DBUILD_BROWSER=OFF ^
    -DBUILD_VST=OFF ^
    -DDISABLE_PLUGINS=ON ^
    -DDISABLE_LUA=ON ^
    -DDISABLE_PYTHON=ON ^
    -DCMAKE_SYSTEM_VERSION=10.0.18363.657 ^
    %OBS_INSTALL_DIR%
  IF ERRORLEVEL 1 GOTO ERR
  cmake --build . --config RelWithDebInfo
  IF ERRORLEVEL 1 GOTO ERR

popd
:OBS_BUILD_END

exit /b 0

:ERR
exit /b 1

endlocal
