# Advanced Graphics Programing engine
In this repository you will find an engine made for the subject of Advanced graphics programing with Jesus diaz as our teacher.

Before we begin I would like to talk a bit about how the properties of the objects can be observed more closely:

On the top-right corner you can find a list of all of the objects, if you were to click on them it would display its transform matrix as well as another button below that opens the list of submeshes that form the object.

//image

Clicking on any of these would open the material properties that affect that submesh, showing all of the textures as well as some sliders to change how they affect the final outcome of the texture.

//show textures

This engine was developed from the base engine provided for the subject by me in order to be able to apply some of the techniques that were explained in class. Some of these techniques are:

* Deferred shading:

This is a technique for lighting that pretends to save computational power by only making lighting calculations on the fragments that will be afected by them.

This is achieved by rendering the scene onto separate images (known as GBuffer) containing information such as the albedo color, fragment position, normals...

Then those maps are used in order to compute the lighting

Here we have an example:

GBuffer images:
<p float="left">
<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/lighting_off.jpg?raw=true"  height="250">
<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/gbuffer_p.jpg?raw=true"  height="250">
<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/gubffer_n.jpg?raw=true"  height="250">
<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/gbuffer_s.jpg?raw=true"  height="250">
</p>

result image

<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/lighting_on.jpg?raw=true"  height="500">

In this engine you have the posibility of selecting to render with the "normal" method, rather known as forward shading or rendering with the deferred shading method by using these buttons on the left, you can also visualize the individual textures of the GBuffer when you are rendering with the deferred shading method

<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/deferred_widget.jpg?raw=true"  height="350">

You can also control the position, color and intensity of the lights present in the scene by using the panel on the bottom-right corner of the window

<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/lights_panel.jpg?raw=true"  height="350">

The shaders files that contain this effect are ```map_calculation.glsl``` to calculate the maps for the GBuffer and ```deferred.glsl``` to calculate the lights and how they affect the scene.

* Water effects:

This effect attempts to simulate how water reflects and refracts as well as how it distorts both of those images, here we have a comparison of how it affects a scene to have this effect on:

<p float="left">
<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/water_off.jpg?raw=true"  height="350">
<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/water_on.jpg?raw=true"  height="350">
</p>

In this case the effect can only be turned off or on using this checkbox on the bottom-left corner

<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/water_widget.jpg?raw=true"  height="350">

The shaders files that contain this effect are ```water_render.glsl``` to calculate the reflection and refraction maps and ```water_plane.glsl``` to actually shade the plane using the previously calculated maps.

* Normal maps:

This technique attempts to simulate depth on an object by changing the surface normals, allowing light and other effects to create detail without rendering additional geometry.

Here we have an example:
<p float="left">
<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/normals_off.jpg?raw=true"  height="350">
<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/normals_on.jpg?raw=true"  height="350">
</p>

In this engine you 

show bumpiness dragfloat

forward_shading.glsl
map_calculation.glsl

* Height/bump maps:

comparison on/off

show bumpiness dragfloat

forward_shading.glsl
map_calculation.glsl
