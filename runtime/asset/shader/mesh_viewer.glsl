@uniforms: {
    #version 460 core

    layout(location = 0) uniform mat4 u_projview;
    layout(location = 1) uniform mat4 u_model;
    layout(location = 2) uniform vec3 u_sun_direction;
    layout(location = 3) uniform vec3 u_sun_intensity;
    layout(location = 4) uniform vec3 u_cam_position;
}
@reflections: {
    u_projview [invisible] // mat4, set by the engine
    u_model [invisible] // mat4, set by the engine
    u_sun_direction = vec3 0.0 0.0 -1.0 // Direction of the sun
    u_sun_intensity = vec3 1.0 1.0 1.0 // Intensity of the sun light
    u_cam_position [invisible] // vec3, set by the engine
}
@stage: vert {
    layout(location = 0) in vec3 a_pos;
    layout(location = 1) in vec3 a_normal;
    layout(location = 2) in vec2 a_tex_coord;
    layout(location = 3) in vec4 a_color;

    layout(location = 0) out vec3 v_world_pos;
    layout(location = 1) out vec3 v_normal;
    layout(location = 2) out vec2 v_tex_coord;
    layout(location = 3) out vec4 v_color;

    void main() {
#include <common/vert.glsl>
    }
}
@stage: frag {
    layout(location = 0) out vec4 frag_color;

    layout(location = 0) in vec3 v_world_pos;
    layout(location = 1) in vec3 v_normal;
    layout(location = 2) in vec2 v_tex_coord;
    layout(location = 3) in vec4 v_color;

#include <common/phone.glsl>

    void main() {
        frag_color = phone_shading(normalize(v_normal), v_world_pos, v_color);
    }
}