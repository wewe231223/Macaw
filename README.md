# Macaw

## Project structure

| Project | Responsibility |
| --- | --- |
| Math | Vectors, matrices, quaternions, and SimpleMath |
| Core | Object system, memory, channels, shared messages and state types, archive interface |
| Serialization | JSON and memory archive implementations |
| Asset | Asset registry, importers, materials, meshes, and pipeline assets |
| World | Actors, components, subsystems, and world state |
| Render | D3D11 rendering, graphics buffers, and ImGui backend |
| Editor | Panels, viewports, input, undo, and component property panels |
| Macaw | Windows application entry point and resources |

The dependencies flow from Math through Core, Serialization, and Asset. World and Render depend on those lower projects; Editor depends on World and Render. `MacawTests` links the libraries instead of compiling the engine sources again.

Regenerate the Visual Studio solution and projects with `premake5 vs2022`.
