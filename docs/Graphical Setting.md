## Graphical Setting

To give impression examples how the rendering could look

- https://i.pinimg.com/originals/50/0d/10/500d104fa3bd02d06617f53c6b7a3809.png
- https://www.deviantart.com/xxxscope001xxx/art/Low-Poly-The-adventurers-grave-532640936
- https://imgur.com/9IXJhNj
- https://forum.unity.com/threads/low-poly-dungeon-pack.479591/
- https://i.pinimg.com/originals/7d/e4/35/7de435eca9f66e563d42db05bc16befc.jpg
- https://imgur.com/6stz5MJ

# Low Poly

- https://sundaysundae.co/how-to-make-low-poly-look-good/
- Simple geometry
- High resolution rendering
- Flat colors as texture
- Polygoncount reflects a) importance b) size
    - ~ half size => half subdivision
    - for differently scaled objects have different models with different polygoncount
- No smooth normals / No shared vertices
- Keep textures to a minimum, create complexity with lighting
    - But subtle textures can break up the surface slightly
    - Use specularity, refraction, scatter
- Be careful of saturation, specularity can help desaturizing
- LIGHTING is important
    - Directional light for the sun
    - Skydome for lighting up shadowed areas (best is cubemap with photographed sky)
    - Indirect light with 2-3 surface-bounces, expensive, therefore best baked into texture
- Also, post processing!
    - Depth Haze
    - Color Correction for the right mood


# Rendering Pipeline

- Forward Shading vs. Deferred Shading
    - Deferred Shading is especially suited towards many lightsources and shadows, but more complex, no semi-transparency (no depth value to test against) and no anti-aliasing
    - Forward Shading concept is simpler and applicable for all cases, but needs more complex shaders
- Deferred Shading may be better for this project
- Semitransparency can be handled using a Forward Shading pass at the end of the pipeline (however attracts errors)
- AA can be done in screen-space, i.e. with edge detection
- Maybe needs rendering to bigger framebuffer because of edge distortions

# Deferred Shading
- https://learnopengl.com/Advanced-Lighting/Deferred-Shading
- https://developer.nvidia.com/gpugems/gpugems2/part-ii-shading-lighting-and-shadows/chapter-9-deferred-shading-stalker

# Further Notes
- Fixed installed lights at lanterns could be resolved not as dynamic lights but as prebaked through a lightmap
    - Can be easily resolved using forward shading I think
- Big sorts of dynamic lights are, if static lights are moving a bit (modulation through the wind?) or if there are many nonstatic lights
    - Actors holding lanterns and moving
    - Maybe whisps in the forest
    - Gloomy eyes of creatures
    - A carriage with lanterns driving along the road, bringing pioneers
