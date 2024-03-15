# RENDERER-NEXT
An attempt to bring renderer to OpenGL 2.1, while making it many times faster than it currently is


## TODO:
- faster culling? (PVS is slow)
- local entities rendering and culling based on PVS ?
- get rid of `GL_POLYGON` - convert to `GL_TRIANGLE`
- get rid of `GL_TRIANGLE_STRIP` - convert to `GL_TRIANGLE`
- render brushmodels and entirity of world with vertexbuffers
- get rid of IM translation/rotation -- calculate matrices and pass them to shaders
- setup projection without immediate calls
- alphatest is gone in newer opengl, do it in shaders, remove calls to `glAlphaTest`
- proper multitexture support -- which is currently disabled to not interfere with world rendering
- per pixel lighting?
- lightmap updates
- anistropic texture filtering
- fog
- MD5 models and skeletal animations
- add support for improved q2bsp [lightgrids, brush normals, etc]
- add `r_picmip` back
- particles can be optionaly lit
- console is drawning char by char with `Draw_Char` which is slow
- sky rendering code is awful

## PARTIALY DONE:
- use GLSL shaders for diferent gemoetries - sky [partial], models [X], warps [-], world [partial], gui [X], particles [X], etc..
- move viewblend to shader (problem: it looks worse)

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
- remove calls to `glColor`, use uniforms instead
- particles can be of any size
