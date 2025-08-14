@uniforms: {
    #version 460 core

    layout(location = 0) uniform mat4 u_projview;
    layout(location = 1) uniform mat4 u_model;
    layout(location = 2) uniform float u_object_id;
}
@reflections: {
    u_projview [invisible]     // mat4, set by the engine
    u_model [invisible]        // mat4, set by the engine
}
@stage: vert {
    layout(location = 0) in vec3 a_pos;
    layout(location = 1) in vec3 a_normal;
    layout(location = 2) in vec2 a_tex_coord;
    layout(location = 3) in vec4 a_color;

    void main() {
        gl_Position = u_projview * u_model * vec4(a_pos, 1.0);
    }
}
@stage: frag {
    layout(location = 0) out vec4 frag_color;

    vec4 pack_uint_to_rgba8(uint v) {
        return vec4(
            float((v >>  0) & 0xFFu) / 255.0,
            float((v >>  8) & 0xFFu) / 255.0,
            float((v >> 16) & 0xFFu) / 255.0,
            float((v >> 24) & 0xFFu) / 255.0
        );
    }

    void main() {
        frag_color = pack_uint_to_rgba8(uint(u_object_id));
    }
}