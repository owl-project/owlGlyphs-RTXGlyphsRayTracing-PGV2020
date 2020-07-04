# owlGlyphs: Sample Code for the "Ray Tracing Glyphs with RTX" Project

## Context/Reference

This sample code is in regards to a project that used OWL to evaluate
the feasibility and performance of RTX-Accelerated ray tracing for
Glyph-based data visualization.

Citation for the corresponding paper: 

[*High-Quality Rendering of
Glyphs Using Hardware-AcceleratedRay Tracing*, Stefan Zellmann, Martin
Aumueller, Nathan Marshak, and Ingo Wald, _Proceedings of Eurographics
Parallel Graphics and Visualization (Short Papers)_, 2020.](https://www.researchgate.net/publication/340793638_High-Quality_Rendering_of_Glyphs_Using_Hardware-Accelerated_Ray_Tracing)

## Requirements/Dependencies

This project uses OWL, and should be checked out with owl as a
submodule. To do so, use the `--recursive` flag when cloning this
repository; alternatively, if you forgot this you can also pull the
submodules via 

```
	git submodule init
	git submodule update
```
which should also end up fetching the correct version of OWL.

To build, you also need the typical OWL dependencies, ie, a C++ 11
compatible compiler, CUDA, and OptiX (version 7 or newer). Both
windows and linux should be supported.

## Building

Using cmake, this project should build on both linux and windows.  If
you're not familiar with cmake, it is suggested to follow OWL's
instructions to first build the samples that come with OWL, then
repeat the same steps here.

## Guide to the Source Code

The following files/classes implement the glyph types we presented in
the paper. `FileName.h` and `FileName.cpp` contain the OWL host code
and `device/FileName.cu` contain the respective OptiX 7 device programs.

### Arrow glyphs

This is an example of a non-affine glyph; i.e. the glyph
cannot just be transformed with the instance transform,
so the geometry is stored in extra buffers.

[ArrowGlyphs.h](/glyphs/ArrowGlyphs.h)
[ArrowGlyphs.cpp](/glyphs/ArrowGlyphs.cpp)
[device/ArrowGlyphs.cu](/glyphs/device/ArrowGlyphs.cu)

### Simple sphere glyphs

Sphere glyphs are "affine glyphs", i.e. under transformation
they can become general ellipsoids and that behavior is desired

[SphereGlyphs.h](/glyphs/SphereGlyphs.h)
[SphereGlyphs.cpp](/glyphs/SphereGlyphs.cpp)
[device/SphereGlyphs.cu](/glyphs/device/SphereGlyphs.cu)

### Motion blur glyphs

This is a sphere glyph, but the geometry of the glyph is
non-affine, and instead it is spread out and blurred over the
extent defined by its instance bounds according to the
link velocity and link acceleration

[MotionSpheres.h](/glyphs/MotionSpheres.h)
[MotionSpheres.cpp](/glyphs/MotionSpheres.cpp)
[device/MotionSpheres.cu](/glyphs/device/MotionSpheres.cu)

### Super quadric glyphs

This is a more involved glyph that uses a Newton/Rhaphson
solver to compute ray/superquadric intersections.
The glyph types implements a trick, as it doesn't use the bounding
box or bounding sphere as an initial root estimate, but rather a
coarse tessellation. The intersection with that tessellation is
computed using hardware-accelerated ray/triangle intersections.

[SuperGlyphs.h](/glyphs/SuperGlyphs.h)
[SuperGlyphs.cpp](/glyphs/SuperGlyphs.cpp)
[device/SuperGlyphs.cu](/glyphs/device/SuperGlyphs.cu)

## Viewer Controls

After building is complete, you should end up with an executable
`./owlGlyphs` or `owlGlyphs.exe`.

Using the [test data set](/testdata.glyphs) from the root dir of this repo just start with
`./owlGlyphs testdata.glyphs`.

<table><tr>
<td><b>Arrow glyphs:</b><br>cmdline: --arrows | -arr<br><img src="/res/arrows_screenshot.png" width="270" /></td>
<td><b>Sphere glyphs:</b><br>cmdline: --spheres | -sph<br><img src="/res/spheres_screenshot.png" width="270" /></td>
</tr><tr>
<td><b>Motion blur glyphs:</b><br>cmdline: --motionblur | -mb<br><img src="/res/motionblur_screenshot.png" width="270" /></td>
<td><b>Super quadric glyphs:</b><br>cmdline: --super | -spr<br><img src="/res/super_screenshot.png" width="270" /></td>
</tr></table>

Keystrokes:

- **!** : dump screenshot
- **H** : enable/disable heatmap
- **<**/**>** : change heatmap scale up/down
- **C** : dump current camera
- **+**/**-** : change mouse motion speed
- **I**: enter 'inspect' mode
- **F**: enter 'fly' mode

Mouse:
- left button: rotate
- middle button: strafe
- right button: forward/backward

## Third-Party Code

The `owlGlyphs` program can optionally load triangle models in wavefront obj
format (cmdline: `-obj <file.obj>`). For that we use
[tinyobjloader](https://github.com/tinyobjloader/tinyobjloader) which is
included as source code. Tinyobjloader's licence is
[MIT](https://opensource.org/licenses/MIT)

We use code for ray/quadric intersection testing by Inigo Quilez. Those tests
can be found on shadertoy: [ray/cylinder test](https://www.shadertoy.com/view/4lcSRn),
[ray/rounded cone test](https://www.shadertoy.com/view/MlKfzm). License is
[MIT](https://opensource.org/licenses/MIT)
