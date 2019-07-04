# Adept Engine

A flexible C++ Game engine focusing on advanced multi-GPU rendering.

This is currently VERY work in progress

[Road Map](https://trello.com/b/tiTV3Fbs/adept-engine-roadmap)

[Dev Blog](https://andrewcjp.wordpress.com/)

[Feature List]([https://github.com/Andrewcjp/Adept-Engine/tree/master/GraphicsEngine/Readme/Feature-List.md])

Platform Support:

* Windows 10 (1803+)
* Linux (planned)

Requirements:

* Windows 10 SDK (1803+)
* DirectX 12 capable GPU
* Visual Studio 2017 (2019 Support Planned)

Key Feature Overview:

* DirectX 12
* Vulkan (WIP)
* Render graph system for flexible renderer design
* Raytracing (DXR) Support (WIP)
* VR rendering support (SteamVR)
* Multi-GPU Shadow mapping
* Asynchronous Multi-GPU Shadow Mapping
* Split frame rendering (SFR)
* SFR with Multi-GPU Shadow mapping

Build Instructions:

1. Install CMake (version 3.12+)
2. Run EngineBuildTool.exe
3. Run Source/GitDependencies.exe
4. Open solution in Visual studio 2017
5. Manually Build the HeaderTool Project
6. Build and Run!
