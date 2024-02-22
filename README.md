### Lame Vulkan Renderer Project
![Preview](/extra/preview.png)
#### How to build (Windows VS2022)
1. Make sure Vulkan SDK is installed and the environment variable is set.
2. Go to the "scripts" folder and run "Generate_Solution_VS2022.bat".
3. Go back to root and go to the "generated" folder. The VS solution should be there.
4. Open the solution. Build and run the project.
#### TODOs:
- [x] Create base program
- [x] Flip viewport
- [x] Write a naive memory allocator (deprecated)
- [x] Queue ownership transfer
- [x] Simple push constants usage (disabled)
- [x] Basic scene with multiple objects
- [x] Simple compute shader (disabled)
- [x] Integrate GPU Open VMA
- [x] Adding a cubemap skybox
- [ ] Instanced rendering
- [ ] Descriptor Management
- [ ] Dynamic uniforms
- [ ] Separate images and sampler descriptors
- [ ] Pregenerated mipmap
- [ ] Pipeline cache
- [ ] Shader permutation
- [ ] Async queue submission
- [ ] Multi-threaded command buffer generation
- [ ] Multiple subpasses
