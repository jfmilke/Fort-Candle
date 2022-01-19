## World Map

About the generation of the world map, without putting too much effort into it.
World map refers to the terrain without further objects (vegetation, structures, objects)

# Low poly world

1. Construct the whole terrain in blender
  - Already map textures and save coordinates
  - Map textures by smart vertex derivations in the shader
2. Construct terrain patches in blender
  - Similar to above
3. Proceduraly construct terrain in OpenGL
  - Upload flat plane mesh to GPU
  - Optional: Tesselation Shader to subdivide Mesh
  - Random z-offset (i.e. based on white noise texture)
  - Map textures to plane
3. Grid system for a blocky world
  - Generate cubes by the coordinates
