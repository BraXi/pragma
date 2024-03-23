# RENDERER
An attempt to bring renderer to OpenGL 2.1, while making it many times faster than GL1.4 renderer was
Once the renderer is core GL 2.1 compatible it can be easily upgraded to GL 3.* if needed

## TODO:
- do _NOT_ update lightmaps! It's tanking performance with dlights and lightstyles due to texture uploads, Instead do what YamagiQ2 GL3 renderer does
- move lighting (styles, dlights) to GPU and add per pixel lighting (releated to upper) -- dlights are ugly with higher res lightmaps
- anistropic texture filtering
- entities should be lit by lightgrid instead of lightmap pixel underneath them (which is awful for objects in air) -- add support for `LIGHTGRID_OCTREE`
- setup projection without immediate calls
- get rid of IM translation/rotation -- calculate matrices and pass them to shaders
- get rid of `GL_POLYGON` draws convert them to `GL_TRIANGLE_STRIP` ?
- convert remaining code that does immediate rendering (glBegin/glEnd) to vertex buffers
- render brushmodels and entirity of world with vertexbuffers
- local entities rendering and culling based on PVS ?
- alphatest is gone in newer opengl, do it in shaders, remove calls to `glAlphaTest`
- exponential fog + make it cull objects that are completly occluded
- MD5 models and skeletal animations (maybe?)
- add `r_picmip` back
- particles can be optionaly lit
- sky rendering code is awful

## PARTIALY DONE:
- use GLSL shaders for diferent gemoetries - sky [partial], models [done], warps [-], world [partial], gui [done], particles [done], etc..
- move viewblend to shader (problem: it looks worse)
- multitexture support -- currently only two TMUs, we need more forl ighting changes

## DONE:
- all UI rendering is now done with vertexbuffers (bringing up console no longer tanks FPS)
- support `QBISM` extended BSP format allowing for greatly more advanced maps
- world loading code is aware of `BSPX` lumps
- decoupled lightmap coords from texture coords (`DECOUPLED_LM` lump) - this also allows for much higher (or lower) quality lightmaps
- configurable `LMSHIFT` (was needed for `DECOUPLED_LM`)
- support GLSL shaders
- implement framebuffer objects and render all the world to texture for post processing
- get rid of `GL_QUAD` primitives
- make `r_gamma` not require restarts
- cull MD3 models based on distance and frustum
- post processing effects such as film grain, saturation, contrast, intensity, grayscale, blur, etc..
- render MD3 models with vbos
- animate MD3 models entirely on GPU -- this has improved pefrormance from 3-6fps to >200fps on my machine (bench1.bsp, 362 animated models, ~5mil vert transforms)
- use opengl to generate mipmaps, thus allow for better textures
- remove calls to `glColor`, use uniforms instead
- particles can be of any size
