@uniforms: {
    #version 460 core

    layout(location = 0) uniform mat4 u_projview;
    layout(location = 1) uniform mat4 u_model;
    layout(location = 2) uniform vec4 u_color;
    layout(location = 3) uniform sampler2D u_texture;
    layout(location = 4) uniform vec4 u_tiling_factor;
}
@reflections: {
    u_projview [invisible]                 // mat4, set by the engine
    u_model [invisible]                    // mat4, set by the engine
    u_color = vec4 1.0 1.0 1.0 1.0         // RGBA
    u_tiling_factor = vec4 1.0 1.0 0.0 0.0 // x, y, offset_x, offset_y
}
@stage: vert {
    layout(location = 0) in vec3 a_position;
    layout(location = 1) in vec2 a_texcoord;

    layout(location = 0) out vec2 v_texcoord;

    void main() {
        gl_Position = u_projview * u_model * vec4(a_position, 1.0);
        v_texcoord = a_texcoord * u_tiling_factor.xy + u_tiling_factor.zw;
    }
}
@stage: frag {
    layout(location = 0) out vec4 frag_color;

    layout(location = 0) in vec2 v_texcoord;

    void main() {
        frag_color = u_color * texture(u_texture, v_texcoord);
    }
}