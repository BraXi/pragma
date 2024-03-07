# RENDERER-NEXT
An attempt to bring renderer to OpenGL 3.3

## TODO:
- get rid of `GL_POLYGON`
- remove calls to glColor, use uniforms instead
- render brushmodels and entirity of world with vertexbuffers
- get rid of IM translation/rotation -- calculate matrices and pass them to shaders
- setup projection without immediate calls
- alphatest is gone in newer opengl, do it in shaders, remove calls to glAlphaTest
- proper multitexture support
- anistropic texture filtering
- MD5 models and skeletal animations
- add `r_picmip` back

## DONE:
- support GLSL shaders [X]
- implement framebuffer objects
- get rid of `GL_QUAD` [X]
- make `r_gamma` not require restarts [X]
- post processing effects such as film grain, saturation, contrast, intensity, grayscale, blur, etc.. [X]
- use GLSL shaders for diferent gemoetries - sky, models, warps, world, gui, etc.. [partialy done]
- render all the ui with vbos [X]
- render md3 models with vbos [X]
- use opengl to generate mipmaps, thus allow for better textures [X]