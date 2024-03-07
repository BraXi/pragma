# RENDERER-NEXT
An attempt to bring renderer to OpenGL 3.3, while making it many times faster than it currently is


## TODO:
- faster culling (PVS is slow)
- local entities rendering and culling based on PVS ?
- get rid of `GL_POLYGON` - convert to `GL_TRIANGLE`
- get rid of `GL_TRIANGLE_STRIP` - convert to `GL_TRIANGLE`
- remove calls to `glColor`, use uniforms instead
- render brushmodels and entirity of world with vertexbuffers
- get rid of IM translation/rotation -- calculate matrices and pass them to shaders
- setup projection without immediate calls
- alphatest is gone in newer opengl, do it in shaders, remove calls to `glAlphaTest`
- proper multitexture support
- lightmap updates
- anistropic texture filtering
- MD5 models and skeletal animations
- add support for improved q2bsp [lightgrids, brush normals, etc]
- add `r_picmip` back

## PARTIALY DONE:
- use GLSL shaders for diferent gemoetries - sky, models, warps, world, gui, etc..

## DONE:
- support GLSL shaders
- implement framebuffer objects
- get rid of `GL_QUAD`
- make `r_gamma` not require restarts
- cull models based on culldistance and frustum
- post processing effects such as film grain, saturation, contrast, intensity, grayscale, blur, etc..
- render all the ui with vbos
- render md3 models with vbos
- animate md3 models entirely on GPU -- this has improved pefrormance from 3-6fps to >200fps on my machine (bench1.bsp, 362 animated models, ~5mil verts)
- use opengl to generate mipmaps, thus allow for better textures

