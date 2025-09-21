OpenGL-Rendering Engine: Physically based renderer with deferred shading, SSAO, HDR, Bloom, IBL, and instancing.

It shows below each stage of feature integrated into the engine 

---

### Stage 1 - Foundations

Like many engines, began with the most basic scene:
a single triangle on screen and from there gradually built the core abstractions.

To support future features, I built core abstractions:
- Hot Reload Shader system for compiling GLSL and managing uniforms.  
- Mesh & Model classes (with Assimp integration for OBJ/FBX/GLTF).  
- Camera supporting.  
- Pipelines for depth, blending, stencil, and rasterizer states.  
---

### Stage 2 - Lighting & Shading

Next came Phong and Blinn–Phong shading, with:

- Diffuse and specular texture maps.  
- Directional, Point, and Spot lights, each with attenuation.  

(Multiple Lights)  
![Phong lighting](https://media.githubusercontent.com/media/KaanAres10/OpenGL-Engine/web/assets/Lighting.png)

---

### Stage 3 - Shadows

Lighting without shadows felt incomplete. I implemented:

- Directional shadow mapping with orthographic projections.  
- Point light shadows using depth cubemaps and omnidirectional sampling.  

I added ImGui debug views to inspect light frustums and tweak bias/PCF settings.  

(Directional shadows projected into the cathedral)  
![Shadows](https://media.githubusercontent.com/media/KaanAres10/OpenGL-Engine/web/assets/RenderDoc_Shadpw.png)  

(PointLight shadows)  
![Shadows2](https://media.githubusercontent.com/media/KaanAres10/OpenGL-Engine/web/assets/RenderDoc_PointLigthShadow.png)

---

### Stage 4 - Normal & Parallax Mapping

Flat textures weren’t enough so introduced:

- Normal Mapping for fine surface detail.  
- Parallax Occlusion Mapping for height illusion.  

(Normal Map)  
![Normal](https://media.githubusercontent.com/media/KaanAres10/OpenGL-Engine/web/assets/Screenshot%202025-09-20%20230928.png)
(Parallax Test)
![Parallax test](https://media.githubusercontent.com/media/KaanAres10/OpenGL-Engine/web/assets/Parallax.png)

---

### Stage 5 - HDR, Bloom & Post-Processing

The next is through tone mapping and bloom:

- HDR framebuffer with exposure controls.  
- Multi-pass Gaussian blur bloom.  
- Gamma correction for accurate brightness.  

(Bloom on emissive lights in Sponza)  
![Bloom](https://media.githubusercontent.com/media/KaanAres10/OpenGL-Engine/web/assets/Bloom.png)  
(High Exposure)  
![Bloom](https://media.githubusercontent.com/media/KaanAres10/OpenGL-Engine/web/assets/High_Exposure.png)
(Pixelated)
![Pixel](https://media.githubusercontent.com/media/KaanAres10/OpenGL-Engine/web/assets/Screenshot%202025-09-20%20225329.png)

---

### Stage 6 - Deferred Rendering & SSAO 

Switching to Different Rendering Technique:

- Deferred Shading with a G-buffer (albedo, normals, positions).  
- SSAO (Screen Space Ambient Occlusion) with random kernel samples + blur.  

RenderDoc captures, stepping through G-buffer attachments.  

(G-buffer outputs)

<p align="center" style="margin-top:20px;">
  <img src="https://media.githubusercontent.com/media/KaanAres10/OpenGL-Engine/web/assets/Deferred.png" alt="Deferred"/>
</p>

(SSAO pass)

<p align="center" style="margin-top:20px;">
  <img src="https://media.githubusercontent.com/media/KaanAres10/OpenGL-Engine/web/assets/ssao.png" alt="SSAO"/>
</p>

---

### Stage 7 - Physically Based Rendering 

I implemented Cook–Torrance BRDF with:

- Albedo, Metallic, Roughness, AO, and Normal maps.  
- Energy-conserving specular highlights.  

Materials began to look photorealistic.  

(Rusty metal sphere)  
![PBR](https://media.githubusercontent.com/media/KaanAres10/OpenGL-Engine/web/assets/rust.png)

---

### Stage 8 - Image-Based Lighting

To further, added IBL (Image-Based Lighting):

- Converted HDR equirectangular maps → cubemaps.  
- Precomputed diffuse irradiance and specular prefilter maps.  
- Integrated into the PBR shader.  

Now objects reflected their environment naturally, even without direct lights.  

(Spheres under HDR skybox with specular reflections)  
![IBL](https://media.githubusercontent.com/media/KaanAres10/OpenGL-Engine/web/assets/ibl.png)

---

### Stage 9 - Instancing & Vegetation 

I added GPU instancing to draw thousands of leaves with a single draw call.  
This unlocked massive vegetation rendering.  

(100.000 Vegetation instancing test grid)  
![Instancing](https://media.githubusercontent.com/media/KaanAres10/OpenGL-Engine/web/assets/instancing.png)

---

### Stage 10 - Debugging Tools

Throughout Debug, I leaned on:

- RenderDoc for frame captures  
- ImGui for parameter settings

(Debug Normals)
![Normal](https://media.githubusercontent.com/media/KaanAres10/OpenGL-Engine/web/assets/Normal.png)  
(Debug Shadow Aliasing)
![Shadow Bias](https://media.githubusercontent.com/media/KaanAres10/OpenGL-Engine/web/assets/Screenshot%202025-08-04%20170230.png)  

---

## Reflection

This project taught me:

- The full graphics pipeline, from vertex data to post-processing.  
- How to manage a C++ engine architecture with reusable systems.  
- How modern rendering features (IBL, SSAO) work mathematically and practically.  

---

## Tech 

- **Languages**: C++17, GLSL  
- **Graphics**: OpenGL 4.5  
- **Build & Debug**: CMake, RenderDoc  


