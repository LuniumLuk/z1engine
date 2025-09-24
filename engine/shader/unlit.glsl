@uniforms: {
    #include <common/uniforms.glsl>

    uniform sampler2D s_base_color;
}
@reflections: {
    #include <common/reflections.glsl>

    s_base_color = sampler2D texture/T_white
}
@stage: vert {
    #include <common/vert.glsl>
}
@stage: frag {
    #include <common/frag_attrs.glsl>

    void main() {
        frag_color = v_color * texture(s_base_color, v_texcoord0);
    }
}