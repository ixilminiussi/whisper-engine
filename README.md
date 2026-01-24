# Whisper Engine
*A Vulkan-based rendering engine for glTF format, built as a learning project in computer graphics and engine architecture.
Features*

# Rendering
* PBR (Physically Based Rendering) with metallic-roughness workflow
* Environment mapping using HDRIs for diffuse lighting
* Shadow mapping and SSAO (Screen Space Ambient Occlusion)
* Render graph system for rapid iteration on rendering techniques
* Optimized for outdoor scenes with natural lighting

# Performance
* Triple-buffered rendering pipeline
* Profiled and optimized using Tracy
* Runs Sponza scene at 60 fps on Intel integrated graphics

# Developer Tools
* *Frost*: Custom code reflection tool for rapid development
* Real-time scene editing with ImGui integration
* Vendor-locked dependencies for long-term stability

# Technical Stack
* Graphics API: Vulkan (vendored for stability)
* 3D Format: glTF 2.0
* Language: C++17
* Build System: CMake

# Building
```bash
mkdir build && cd build
cmake ..
cmake --build .
```

*All dependencies are vendored in the vendor/ directory—no external downloads required.*

## Post-Mortem: Lessons Learned
This was my first major engine project, and it taught me invaluable lessons about both graphics programming and software architecture.

### What Went Right
* **Vendoring dependencies**: After my previous engine (Chamoix) broke due to upstream library changes, I vendored everything. 
* **Render graph abstraction**: The graph-based rendering architecture made experimenting with new techniques straightforward.
* **Performance-first approach**: Profiling with Tracy from day one kept performance in check, and taught me to not always trust my instincts on what the *best* architecture is.

## What I'd Do Differently

* *C++ limitations became apparent*: Compile times ballooned as the project grew. The lack of native introspection made Frost necessary but cumbersome.
* *SOLID principles have trade-offs*: While clean, the strict adherence to polymorphism and single-responsibility led to verbose class hierarchies and indirection that hurt both readability and compile times.

## Moving Forward
I'm grateful for what C++ taught me, but I've hit its limitations for rapid prototyping. I'm now porting this engine to Jai, a language designed to address compile times and introspection.
This isn't a rejection of C++, but an exploration of alternatives. I want my expertise to be in programming, not just navigating the C++ standard library.
