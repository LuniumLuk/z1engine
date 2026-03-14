# Sample Sprite Prefabs

This directory contains example sprite prefabs that demonstrate how to use the SpriteComponent in z1engine scenes.

## Available Prefabs

### SampleSprite.prefab.yaml
A basic white sprite at the origin (0, 0, 0) with default scale of 1x1.

**Features:**
- Uses the white texture (T_white)
- Default white color [1, 1, 1, 1]
- Standard scale 1:1
- Perfect for learning and quick prototyping

**Usage:** Drag into a scene as a starting template.

### ColoredSprite.prefab.yaml
A 2x2 scaled sprite with an orange-brown color to demonstrate color tinting.

**Features:**
- Uses the white texture (T_white)
- Orange-brown color [1, 0.5, 0.2, 1]
- 2x2 world scale
- Shows how color blending works with textures

**Usage:** Good example for understanding color tinting and scale manipulation.

### MagentaSprite.prefab.yaml
A 1.5x1.5 scaled magenta-colored sprite using the magenta test texture.

**Features:**
- Uses the magenta texture (T_magenta)
- Pure magenta color [1, 0, 1, 1]
- 1.5x1.5 world scale
- Demonstrates texture switching

**Usage:** Example of loading different texture assets.

## SpriteComponent Fields Reference

| Field | Type | Purpose |
|-------|------|---------|
| `color` | `[R, G, B, A]` | Color tint applied to the sprite (0-1 per channel) |
| `texture` | `guid` | Asset path or GUID of the texture to display |
| `tiling_scale` | `[X, Y]` | UV tiling multiplier (1, 1) = full texture once |
| `tiling_offset` | `[X, Y]` | UV offset applied before rendering |
| `texcoords` | Array of 4 UV pairs | Quad corners in UV space (for sprite atlases) |

## Creating Custom Sprites

1. Copy one of these prefabs as a template
2. Modify the sprite section:
   - Change `color` to your desired RGBA values
   - Set `texture` to point to your texture asset
   - Adjust `tiling_scale` and `tiling_offset` if needed
3. Adjust the `transform` section (location, rotation, scale)
4. Give the entity a descriptive name
5. Update the `meta.guid` to a new unique identifier
6. Update the `meta.path` to match your prefab filename

## Example: Creating a Blue Sprite

```yaml
entities:
  - name: Blue Sprite
    id: 1
    transform:
      location: [0, 0, 0]
      rotation: [0, 0, 0]
      scale: [1, 1, 1]
    sprite:
      color: [0, 0.5, 1, 1]
      texture: texture/T_white
      tiling_scale: [1, 1]
      tiling_offset: [0, 0]
      texcoords:
        - [0, 0]
        - [1, 0]
        - [1, 1]
        - [0, 1]
```

## Notes

- All prefabs use the engine's built-in test textures (T_white, T_magenta)
- Sprites are 2D quads aligned with the XY plane (Z forward)
- Color values use linear color space [0, 1] range
- The 4 texcoords define the quad corners in UV space (useful for sprite atlases)
- Rotation in transform affects the sprite's orientation in world space
- Scale in transform affects the sprite's size in world units
