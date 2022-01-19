# Components

The "parts of the engine". A data-centric approach to allow decoupling of game modules.

## Modules / Systems
- World System
  - Terrain

- Object System
  - Structures
  - Items
  - Decals

- Character System
  - Player
  - NPCs
  - Action Input

- Input System
  - Keyboard interactions (Player)
  - AI (NSCs)

- Camera System

- Light System
  - Ambient Light (Moon)1
  - Light objects

- Audio System
  - Music
  - Sounds

- Physics System
  - Collision

## Current Components
### Mesh Component
  Holds geometric information stored per vertex in a special half-edge format, returns a VAO.
  - Position
  - Normal
  - Tangent
  - Texcoord
### Transform Component (advanced)
  Holds information about model transformations and methods for applying those.
  - Translation
  - Scaling
  - Rotation
### World Component (advanced)
  Handles the (rendered) world objects (instances), also restricts them to a max number.
  - [Instance_Component]
### Instance Component (advanced)
  A "renderable", saving its states.
  - [Mesh_Component]
  - Textures (Albedo, Normal, AO+Roughness+Metallic)
  - [Transform_Component]
  + Currently holds animation, but this should be seperate
### Statistics Component (advanced)
  Giving out useful debug information.
### Editor Component (advanced)
  Useful to experiment with placing objects and obtain the transform-coordinates to bake them into the code.
### Advanced Component (advanced)
  Initializes the demo and statistics.
  - [World_Component]
  - [Statistics_Component]
### Freefly Camera Component
  Holds camera information.
### Game Component
  Does the main loop.
  - [Camera_Component]
  - [Advanced_Component]
  - Also updates the camera

## Future Components
### Light System
  Describes the light perimeters
  - Position
  - Radius
  - Intensity
  - ...
### 
