# SVG Renderer

## Cloning
```
git clone https://github.com/martbelko/SvgRenderer --recursive
```
It is possible to use ```git submodule update --init``` to fetch the submodules after cloning without --recursive flag.

## Building
Use CMake to build the project, either by using Visual Studio, or CMake command line. Switch to Release x64 build SvgRenderer target.

## Running
The application asks for user input (1, 2 or 3) to select which SVG file to render. Currently, there are 3 SVG example files from which the user can choose.

## Notes
It is possible to change the window size and other parameters in the Defs.h file. Also, in the Application.cpp file, it is possible to specify custom SVG filepath.
Many features from SVG standards are missing. This needs to be taken into account when providing custom SVG files. 
