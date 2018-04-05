# sparklepp
particule simulation with OpenGL

![alt text](https://github.com/pptacher/sparklepp/blob/master/screenshot.jpg)

I have reengineered the particle engine from tcoppex/sparkle, to make it compatible with OpenGL 4.1 only hardware (Mac).

Compute shader functionality, atomic counter & shader storage extensions are not supported by OpenGL 4.1. 

Instead of shader storage, textures are used to share indices of particules to be sorted for alpha blending  between fragment shaders instanciations.
I use queries instead of atomic counter to get the number of alive particules.


## references

- *Spärkle*, Thibault Coppex, [A modern particle engine running on GPU, using c++11 and OpenGL 4.x.](https://github.com/tcoppex/sparkle)

- *GPU Gems*, Ian Buck Tim Purcell, [A Toolkit for Computation on GPUs.](http://developer.download.nvidia.com/books/HTML/gpugems/gpugems_ch37.html)

- *Noise-based particles*, Philip Rideout, [Noise-Based Particles, Part II](http://prideout.net/blog/?p=67)
