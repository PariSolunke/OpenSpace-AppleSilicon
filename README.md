
![OpenSpace Logo](/data/openspace-horiz-logo-crop.png)
[OpenSpace](http://openspaceproject.com) is an open source, non-commercial, and freely available interactive data visualization software designed to visualize the entire known universe and portray our ongoing efforts to investigate the cosmos.  

This is a fork of the feature/applesilicon branch of OpenSpace, with additional modifications to the shaders in the GlobeBrowsing module to ensure compatibility with Mac computers running Apple Silicon. To read more about the project please or see [here](https://github.com/OpenSpace/OpenSpace). This is the same version of OpenSpace (v 0.19.0) as the applesilicon branch, but I imagine it should be possible to integrate the shader chamnges here, and the changes from the applesilicon branch into the current version of the software.

# Compilation Instructions

1) **CMake Best Practices:**
   - Delete Build
   - Clear Cache

2) **Clone this repo** (use the recursive -r flag while cloning)

3) **Install the dependencies listed in the macOS compilation instructions [here](https://docs.openspaceproject.com/en/latest/contribute/development/compiling/macos.html)**  
   Use the standard `brew` commands from the wiki.

4) **Install QT 6 from [here](https://doc.qt.io/qt-6/macos.html](https://doc.qt.io/qt-6/get-and-install-qt.html)** (tested with qt 6.5) 

5) **For Apple Silicon, you may need to update `apache-arrow`**  
   - Check if your GDAL install is not `3.8.3_2`
   - If it isn't, run:  
     ```bash
     brew upgrade apache-arrow
     ```  
     This will update a bunch of dependencies including GDAL from `3.7 -> 3.8`.

6) **CMake as normal**
   - Ensure XCode is named **XCode** (and not XCode Beta or XCode 15, etc.)
   - Ensure that XCode command line tools are installed:  
     ```bash
     xcode-select --install
     ```

7) **CMake Configure**

8) **CMake Generate**

9) **Open XCode Project**
   - Select the `OpenSpace` project file in the file browser on the top left.
   - Select `Build Settings` and change the following settings for **EVERY SINGLE BUILD TARGET** (select all):
     - Change **Architecture** to **Standard Architectures** (this may take some time)
     - Change **Build Active Architecture Only** to **YES** (this may take some time)
   - Select the **Assimp** target:
     - Remove the `-Werror` flag on **Other C++ Flags** and **Other C Flags** (under Apple Clang - Custom Compiler flags) for all configurations (Debug, MinSizeRel, RelWithDebInfo, Release).

10) Ensure **'My Mac'** is selected in the top XCode toolbar.

11) Select the `OpenSpace` target, then click on **Edit Scheme** and set the build configuration to **Release**.

12) **Build**


# Current Status and Issues

## Functionality Overview

### What Works
- **Globebrowsing Module**: This module works in full. The `applesilicon` profile successfully loads key elements such as:
  - The digital universe
  - The Earth and its default layers with fully functional textures
  - The Moon, Sun, and Jupiter
- **Default Profile**: Works well aside from the issues noted with the atmosphere module.

### Partially Working Profiles
- A few other profiles work with minor issues. More details will be elaborated.

## Issues Encountered

### Atmosphere Module
- The shaders for this module do not function as expected. When enabled, intense flickering occurs around the planets, making for a jarring experience.

### GUI (Graphical User Interface)
- **Buggy Behavior**: The GUI sometimes fails to load properly. 
  - A potential workaround is to close the application, clear the cache folder (located in the application root directory), and relaunch. However, this is not a guaranteed fix everytime, and may need multiple relaunches.
  - In the event the main GUI fails to load, you can still access the older GUI by pressing `fn + F1`, which always works.

---
