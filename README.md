# Raytracing
A simple Raytracing-Algorithm based on the article of scrathpixel.
Extended with soft shadows. This project was developed for the university of applied science karlsruhe (germany), course of study 'computer science', subject 'computer graphics'.
Feel free to create pull requests!

## Features (already implemented)
* Spheres with transparency, emissive, reflective and surfaceColor
* Reflection
* Refraction
* Hard Shadows
* Soft Shadows
* Output in .ppm image

## Settings
Starting at line 94, you can controll the quality of the shadows and the overall result.
* Change the max. ray depth of all tracing
```C++
#define MAX_RAY_DEPTH 12
```
* Change the amount of shadow rays send by every pixel which intersects with a sphere. (0 -> no shadows, 1 -> hard shadows without noise, high value -> less noise)
```C++
#define SHADOW_RAYS 5000.0
```
* Adjust the Offset for all the shadow rays in percent. (0% = no offset -> hard shadows with noise)
```C++
#define OFFSET_PERCENT 3.50
```
* Control the how dark the shadow will be, if no shadow ray hits the emissive sphere
```C++
#define MIN_SHADOW_BRIGHTNESS 0.15
```

## Authors
Author of the scratchpixel.com blog, @mbpictures, @vacl.
