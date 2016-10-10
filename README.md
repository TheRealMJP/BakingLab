# The Baking Lab

This project is a D3D11 demo application that implements various techniques for storing pre-computed lighting in 2D lightmaps, including using a set of Spherical Gaussians to approximate incoming radiance. The demo is a companion to a series of articles on my blog, which you can find here: [https://mynameismjp.wordpress.com/2016/10/09/new-blog-series-lightmap-baking-and-spherical-gaussians/](https://mynameismjp.wordpress.com/2016/10/09/new-blog-series-lightmap-baking-and-spherical-gaussians/). The techniques implemented in the demo were originally presented in our [presentation](http://blog.selfshadow.com/publications/s2015-shading-course/rad/s2015_pbs_rad_slides.pptx) from the [Physically Based Shading course](http://blog.selfshadow.com/publications/s2015-shading-course/) at SIGGRAPH 2015.

# Build Instructions

The repository contains a Visual Studio 2015 project and solution file that's ready to build on Windows. All external dependencies are included in the repository, so there's no need to download additional libraries. Running the demo requires Windows 7 or higher, as well as a GPU that supports Feature Level 11_0.

# Using the Demo App

To move the camera, press the W/S/A/D/Q/E keys. The camera can also be rotated by right-clicking on the window and dragging the mouse. Everything else is controlled through the in-app settings UI. For more information on the features of the demo app, see [this blog post](https://mynameismjp.wordpress.com/2016/10/09/sg-series-part-6-step-into-the-baking-lab/).