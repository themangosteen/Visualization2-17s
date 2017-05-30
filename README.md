# Overview
This project aims to implement a small framework for rendering of dense line data,
based on [**2009 Everts et al. "Depth-Dependent Halos: Illustrative Rendering of Dense Line Data"**](http://tobias.isenberg.cc/personal/papers/Everts_2009_DDH.pdf).

## Framework
The framework is based on a Qt 5 UI and uses OpenGL 3.3+ for drawing.
The [**source code**](https://github.com/mangostaniko/Visualization2-17s) consists of C++ and GLSL 3.3 shader code.

Please note that for now CMake build files are provided for Linux only. Dependencies are Qt 5 and OpenGL 3.3.

Features:
  * draw entire dense line datasets (nerve fiber tractography, flow visualization, ...) without clutter
  * intuitively visualize depth
  * emphasize colinear line bundles
  * filter data using clipping plane
  * concise user interface

## Supported Data File Formats
For now only [**TrackVis .trk**](http://www.trackvis.org/docs/?subsect=fileformat) tractography line data is supported.
Support for .trk reading is enabled by an external library [**libtrkfileio**](https://github.com/lheric/libtrkfileio) by lheric.
An example dataset of the human connectome is included. Moreover, the framework allows to generate random line data for testing.

## Video
TODO

note: for video show comparison of bundling = 0 and bundling non zero for connectome!!!

## Screenshots
![Framework Screenshot](../../screenshot.png)
