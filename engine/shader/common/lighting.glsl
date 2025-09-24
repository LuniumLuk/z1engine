vec4 lambert_shading(vec3 normal, vec4 color) {
    vec3 sun_dir = normalize(u_sun_direction);
    float NoL = max(dot(normal, sun_dir), 0.0);
    return vec4(color.rgb * NoL, color.a);
}

vec4 phone_shading(vec3 normal, vec3 world_pos, vec4 color) {
    // Ambient lighting
    float ambient_strength = 0.1;
    vec3 ambient = ambient_strength * u_sun_intensity;

    // Diffuse lighting
    vec3 sun_dir = normalize(u_sun_direction);
    float diff = max(dot(normal, sun_dir), 0.0);
    vec3 diffuse = diff * u_sun_intensity;

    // Specular lighting
    float specular_strength = 0.5;
    vec3 view_dir = normalize(u_cam_position - world_pos);
    vec3 refl_dir = reflect(-sun_dir, normal);
    float spec = pow(max(dot(view_dir, refl_dir), 0.0), 32);
    vec3 specular = specular_strength * spec * u_sun_intensity;

    vec3 result = (ambient + diffuse + specular) * color.rgb;
    return vec4(result, color.a);
}