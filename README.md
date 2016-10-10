# The Baking Lab

This project is a D3D11 demo application that implements various techniques for storing pre-computed lighting in 2D lightmaps, including using a set of Spherical Gaussians to approximate incoming radiance.

# Build Instructions

The repository contains a Visual Studio 2015 project and solution file that's ready to build on Windows. All external dependencies are included in the repository, so there's no need to download additional libraries. Running the demo requires Windows 7 or higher, as well as a GPU that supports Feature Level 11_0.

# Using the Demo App

To move the camera, press the W/S/A/D/Q/E keys. The camera can also be rotated by right-clicking on the window and dragging the mouse. Everything else is controlled through the in-app settings UI.