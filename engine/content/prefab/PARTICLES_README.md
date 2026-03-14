# Sample Particle System Prefabs

This directory contains example particle system prefabs demonstrating various effects using the ParticleComponent system in z1engine.

## Available Prefabs

### BasicParticles.prefab.yaml
A simple, minimal particle effect to start with.

**Features:**
- 100 max particles, 30 emission rate
- Simple upward motion with gravity
- 1-2 second lifetime
- White to transparent fade
- Alpha blend mode
- Point emitter shape
- No rotation
- Minimal spread

**Use Case:** Learning particle basics, placeholder effects, debugging

**Key Parameters:**
- `emission_rate: 30` - Slow steady emission
- `direction_spread: 0.2` - Tight, focused beam
- `gravity: [0, -5, 0]` - Gentle downward pull
- `blend_mode: 0` - Alpha blending (standard transparency)

---

### FountainParticles.prefab.yaml
A water fountain effect with upward arc and spread.

**Features:**
- 500 max particles, 100 emission rate
- Fast upward motion (3-5 units/sec)
- Blue color [0.2, 0.5, 1]
- Large spread for fountain arc
- Good rotation variation
- Lifetime 1.5-2.5 seconds
- Alpha blend with sorting
- Point emitter

**Use Case:** Fountains, water effects, geysers

**Key Parameters:**
- `emission_rate: 100` - Medium-high emission
- `initial_speed: [3, 5]` - Fast upward motion
- `direction_spread: 0.4` - Wide arc pattern
- `gravity: [0, -9.81, 0]` - Natural gravity
- `size_over_life: [1, 0.5]` - Particles shrink slightly
- `sort_by_depth: true` - Sort for proper transparency blending

---

### FireParticles.prefab.yaml
A flame/fire effect with upward velocity and color fade.

**Features:**
- 200 max particles, 80 emission rate
- Orange to red color [1, 0.7, 0 → 1, 0.2, 0]
- Upward rising motion (counter gravity)
- Sphere emitter for volume
- Short 0.8-1.5 sec lifetime
- Higher damping (0.3) for drag
- Additive blend mode (glowing effect)
- Rotating particles

**Use Case:** Fire, explosions, heat effects

**Key Parameters:**
- `emission_rate: 80` - Medium emission
- `gravity: [0, 2, 0]` - Upward acceleration (fire rises)
- `damping: 0.3` - Slower particle movement
- `blend_mode: 1` - Additive blending (bright/glowing)
- `emitter_shape: 1` - Sphere shape for volume
- `direction_spread: 0.6` - Wider spread
- `size_over_life: [1, 0.1]` - Shrinks as it rises

---

### SmokeParticles.prefab.yaml
A smoke/dust cloud effect that rises slowly and grows.

**Features:**
- 300 max particles, 60 emission rate
- Gray color [0.5, 0.5, 0.5]
- Slow gentle rise
- Large lifetime 2-4 seconds
- Sphere emitter for natural dispersal
- Low damping (0.05) for slow drift
- Size grows over life (expands as it rises)
- Alpha blend with sorting
- Subtle rotation

**Use Case:** Smoke, dust clouds, steam, fog effects

**Key Parameters:**
- `emission_rate: 60` - Steady emission
- `lifetime: [2, 4]` - Long lifetime
- `gravity: [0, 0.5, 0]` - Very gentle upward push
- `damping: 0.05` - Minimal drag
- `size_over_life: [0.5, 1.5]` - Grows over time (dispersal)
- `initial_size: [0.1, 0.2]` - Larger base size
- `direction_spread: 0.8` - Very wide spread
- `sort_by_depth: true` - Proper transparency

---

### SparkleParticles.prefab.yaml
A magical sparkle/glitter effect with fast particles.

**Features:**
- 150 max particles, 120 emission rate
- Cyan to light cyan [0, 1, 1 → 0.5, 1, 1]
- Fast outward velocity 2-6 units/sec
- Very small particles (0.03-0.06)
- Short lifetime 0.5-1.2 seconds
- Point emitter for tight origin
- Point-emitter for tight burst
- Very fast rotation 180-360 deg/sec
- Additive blend for bright sparkles
- Strong downward gravity pulls them down quickly

**Use Case:** Magic effects, sparkles, sweat particles, stars, glitter

**Key Parameters:**
- `emission_rate: 120` - High emission rate
- `initial_speed: [2, 6]` - Very fast particles
- `direction_spread: 0.9` - Nearly hemispherical spread
- `gravity: [0, -3, 0]` - Strong downward pull
- `blend_mode: 1` - Additive blending (bright)
- `initial_size: [0.03, 0.06]` - Very small
- `rotation_speed: [180, 360]` - Very fast spin
- `lifetime: [0.5, 1.2]` - Short burst

---

## ParticleComponent Configuration Reference

### Emission
| Field | Type | Purpose |
|-------|------|---------|
| `max_particles` | uint32 | Maximum particles alive at once (pool size) |
| `emission_rate` | float | Particles spawned per second |
| `burst_count` | uint32 | Particles spawned on emit_burst() call |
| `loop` | bool | If false, emitter stops after spawning max_particles total |
| `playing` | bool | Play/pause toggle |

### Particle Physics
| Field | Type | Purpose |
|-------|------|---------|
| `lifetime` | vec2 | Min/max particle lifetime in seconds (randomized) |
| `initial_speed` | vec2 | Min/max initial velocity magnitude |
| `direction` | vec3 | Emission direction (normalized) |
| `direction_spread` | float | Cone spread (0=focused, 1=hemisphere) |
| `gravity` | vec3 | Constant acceleration per frame |
| `damping` | float | Velocity damping per second (0=none, 1=stop) |
| `world_space` | bool | If true, particles simulate in world space |

### Particle Rendering
| Field | Type | Purpose |
|-------|------|---------|
| `initial_size` | vec2 | Min/max initial billboard size (world units) |
| `size_over_life` | vec2 | Size multiplier at birth and death (lerp) |
| `initial_color` | vec4 | Start color RGBA [R, G, B, A] |
| `end_color` | vec4 | End color at death (lerp) |
| `texture` | guid | Particle billboard texture (null = white) |
| `blend_mode` | enum | 0=Alpha, 1=Additive, 2=Soft |
| `sort_by_depth` | bool | Sort back-to-front each frame (for alpha) |
| `initial_rotation` | vec2 | Min/max initial rotation in degrees |
| `rotation_speed` | vec2 | Min/max angular velocity deg/sec |

### Emitter Shape
| Field | Type | Purpose |
|-------|------|---------|
| `emitter_shape` | enum | 0=Point, 1=Sphere, 2=Box, 3=Cone |
| `shape_radius` | float | Radius for Sphere/Cone shapes |
| `shape_extents` | vec3 | Half-extents for Box shape |

## Blend Modes Explained

- **Alpha (0)**: Standard transparency. Use `sort_by_depth: true` for correct ordering.
- **Additive (1)**: Bright glowing effect. Colors add together. No sorting needed.
- **Soft (2)**: Soft particles (depth fading at geometry intersection). Experimental.

## Emitter Shapes Explained

- **Point (0)**: Emit from a single point. Smallest memory, good for bursts.
- **Sphere (1)**: Emit from surface/interior of sphere. Good for volume effects.
- **Box (2)**: Emit from surface/interior of AABB. Good for area emissions.
- **Cone (3)**: Emit from cone base disk. Good for directional effects.

## Creating Custom Particle Effects

1. **Choose a base prefab** that's closest to your effect
2. **Adjust these first** (biggest visual impact):
   - `initial_color` and `end_color` - The colors
   - `emission_rate` - How dense the effect is
   - `initial_speed` and `direction` - Movement direction/speed
   - `gravity` - How particles fall/rise
3. **Refine with**:
   - `lifetime` - How long particles last
   - `initial_size` and `size_over_life` - Particle size animation
   - `damping` - Air resistance
   - `direction_spread` - Spread pattern
4. **Polish with**:
   - `initial_rotation` and `rotation_speed` - Visual interest
   - `blend_mode` - How particles blend with background
   - `sort_by_depth` - For proper alpha blending (costs performance)

## Tips & Tricks

### Performance
- Higher `max_particles` = more memory usage
- `sort_by_depth: true` adds CPU cost (sorts every frame)
- Additive blend is faster than alpha (no sorting needed)
- Use smaller textures or none (null = white square)

### Visual Quality
- Combine multiple emitters (fire + smoke separately)
- Use slight random color variation by tweaking min/max values
- Rotation makes small particles look less flat
- Alpha blend with sorting looks better but costs more

### Common Effects
- **Rain**: High emission, falling direction, low spread, thin particles, alpha blend
- **Explosion**: Short lifetime, high initial speed, sphere emitter, additive blend
- **Magic Trail**: Follow moving object, high rotation speed, cyan/blue color, additive
- **Dust**: Slow rise, sphere emitter, grows over life, brown colors, alpha blend with sort

## Notes

- All prefabs use no texture (white default) for maximum compatibility
- Particles render after all geometry (forward pass depth-ordered)
- No depth buffer writes (depth test enabled, depth write disabled)
- Spawns are CPU-side; rendering is GPU-driven
- One draw call per emitter per blend mode (v1)

## See Also

- `openspec/changes/2026-03-14-particle-system/proposal.md` - Full particle system design
- `engine/runtime/source/scene/component/particle.h` - ParticleComponent declaration
- `engine/runtime/source/scene/particle_system.cpp` - Simulation logic
