@uniforms: {
    #version 460 core

    layout(location = 0) uniform mat4 u_projview;
    layout(location = 1) uniform mat4 u_model;
    layout(location = 2) uniform sampler2D s_base_color;
}
@reflections: {
    u_projview [invisible]              // mat4, set by the engine
    u_model [invisible]                 // mat4, set by the engine
    s_base_color = sampler2D            // sampler2D, base color
}
@stage: vert {
#include <common/vert.glsl>
}
@stage: frag {
#include <common/frag_attrs.glsl>

    void main() {
        frag_color = texture(s_base_color, v_texcoord0);
    }
}