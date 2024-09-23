@uniforms: {
    #version 460 core

    layout(location = 0) uniform mat4 u_projview;
    layout(location = 1) uniform mat4 u_model;
    layout(location = 2) uniform vec4 u_color;
    layout(location = 3) uniform sampler2D u_texture;
    layout(location = 4) uniform vec4 u_tiling_factor;
}
@stage: vert {
    layout(location = 0) in vec3 a_pos;
    layout(location = 1) in vec2 a_uv;

    layout(location = 0) out vec2 v_uv;

    void main() {
        gl_Position = u_projview * u_model * vec4(a_pos, 1.0);
        v_uv = a_uv * u_tiling_factor.xy + u_tiling_factor.zw;
    }
}
@stage: frag {
    layout(location = 0) out vec4 frag_color;

    layout(location = 0) in vec2 v_uv;

    void main() {
        frag_color = u_color * texture(u_texture, v_uv);
    }
}