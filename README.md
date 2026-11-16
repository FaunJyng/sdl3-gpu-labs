# SDL3 GPU API Labs

This repository contains hands-on labs focused on SDL3, specifically its GPU API.

> [!NOTE]
> Not all labs have been completed yet. I aim to finish all of them according to the original plan.

All labs are located in the `📁 labs` directory and organized into numbered subdirectories (`📁 01`, `📁 02`, etc.). The labs are arranged in ascending order of difficulty, with each subsequent group introducing more advanced concepts and challenges. The `📁 deps` includes all dependecies (SDL3, SDL_iamge, etc.) for all labs. 

A description and status of each lab is provided below.

| Source Folder | Name | Requirements | Status |
|:------:|:---|:---|:---:|
| `📁 01` | Clear Screen | Clear the entire screen to a single color. | 🟢 |
| `📁 01` | Animated Background | Continuously change the background color over time. | 🟢 |
| `📁 01` | First Triangle | Render a triangle using a Vertex Shader and a Fragment Shader. | 🟢 |
| `📁 01` | Triangle Color | Assign different colors to each vertex and observe color interpolation. | 🟢 |
| `📁 01` | Gradient Background | Create a background with a smoothly changing RGB color gradient. | 🟢 |
| `📁 02` | Vertex Buffer Triangle | Store three vertices in an `SDL_GPUBuffer` and use it to render a triangle. | 🟢 |
| `📁 02` | Movable Triangle | Use ImGui to interactively reposition the triangle's three vertices. | 🟢 |
| `📁 02` | Multiple Vertex Buffers | Store vertex positions and colors in two separate vertex buffers. | 🔴 |
| `📁 02` | Triangle with Index Buffer | Use an index buffer to render a triangle. | 🔴 |
| `📁 02` | Indexed Quad | Render a quad using four vertices and six indices. | 🔴 |
| `📁 02` | Primitive Topology | Experiment with the primitive topologies supported by SDL3 and compare how they construct geometry. | 🔴 |
| `📁 02` | Wireframe Geometry | Render geometric edges using line primitives instead of filled triangles. | 🔴 |
| `📁 03` | Push Constant Color | Pass color data from C++ to the Fragment Shader using push constants. | 🔴 |
| `📁 03` | Push Constant Position | Pass position coordinates to control the triangle's position. | 🔴 |
| `📁 03` | Time Uniform | Pass time information from the CPU to the shader to create animations. | 🔴 |
| `📁 03` | Scale Transformation | Scale a triangle up and down. | 🔴 |
| `📁 03` | Rotation Transformation | Rotate a triangle around its center. | 🔴 |
| `📁 03` | Translation Transformation | Move geometry using translation transformations. | 🔴 |
| `📁 03` | Combined Transformations | Combine translation, rotation, and scaling transformations. | 🔴 |
| `📁 03` | Multiple Objects | Render multiple objects, each with its own transformation. | 🔴 |
| `📁 03` | Per-object Color | Pass different colors to individual objects using push constants. | 🔴 |
| `📁 03` | Transform Matrix | Use 3×3 or 4×4 matrices to transform vertices in HLSL. | 🔴 |
| `📁 04` | Load Image Texture | Load an image and upload it as a GPU texture. | 🔴 |
| `📁 04` | Texture Mapping | Map a texture onto a quad using UV coordinates. | 🔴 |
| `📁 04` | UV Visualization | Visualize UV coordinates as colors to understand texture mapping. | 🔴 |
| `📁 04` | Texture Tiling | Repeat a texture across a surface by modifying UV coordinates. | 🔴 |
| `📁 04` | Texture Offset | Move a texture across a surface without moving the underlying geometry. | 🔴 |
| `📁 04` | Nearest Sampling | Use nearest-neighbor filtering and observe the resulting pixelation. | 🔴 |
| `📁 04` | Linear Sampling | Use linear filtering and compare its results with nearest-neighbor filtering. | 🔴 |
| `📁 04` | Address Modes | Experiment with clamp, repeat, mirror, and other supported texture address modes. | 🔴 |
| `📁 04` | Multiple Textures | Use two textures within the same Fragment Shader. | 🔴 |
| `📁 04` | Texture Blending | Blend two textures based on a blending factor. | 🔴 |
| `📁 05` | Offscreen Rendering | Render a triangle into an `SDL_GPUTexture` instead of the swapchain. | 🔴 |
| `📁 05` | Display Render Target | Display the rendered texture on the screen using a fullscreen quad. | 🔴 |
| `📁 05` | Render Target Resolution | Render to a texture with a lower resolution than the window, then upscale it to the screen. | 🔴 |
| `📁 05` | Multiple Render Targets | Create two render targets and render the same scene into both textures. | 🔴 |
| `📁 05` | Depth Buffer Creation | Create a depth texture and attach it to a graphics pipeline. | 🔴 |
| `📁 05` | Depth Testing | Render two overlapping triangles and enable depth testing to handle near and far objects correctly. | 🔴 |
| `📁 05` | Depth Visualization | Visualize depth values as grayscale intensities. | 🔴 |
| `📁 05` | 3D Cube | Construct a cube from triangles and render it using perspective projection. | 🔴 |
| `📁 05` | Camera Movement | Move a camera around a 3D cube. | 🔴 |
| `📁 05` | Multiple 3D Objects | Render multiple cubes with different positions and depths. | 🔴 |
| `📁 06` | Viewport | Configure the viewport to render geometry within a specific screen region. | 🔴 |
| `📁 06` | Scissor Test | Restrict the pixels that can be written using a scissor rectangle. | 🔴 |
| `📁 06` | Multiple Viewports | Divide the screen into multiple viewing regions by setting up different viewports. | 🔴 |
| `📁 06` | Back-face Culling | Enable back-face culling and observe the difference between front-facing and back-facing surfaces. | 🔴 |
| `📁 06` | Front Face Winding | Experiment with clockwise and counter-clockwise winding orders. | 🔴 |
| `📁 06` | Fill Mode | Compare polygon fill modes and polygon rasterization modes supported by SDL3 and its backends. | 🔴 |
| `📁 06` | Alpha Blending | Render two semi-transparent shapes and configure the blend state. | 🔴 |
| `📁 06` | Additive Blending | Create a color-addition effect by blending multiple objects. | 🔴 |
| `📁 06` | Depth Write Control | Control depth writing to handle transparent objects. | 🔴 |
| `📁 06` | Stencil Test | Use a stencil buffer to restrict rendering to a marked region. | 🔴 |
| `📁 07` | Texture Atlas | Pack multiple small images into a single texture and access individual regions using UV coordinates. | 🔴 |
| `📁 07` | Sprite Sheet Animation | Use a sprite sheet to animate a 2D character. | 🔴 |
| `📁 07` | Texture Channel Visualization | Visualize the R, G, B, and A channels of a texture independently. | 🔴 |
| `📁 07` | Grayscale Filter | Convert a color texture into a grayscale image in the Fragment Shader. | 🔴 |
| `📁 07` | Color Inversion | Create a color inversion effect by transforming RGB values. | 🔴 |
| `📁 07` | Gamma Correction | Experiment with gamma correction and compare linear color with gamma-encoded color. | 🔴 |
| `📁 07` | Mipmap Generation | Generate mip levels for a texture and observe how they affect objects viewed from a distance. | 🔴 |
| `📁 07` | Anisotropic Filtering | Experiment with anisotropic filtering if supported by the backend and hardware. | 🔴 |
| `📁 07` | Texture Format Comparison | Compare texture formats such as RGBA8, BGRA8, and supported compressed formats. | 🔴 |
| `📁 07` | Cubemap Environment | Create a cubemap and render the surrounding environment of an object using cubemap sampling. | 🔴 |
| `📁 08` | First Compute Shader | Write a simple compute shader and dispatch a workgroup. | 🔴 |
| `📁 08` | Thread ID Visualization | Use `SV_DispatchThreadID` to write each thread's ID into a buffer. | 🔴 |
| `📁 08` | Vector Addition | Perform vector addition on two arrays using the GPU. | 🔴 |
| `📁 08` | Scalar Multiplication | Multiply every element in an array by a scalar value. | 🔴 |
| `📁 08` | Matrix Multiplication | Perform matrix multiplication using a compute shader. | 🔴 |
| `📁 08` | Workgroup Size | Experiment with different `numthreads` configurations and observe the resulting number of threads. | 🔴 |
| `📁 08` | Multiple Workgroups | Dispatch multiple workgroups to process a larger array. | 🔴 |
| `📁 08` | Bounds Checking | Handle cases where the number of elements is not evenly divisible by the number of threads in a workgroup. | 🔴 |
| `📁 08` | GPU Image Processing | Use a compute shader to process a texture and write the results into a storage texture. | 🔴 |
| `📁 08` | Compute to Graphics | Generate a texture using a compute shader, then use it in a graphics pipeline for display. | 🔴 |
| `📁 09` | Fullscreen Quad | Create a fullscreen quad for displaying post-processing results. | 🔴 |
| `📁 09` | Blur Filter | Blur an image by sampling neighboring pixels in the Fragment Shader. | 🔴 |
| `📁 09` | Sharpen Filter | Create an image sharpening effect using a convolution kernel. | 🔴 |
| `📁 09` | Edge Detection | Detect image edges using the Sobel operator. | 🔴 |
| `📁 09` | Sepia Filter | Create a sepia effect using color transformations. | 🔴 |
| `📁 09` | Brightness and Contrast | Adjust the overall brightness and contrast of an image. | 🔴 |
| `📁 09` | Multi-pass Rendering | Render a scene through two consecutive render passes to create a post-processing effect. | 🔴 |
| `📁 09` | Normal Visualization | Generate a normal buffer and visualize its normal vectors as RGB colors. | 🔴 |
| `📁 09` | Diffuse Lighting | Implement Lambertian lighting based on surface normals and light direction. | 🔴 |
| `📁 09` | Specular Lighting | Add specular highlights using Blinn-Phong lighting. | 🔴 |
| `📁 10` | 2D Sprite Renderer | Build a renderer capable of rendering multiple sprites using vertex buffers, textures, and samplers. | 🔴 |
| `📁 10` | Sprite Animation System | Manage multiple animations, frame indices, and animation timing. | 🔴 |
| `📁 10` | 2D Camera | Build a 2D camera that supports panning and zooming. | 🔴 |
| `📁 10` | Tilemap Renderer | Render a grid-based map using a tile atlas and an index buffer. | 🔴 |
| `📁 10` | Particle System | Create thousands of particles with positions, velocities, and colors that change over time. | 🔴 |
| `📁 10` | GPU Particle Simulation | Move particle updates from the CPU to a Compute Shader. | 🔴 |
| `📁 10` | 3D Model Renderer | Render a 3D model with vertex positions, normals, UV coordinates, and textures. | 🔴 |
| `📁 10` | Dynamic Lighting Scene | Build a 3D scene with a camera, multiple objects, and multiple light sources. | 🔴 |
| `📁 10` | Shadow Mapping | Generate a shadow map from the light's point of view and use it to determine shadowed regions in the main scene. | 🔴 |
| `📁 10` | Mini Rendering Engine | Build a small rendering application featuring a camera, meshes, textures, materials, depth testing, lighting, and post-processing. | 🔴 |

- 🔴 Not Started
- 🟡 In Progress
- 🟢 Completed