# 📝 TODO: Minecraft Clone (OpenGL)

A slow, structu- [x] Add basic chunk generation based on 2D Perlin noiseed plan to build a Minecraft clone using OpenGL.

---

## ✅ PHASE 1: Window & Rendering Setup

- [x] Create main loop with GLFW and OpenGL context
- [x] Create a window titled "Minecraft Clone"
- [x] Set background color and clear screen every frame
- [x] Add basic input handling (ESC to quit)
- [x] Print OpenGL version for verification

---

## ✅ PHASE 2: Cube Rendering

- [x] Define cube vertex data (positions only)
- [x] Create VAO, VBO, and EBO for the cube
- [x] Write minimal vertex and fragment shaders
- [x] Draw a static cube on screen

---

## ✅ PHASE 3: Camera System

- [x] Integrate GLM for math
- [x] Create a Camera class with:
  - [x] Position vector
  - [x] View matrix
  - [x] Projection matrix
- [x] Add WASD + mouse control for free movement
- [x] Implement first-person camera (yaw + pitch)

---

## ✅ PHASE 4: Chunk System

- [x] Create a `Chunk` class
- [x] Define a 3D array of blocks (16 x 256 x 16)
- [x] Create an enum for block types (Air, Dirt, Stone)
- [x] Generate a flat chunk with only dirt
- [x] Generate mesh data from block array
- [x] Only generate faces that are visible (not next to another solid block)

---

## ✅ PHASE 5: World & Chunk Management

- [x] Create a `World` class to manage chunks
- [x] Store chunks in a map: `std::unordered_map<ChunkCoord, Chunk>`
- [x] Load chunks around the player (e.g., 3x3 grid)
- [x] Unload distant chunks
- [x] Add basic chunk generation based on 2D Perlin noise

---

## ✅ PHASE 6: Texture & Lighting

- [x] Load texture atlas using `stb_image.h`
- [x] Add UV coordinates to block faces
- [x] Bind and use texture atlas in shader
- [x] Assign UVs based on block type
- [x] Add simple directional lighting (sunlight)
- [x] Add ambient lighting

---

## ✅ PHASE 7: Player Interaction

- [x] Cast a ray from camera to find targeted block
- [x] On left-click: remove block (set to Air)
- [x] On right-click: add block (e.g., Dirt) to adjacent face
- [x] Update chunk mesh on block change

---

## ✅ PHASE 8: Optimization

- [ ] Only update mesh when chunk data changes
- [ ] Implement greedy meshing to reduce face count
- [ ] Add basic frustum culling
- [ ] Use multi-threading for chunk generation

---

## ✅ PHASE 9: Polish

- [ ] Add basic crosshair in center of screen
- [ ] Add block highlight outline or semi-transparent preview
- [ ] Add FPS counter
- [ ] Create a simple hotbar to switch block types

---

## ✅ OPTIONAL FUTURE FEATURES

- [ ] World saving/loading to file (simple binary or JSON)
- [ ] Inventory system
- [ ] Day/night cycle
- [ ] Shadows and water rendering
- [ ] Multiplayer with simple client/server
- [ ] Entities (e.g., mobs, animals)
