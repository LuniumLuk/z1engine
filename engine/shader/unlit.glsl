@uniforms: {
    #version 460 core

    layout(location = 0) uniform mat4 u_projview;
    layout(location = 1) uniform mat4 u_model;

    layout(location = 2) uniform sampler2D u_texture;
}
@reflections: {
    u_projview [invisible]              // mat4, set by the engine
    u_model [invisible]                 // mat4, set by the engine
}
@stage: vert {
#include <common/vert.glsl>
}
@stage: frag {
#include <common/frag_attrs.glsl>

    void main() {
        frag_color = texture(u_texture, v_uv);
    }
}