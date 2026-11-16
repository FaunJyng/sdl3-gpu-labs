# SDL3 GPU API Labs

This repository contains 100 hands-on labs focused on SDL3, specifically its GPU API.

> [!NOTE]
> Not all 100 labs have been completed yet. I aim to finish all of them according to the original plan.

All labs are located in the `📁 labs` directory and organized into numbered subdirectories (`📁 01`, `📁 02`, etc.). The labs are arranged in ascending order of difficulty, with each subsequent group introducing more advanced concepts and challenges. The `📁 deps` includes all dependecies (SDL3, SDL_iamge, etc.) for all labs. 

A description and status of each lab is provided below.

| Source Folder | No. | Name | Requirements | Status |
|:------:|:---:|:---|:---|:---:|
| `📁 01` | 1 | Clear Screen | Clear the entire screen to a single color. | 🟢 |
| `📁 01` | 2 | Animated Background | Continuously change the background color over time. | 🟢 |
| `📁 01` | 3 | First Triangle | Render a triangle using a Vertex Shader and a Fragment Shader. | 🟢 |
| `📁 01` | 4 | Triangle Color | Assign different colors to each vertex and observe color interpolation. | 🟢 |
| `📁 01` | 5 | Gradient Background | Create a background with a smoothly changing RGB color gradient. | 🟢 |
| `📁 01` | 6 | Triangle Position | Modify the coordinates of the three vertices to move the triangle. | 🔴 |
| `📁 01` | 7 | Renders a triangle with red, green, and blue vertices that smoothly interpolates colors across its surface while rotating continuously in a circle. | 🔴 |
| `📁 01` | 8 | Multiple Triangles | Render multiple triangles within a single frame. | 🔴 |
| `📁 01` | 9 | Triangle Rotation | Rotate a triangle using coordinate transformations in HLSL. | 🔴 |
| `📁 01` | 10 | Animated Triangle | Animate a triangle by making it move, rotate, and change color over time. | 🔴 |
| `📁 02` | 11 | Vertex Buffer Triangle | Store three vertices in an `SDL_GPUBuffer` and use it to render a triangle. | 🔴 |
| `📁 02` | 12 | Vertex Structure | Define a C++ struct containing position and color attributes for each vertex. | 🔴 |
| `📁 02` | 13 | Vertex Input Layout | Define vertex attributes and configure their mapping to Vertex Shader inputs. | 🔴 |
| `📁 02` | 14 | Attribute Offset | Modify the position and color attribute offsets and observe how the GPU reads vertex data. | 🔴 |
| `📁 02` | 15 | Attribute Format | Experiment with different vertex data formats, such as `float2`, `float3`, and `float4`. | 🔴 |
| `📁 02` | 16 | Multiple Vertex Buffers | Store vertex positions and colors in two separate vertex buffers. | 🔴 |
| `📁 02` | 17 | Triangle with Index Buffer | Use an index buffer to render a triangle. | 🔴 |
| `📁 02` | 18 | Indexed Quad | Render a quad using four vertices and six indices. | 🔴 |
| `📁 02` | 19 | Primitive Topology | Experiment with the primitive topologies supported by SDL3 and compare how they construct geometry. | 🔴 |
| `📁 02` | 20 | Wireframe Geometry | Render geometric edges using line primitives instead of filled triangles. | 🔴 |
| `📁 03` | 21 | Push Constant Color | Pass color data from C++ to the Fragment Shader using push constants. | 🔴 |
| `📁 03` | 22 | Push Constant Position | Pass position coordinates to control the triangle's position. | 🔴 |
| `📁 03` | 23 | Time Uniform | Pass time information from the CPU to the shader to create animations. | 🔴 |
| `📁 03` | 24 | Scale Transformation | Scale a triangle up and down. | 🔴 |
| `📁 03` | 25 | Rotation Transformation | Rotate a triangle around its center. | 🔴 |
| `📁 03` | 26 | Translation Transformation | Move geometry using translation transformations. | 🔴 |
| `📁 03` | 27 | Combined Transformations | Combine translation, rotation, and scaling transformations. | 🔴 |
| `📁 03` | 28 | Multiple Objects | Render multiple objects, each with its own transformation. | 🔴 |
| `📁 03` | 29 | Per-object Color | Pass different colors to individual objects using push constants. | 🔴 |
| `📁 03` | 30 | Transform Matrix | Use 3×3 or 4×4 matrices to transform vertices in HLSL. | 🔴 |
| `📁 04` | 31 | Load Image Texture | Load an image and upload it as a GPU texture. | 🔴 |
| `📁 04` | 32 | Texture Mapping | Map a texture onto a quad using UV coordinates. | 🔴 |
| `📁 04` | 33 | UV Visualization | Visualize UV coordinates as colors to understand texture mapping. | 🔴 |
| `📁 04` | 34 | Texture Tiling | Repeat a texture across a surface by modifying UV coordinates. | 🔴 |
| `📁 04` | 35 | Texture Offset | Move a texture across a surface without moving the underlying geometry. | 🔴 |
| `📁 04` | 36 | Nearest Sampling | Use nearest-neighbor filtering and observe the resulting pixelation. | 🔴 |
| `📁 04` | 37 | Linear Sampling | Use linear filtering and compare its results with nearest-neighbor filtering. | 🔴 |
| `📁 04` | 38 | Address Modes | Experiment with clamp, repeat, mirror, and other supported texture address modes. | 🔴 |
| `📁 04` | 39 | Multiple Textures | Use two textures within the same Fragment Shader. | 🔴 |
| `📁 04` | 40 | Texture Blending | Blend two textures based on a blending factor. | 🔴 |
| `📁 05` | 41 | Offscreen Rendering | Render a triangle into an `SDL_GPUTexture` instead of the swapchain. | 🔴 |
| `📁 05` | 42 | Display Render Target | Display the rendered texture on the screen using a fullscreen quad. | 🔴 |
| `📁 05` | 43 | Render Target Resolution | Render to a texture with a lower resolution than the window, then upscale it to the screen. | 🔴 |
| `📁 05` | 44 | Multiple Render Targets | Create two render targets and render the same scene into both textures. | 🔴 |
| `📁 05` | 45 | Depth Buffer Creation | Create a depth texture and attach it to a graphics pipeline. | 🔴 |
| `📁 05` | 46 | Depth Testing | Render two overlapping triangles and enable depth testing to handle near and far objects correctly. | 🔴 |
| `📁 05` | 47 | Depth Visualization | Visualize depth values as grayscale intensities. | 🔴 |
| `📁 05` | 48 | 3D Cube | Construct a cube from triangles and render it using perspective projection. | 🔴 |
| `📁 05` | 49 | Camera Movement | Move a camera around a 3D cube. | 🔴 |
| `📁 05` | 50 | Multiple 3D Objects | Render multiple cubes with different positions and depths. | 🔴 |
| `📁 06` | 51 | Viewport | Configure the viewport to render geometry within a specific screen region. | 🔴 |
| `📁 06` | 52 | Scissor Test | Restrict the pixels that can be written using a scissor rectangle. | 🔴 |
| `📁 06` | 53 | Multiple Viewports | Divide the screen into multiple viewing regions by setting up different viewports. | 🔴 |
| `📁 06` | 54 | Back-face Culling | Enable back-face culling and observe the difference between front-facing and back-facing surfaces. | 🔴 |
| `📁 06` | 55 | Front Face Winding | Experiment with clockwise and counter-clockwise winding orders. | 🔴 |
| `📁 06` | 56 | Fill Mode | Compare polygon fill modes and polygon rasterization modes supported by SDL3 and its backends. | 🔴 |
| `📁 06` | 57 | Alpha Blending | Render two semi-transparent shapes and configure the blend state. | 🔴 |
| `📁 06` | 58 | Additive Blending | Create a color-addition effect by blending multiple objects. | 🔴 |
| `📁 06` | 59 | Depth Write Control | Control depth writing to handle transparent objects. | 🔴 |
| `📁 06` | 60 | Stencil Test | Use a stencil buffer to restrict rendering to a marked region. | 🔴 |
| `📁 07` | 61 | Texture Atlas | Pack multiple small images into a single texture and access individual regions using UV coordinates. | 🔴 |
| `📁 07` | 62 | Sprite Sheet Animation | Use a sprite sheet to animate a 2D character. | 🔴 |
| `📁 07` | 63 | Texture Channel Visualization | Visualize the R, G, B, and A channels of a texture independently. | 🔴 |
| `📁 07` | 64 | Grayscale Filter | Convert a color texture into a grayscale image in the Fragment Shader. | 🔴 |
| `📁 07` | 65 | Color Inversion | Create a color inversion effect by transforming RGB values. | 🔴 |
| `📁 07` | 66 | Gamma Correction | Experiment with gamma correction and compare linear color with gamma-encoded color. | 🔴 |
| `📁 07` | 67 | Mipmap Generation | Generate mip levels for a texture and observe how they affect objects viewed from a distance. | 🔴 |
| `📁 07` | 68 | Anisotropic Filtering | Experiment with anisotropic filtering if supported by the backend and hardware. | 🔴 |
| `📁 07` | 69 | Texture Format Comparison | Compare texture formats such as RGBA8, BGRA8, and supported compressed formats. | 🔴 |
| `📁 07` | 70 | Cubemap Environment | Create a cubemap and render the surrounding environment of an object using cubemap sampling. | 🔴 |
| `📁 08` | 71 | First Compute Shader | Write a simple compute shader and dispatch a workgroup. | 🔴 |
| `📁 08` | 72 | Thread ID Visualization | Use `SV_DispatchThreadID` to write each thread's ID into a buffer. | 🔴 |
| `📁 08` | 73 | Vector Addition | Perform vector addition on two arrays using the GPU. | 🔴 |
| `📁 08` | 74 | Scalar Multiplication | Multiply every element in an array by a scalar value. | 🔴 |
| `📁 08` | 75 | Matrix Multiplication | Perform matrix multiplication using a compute shader. | 🔴 |
| `📁 08` | 76 | Workgroup Size | Experiment with different `numthreads` configurations and observe the resulting number of threads. | 🔴 |
| `📁 08` | 77 | Multiple Workgroups | Dispatch multiple workgroups to process a larger array. | 🔴 |
| `📁 08` | 78 | Bounds Checking | Handle cases where the number of elements is not evenly divisible by the number of threads in a workgroup. | 🔴 |
| `📁 08` | 79 | GPU Image Processing | Use a compute shader to process a texture and write the results into a storage texture. | 🔴 |
| `📁 08` | 80 | Compute to Graphics | Generate a texture using a compute shader, then use it in a graphics pipeline for display. | 🔴 |
| `📁 09` | 81 | Fullscreen Quad | Create a fullscreen quad for displaying post-processing results. | 🔴 |
| `📁 09` | 82 | Blur Filter | Blur an image by sampling neighboring pixels in the Fragment Shader. | 🔴 |
| `📁 09` | 83 | Sharpen Filter | Create an image sharpening effect using a convolution kernel. | 🔴 |
| `📁 09` | 84 | Edge Detection | Detect image edges using the Sobel operator. | 🔴 |
| `📁 09` | 85 | Sepia Filter | Create a sepia effect using color transformations. | 🔴 |
| `📁 09` | 86 | Brightness and Contrast | Adjust the overall brightness and contrast of an image. | 🔴 |
| `📁 09` | 87 | Multi-pass Rendering | Render a scene through two consecutive render passes to create a post-processing effect. | 🔴 |
| `📁 09` | 88 | Normal Visualization | Generate a normal buffer and visualize its normal vectors as RGB colors. | 🔴 |
| `📁 09` | 89 | Diffuse Lighting | Implement Lambertian lighting based on surface normals and light direction. | 🔴 |
| `📁 09` | 90 | Specular Lighting | Add specular highlights using Blinn-Phong lighting. | 🔴 |
| `📁 10` | 91 | 2D Sprite Renderer | Build a renderer capable of rendering multiple sprites using vertex buffers, textures, and samplers. | 🔴 |
| `📁 10` | 92 | Sprite Animation System | Manage multiple animations, frame indices, and animation timing. | 🔴 |
| `📁 10` | 93 | 2D Camera | Build a 2D camera that supports panning and zooming. | 🔴 |
| `📁 10` | 94 | Tilemap Renderer | Render a grid-based map using a tile atlas and an index buffer. | 🔴 |
| `📁 10` | 95 | Particle System | Create thousands of particles with positions, velocities, and colors that change over time. | 🔴 |
| `📁 10` | 96 | GPU Particle Simulation | Move particle updates from the CPU to a Compute Shader. | 🔴 |
| `📁 10` | 97 | 3D Model Renderer | Render a 3D model with vertex positions, normals, UV coordinates, and textures. | 🔴 |
| `📁 10` | 98 | Dynamic Lighting Scene | Build a 3D scene with a camera, multiple objects, and multiple light sources. | 🔴 |
| `📁 10` | 99 | Shadow Mapping | Generate a shadow map from the light's point of view and use it to determine shadowed regions in the main scene. | 🔴 |
| `📁 10` | 100 | Mini Rendering Engine | Build a small rendering application featuring a camera, meshes, textures, materials, depth testing, lighting, and post-processing. | 🔴 |

- 🔴 Not Started
- 🟡 In Progress
- 🟢 Completed