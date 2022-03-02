@echo off
setlocal enabledelayedexpansion

cd %~dp0
cd ..
set DEPS_DIR=%CD%\deps
cd %DEPS_DIR%

set OBS_VERSION=27.2.2

set OBS_URL=https://github.com/obsproject/obs-studio/archive/refs/tags/%OBS_VERSION%.zip
set OBS_ZIP=%DEPS_DIR%\obs-studio-%OBS_VERSION%.zip
set OBS_SRC_DIR=%DEPS_DIR%\obs-studio-%OBS_VERSION%
set OBS_BUILD_DIR=%OBS_SRC_DIR%\build
set OBS_INSTALL_DIR=%DEPS_DIR%\obs-studio

set OBS_DEPS_URL=https://obsproject.com/downloads/dependencies2019.zip
set OBS_DEPS_ZIP=%DEPS_DIR%\dependencies2019.zip
set OBS_DEPS_DIR=%DEPS_DIR%\dependencies2019

set QT_VERSION=5.15.2
set QT_URL=https://cdn-fastly.obsproject.com/downloads/Qt_%QT_VERSION%.7z
set QT_7Z=%DEPS_DIR%\Qt_%QT_VERSION%.7z
set QT_DIR=%DEPS_DIR%\qt
set QT_VERSION_DIR=%DEPS_DIR%\qt\%QT_VERSION%

set CEF_VERSION=4638
set CEF_URL=https://cdn-fastly.obsproject.com/downloads/cef_binary_%CEF_VERSION%_windows_x64.zip
set CEF_ZIP=%DEPS_DIR%\cef_binary_%CEF_VERSION%_windows_x64.zip
set CEF_DIR=%DEPS_DIR%\cef
set CEF_VERSION_DIR=%DEPS_DIR%\cef\cef_binary_%CEF_VERSION%_windows_x64

IF exist %OBS_INSTALL_DIR% goto OBS_BUILD_END

IF not exist %DEPS_DIR% mkdir %DEPS_DIR%

IF not exist %OBS_DEPS_ZIP% curl -L -o %OBS_DEPS_ZIP% %OBS_DEPS_URL%
IF not exist %OBS_DEPS_DIR% mkdir %OBS_DEPS_DIR%
IF not exist %OBS_DEPS_DIR%\win64 7z x -o%OBS_DEPS_DIR% %OBS_DEPS_ZIP%

IF not exist %QT_7Z% curl -L -o %QT_7Z% %QT_URL%
IF not exist %QT_DIR% mkdir %QT_DIR%
IF not exist %QT_VERSION_DIR% 7z x -o%QT_DIR% %QT_7Z%

IF not exist %CEF_ZIP% curl -L -o %CEF_ZIP% %CEF_URL%
IF not exist %CEF_DIR% mkdir %CEF_DIR%
IF not exist %CEF_VERSION_DIR% 7z x -o%CEF_DIR% %CEF_ZIP%

IF not exist %OBS_ZIP% curl -L -o %OBS_ZIP% %OBS_URL%
IF not exist %OBS_SRC_DIR% 7z x -y %OBS_ZIP%

mkdir %OBS_BUILD_DIR%
pushd %OBS_BUILD_DIR%
  cmake ^
    -G"Visual Studio 16 2019" -A"x64" ^
    -DDepsPath=%OBS_DEPS_DIR%\win64 ^
    -DQTDIR=%QT_VERSION_DIR% ^
    -DQt5Widgets_DIR=%QT_VERSION_DIR%\msvc2019_64\lib\cmake\Qt5Widgets ^
    -DQt5Svg_DIR=%QT_VERSION_DIR%\msvc2019_64\lib\cmake\Qt5Svg ^
    -DQt5Xml_DIR=%QT_VERSION_DIR%\msvc2019_64\lib\cmake\Qt5Xml ^
    -DQt5Network_DIR=%QT_VERSION_DIR%\msvc2019_64\lib\cmake\Qt5Network ^
    -DQt5WinExtras_DIR=%QT_VERSION_DIR%\msvc2019_64\lib\cmake\Qt5WinExtras ^
    -DBUILD_BROWSER=OFF ^
    -DBUILD_VST=OFF ^
    -DDISABLE_PLUGINS=ON ^
    -DDISABLE_LUA=ON ^
    -DDISABLE_PYTHON=ON ^
    -DCMAKE_SYSTEM_VERSION=10.0.18363.657 ^
    %OBS_SRC_DIR%
  IF ERRORLEVEL 1 GOTO ERR
  cmake --build . --config Release
  IF ERRORLEVEL 1 GOTO ERR

popd
move %OBS_SRC_DIR% %OBS_INSTALL_DIR%
:OBS_BUILD_END

exit /b 0

:ERR
exit /b 1

endlocal
