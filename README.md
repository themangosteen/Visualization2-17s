# Overview
This project aims to implement a small framework for rendering of dense line data,
based on [**2009 Everts et al. "Depth-Dependent Halos: Illustrative Rendering of Dense Line Data"**](http://tobias.isenberg.cc/personal/papers/Everts_2009_DDH.pdf) [1].

## Framework
The framework is based on a Qt 5 UI and uses OpenGL 3.3+ for drawing.
The [**source code**](https://github.com/mangostaniko/Visualization2-17s) consists of C++ and GLSL 3.3 shader code.

Please note that for now CMake build files are provided for Linux only. Dependencies are Qt 5 and OpenGL 3.3.

Features:
  * draw entire dense line datasets (nerve fiber tractography, flow visualization, ...) without clutter
  * intuitively visualize depth (line width is depth dependent, halos occlude other lines)
  * emphasize colinear line bundles (lines at the same depth are not affected by halos)
  * filter data using clipping plane
  * concise user interface (it's great, promise!)

## Supported Data File Formats
For now only [**TrackVis .trk**](http://www.trackvis.org/docs/?subsect=fileformat) tractography line data is supported.
Support for .trk reading is enabled by an external library [**libtrkfileio**](https://github.com/lheric/libtrkfileio) by lheric.
An example dataset of the human connectome is included. Moreover, the framework allows to generate random line data for testing.

## Video
Uploaded on Youtube: 
[**SpaghettiVis: An implementation of Everts et al. dense line data visualization**](https://www.youtube.com/watch?v=fPJtKaP3_kU).

## How to run

in directory containing CMakeLists.txt

    mkdir build
    cd build
    cmake ..
    make
    ./vis2

## Thanks to
    * Everts et al. [1] for the great visualization algorithm
    * the organizers of the [**Visualization 2**](https://www.cg.tuwien.ac.at/courses/Visualisierung2/) course at TU Wien
    * the [**RealtimeVis lab**](https://www.cg.tuwien.ac.at/courses/RTVis/) course at TU Wien for the UI layout and the camera class

## Screenshots
![Framework Screenshot 1](../screenshot.png)
![Framework Screenshot 2](../screenshot2.png)

## References
[1] Everts, Maarten H., et al. "Depth-dependent halos: Illustrative rendering of dense line data." IEEE Transactions on Visualization and Computer Graphics 15.6 (2009).
