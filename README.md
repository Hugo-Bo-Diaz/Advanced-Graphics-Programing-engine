# Advanced Graphics Programing engine
In this repository you will find an engine made for the subject of Advanced graphics programing with Jesus diaz as our teacher.

Before we begin I would like to talk a bit about how the properties of the objects can be observed more closely:

On the top-right corner you can find a list of all of the objects, if you were to click on them it would display its transform matrix as well as another button below that opens the list of submeshes that form the object.

//image

Clicking on any of these would open the material properties that affect that submesh, showing all of the textures as well as some sliders to change how they affect the final outcome of the texture.

//show textures

**CONTROLS**

- Left mouse: while held on the render view it allows for the user to change the camera orientation

- WASD: used for camera movement.

- SPACE: it toggles between the regular camera and orbital camera, when it is on orbital mode it centers on (0,0,0) and allows for camera movement forward and backward.

This engine was developed from the base engine provided for the subject by me in order to be able to apply some of the techniques that were explained in class. 
Now I will review the techniques applied.

## TECHNIQUES USED

### Deferred shading:

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

here we have the resulting image

<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/lighting_on.jpg?raw=true"  height="500">

In this engine you have the posibility of selecting to render with the "normal" method, rather known as forward shading or rendering with the deferred shading method by using these buttons on the left, you can also visualize the individual textures of the GBuffer when you are rendering with the deferred shading method

<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/deferred_widget.jpg?raw=true"  height="350">

You can also control the position, color and intensity of the lights present in the scene by using the panel on the bottom-right corner of the window

<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/lights_panel.jpg?raw=true"  height="350">

The shaders files that contain this effect are ```map_calculation.glsl``` to calculate the maps for the GBuffer and ```deferred.glsl``` to calculate the lights and how they affect the scene.

### Water effects:

This effect attempts to simulate how water reflects and refracts as well as how it distorts both of those images, here we have a comparison of how it affects a scene to have this effect on:

<p float="left">
<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/water_off.jpg?raw=true"  height="350">
<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/water_on.jpg?raw=true"  height="350">
</p>

In this case the effect can only be turned off or on using this checkbox on the bottom-left corner

<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/water_widget.jpg?raw=true"  height="350">

The shaders files that contain this effect are ```water_render.glsl``` to calculate the reflection and refraction maps and ```water_plane.glsl``` to actually shade the plane using the previously calculated maps.

### Normal maps:

This technique attempts to simulate depth on an object by changing the surface normals, allowing light and other effects to create detail without rendering additional geometry.

Here we have an example:
<p float="left">
<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/normals_off.jpg?raw=true"  height="350">
<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/normals_on.jpg?raw=true"  height="350">
</p>

In this engine you can use the afromentioned interface in order to change how much is the normal map going to affect the normals, the higher the number the more the normals will change according to the texture

<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/normals_widget.jpg?raw=true"  height="350">

This technique is allocated on the normal calculation that we can find in the files of ```forward_shading.glsl``` and ```map_calculation.glsl```

### Height/bump maps:

This technique focuses on creating a depth illusion by changing the position of the pixels when deciding its coordinates, it does it by applying some transformations according to a bump map. It is normally combined with a normal map in order to apply lighting calculations correctly onto the scene.

Here we have the three step process of applying these maps in our engine, we first apply the normals and then the bump map:

<p float="left">
<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/bumpmap_off.jpg?raw=true"  height="200">
<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/bumpmap_normals.jpg?raw=true"  height="200">
<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/bumpmap_on.jpg?raw=true"  height="200">
</p>


Just like the normal maps, you can change how much these calculations affect the result with the sliders mentioned previously on top of the texture showcased on the materials properties

<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/bump_widget.jpg?raw=true"  height="350">

Finally the application of this technique also means that the interaction of the surface with the surrounding objects should also change according to this new depth value added, we can find it here implemented on the engine

<img src="https://github.com/Hugo-Bo-Diaz/Advanced-Graphics-Programing-engine/blob/main/showcase_photos/bumpmap_showdepth.jpg?raw=true"  height="350">

The calculations performed here are also part of the ```forward_shading.glsl``` and ```map_calculation.glsl``` shaders, because of the same reasons as the normal maps.
