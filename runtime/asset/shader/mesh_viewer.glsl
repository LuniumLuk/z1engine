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
        vec3 world_pos = (u_model * vec4(a_pos, 1.0)).xyz;
        gl_Position = u_projview * vec4(world_pos, 1.0);
        v_world_pos = world_pos;
        v_normal = mat3(transpose(inverse(u_model))) * a_normal;
        v_tex_coord = a_tex_coord;
        v_color = a_color;
    }
}
@stage: frag {
    layout(location = 0) out vec4 frag_color;

    layout(location = 0) in vec3 v_world_pos;
    layout(location = 1) in vec3 v_normal;
    layout(location = 2) in vec2 v_tex_coord;
    layout(location = 3) in vec4 v_color;

    void main() {
        // Normalize the normal vector to get correct per-fragment lighting
        vec3 normal = normalize(v_normal);

        // Ambient lighting
        float ambient_strength = 0.1;
        vec3 ambient = ambient_strength * u_sun_intensity;

        // Diffuse lighting
        vec3 sun_dir = normalize(u_sun_direction);
        float diff = max(dot(normal, sun_dir), 0.0);
        vec3 diffuse = diff * u_sun_intensity;

        // Specular lighting
        float specular_strength = 0.5;
        vec3 view_dir = normalize(u_cam_position - v_world_pos);
        vec3 refl_dir = reflect(-sun_dir, normal);
        float spec = pow(max(dot(view_dir, refl_dir), 0.0), 32);
        vec3 specular = specular_strength * spec * u_sun_intensity;

        vec3 result = (ambient + diffuse + specular) * v_color.rgb;
        frag_color = vec4(result, v_color.a);
    }
}