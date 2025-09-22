@uniforms: {
    #version 460 core

    layout(location = 0) uniform mat4 u_projview;
    layout(location = 1) uniform mat4 u_model;
    layout(location = 2) uniform vec3 u_sun_direction;
    layout(location = 3) uniform vec3 u_sun_intensity;
    layout(location = 4) uniform vec3 u_cam_position;
}
@reflections: {
    u_projview [invisible]              // mat4, set by the engine
    u_model [invisible]                 // mat4, set by the engine
    u_sun_direction [invisible]         // vec3, set by the engine
    u_sun_intensity [invisible]         // vec3, set by the engine
    u_cam_position [invisible]          // vec3, set by the engine
}
@stage: vert {
#include <common/vert.glsl>
}
@stage: frag {
#include <common/frag_attrs.glsl>
#include <common/phone.glsl>

    void main() {
        frag_color = phone_shading(normalize(v_normal), v_world_position, v_color);
    }
}