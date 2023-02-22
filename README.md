# OBS Virtual background plugin

English / [Japanese](./README_ja.md)

**OBS Virtual background plugin** is a plugin for OBS. This plugin allows you to crop the background area without a green screen like Zoom and Meet virtual backgrounds.

## Supported Operating System

- Windows 11 64bit
- latest Mac OS, Intel architecture
- Ubuntu-20.04 on x86_64 architecutre

## Install


### Windows

Download the latest version of obs-virtualbg-vX.X.X-win64.zip from Releases on the right side of the [GitHub Page](https://github.com/kounoike/obs-virtualbg).

Extract the zip and copy the `data` and `obs-plugins` folders to your OBS Studio Folder (`C:\Program Files\obs-studio` by default).


Note: If you have the **OBS Background removal plugin** installed, uninstall it first, because the ONNXRuntime DLL will conflict.


### Mac OS

Download the latest version of obs-virtualbg-vX.X.X-Darwin.zip from [norihiro's fork repository](https://github.com/norihiro/obs-virtualbg/releases).

Extract the zip and copy to `~/Library/Application Support/obs-studio/plugins/obs-virtualbg` directry.

### Linux

Download the latest version of obs-virtualbg-X.X.X-Linux.deb. And run `sudo dpkg -i obs-virtualbg-X.X.X-Linux.deb`


## Usage

### filter settings

Add **Media source** or **Video capture device** to scene, and open Filters.

![](doc/scene_en.png)

![](doc/filter_en.png)


Add **Virtual Background Detector** to Audio/Video Filters. and also add **Virtual Background Renderer** to Effect Filters. **Both** filters required.

![](doc/filter_2_en.png)

Since v1.2.0, default inference engine is changed. Now, default inference engine is CPU. But, it is low accuracy, heavy to compute. If you can use GPU, changing to GPU is better. But some case, GPU engine is crashed at initialize phase.

### Background settings

Place a background image, window capture, or game capture source below the filtered source. Set the size, crop, etc. to complete the settings.

![](doc/scene_2_en.png)

## Caution

Since the human region is detected by inference using a deep learning model, detection error inevitably occur. In some cases, the person's area is not detected and is transparent, and in other cases, the background area is mis-detected and some of the room is shown. In particular, it is not good at recognizing hands, so it may not be suitable for performance. This is a limitation of the detection model and is not something that can be adjusted by adjusting parameters. If you need accurate cropping, please use the green background and chroma key filters.


