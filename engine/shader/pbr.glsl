@uniforms: {
	#include <common/uniforms.glsl>

	uniform sampler2D s_base_color;
	uniform sampler2D s_metallic_roughness;
	uniform sampler2D s_normal;
	uniform sampler2D s_emissive;
	uniform sampler2D s_occlusion;

	uniform vec4 u_base_color_factor;
	uniform float u_roughness_factor;
	uniform float u_metallic_factor;
}
@reflections: {
	#include <common/reflections.glsl>

	s_base_color         = sampler2D texture/T_white
	s_metallic_roughness = sampler2D texture/T_white
	s_normal             = sampler2D texture/T_normal
	s_emissive           = sampler2D texture/T_black
	s_occlusion          = sampler2D texture/T_white

	u_base_color_factor  = vec4 1.0 1.0 1.0 1.0
	u_roughness_factor   = float 0.5
	u_metallic_factor    = float 0.5
}
@stage: vert {
	#include <common/vert.glsl>
}
@stage: frag {
	#include <common/frag_attrs.glsl>
	#include <common/lighting.glsl>

	void main() {

		// Inputs
		vec3 normal_map = texture(s_normal, v_texcoord0).rgb * 2.0 - 1.0;

		vec3 N = get_normal_from_map(v_world_position, normalize(v_normal), normalize(v_tangent), normal_map);
		vec3 V = normalize(u_cam_position.xyz - v_world_position);

		float shadow = get_shadow();

		// Base material inputs
		vec4 base_color_sample = texture(s_base_color, v_texcoord0);
		base_color_sample.rgb = pow(base_color_sample.rgb, vec3(2.2));
		vec4 base_color_vec = base_color_sample * v_color * u_base_color_factor;
		vec3 base_color = base_color_vec.rgb;
		float alpha = base_color_vec.a;

		vec2 rm = texture(s_metallic_roughness, v_texcoord0).gb;
		rm = pow(rm, vec2(2.2));
		float roughness = clamp(rm.x * u_roughness_factor, 0.04, 1.0);
		float metallic  = rm.y * u_metallic_factor;

		// Fresnel reflectance at normal incidence
		vec3 F0 = mix(vec3(0.04), base_color, metallic);

		vec3 L_diffuse = vec3(0.0);
		vec3 L_specular = vec3(0.0);

		int count = int(u_lights_count.x);
		for (int i = 0; i < count; ++i) {
			Light light = u_lights[i];
			vec3 light_dir;
			float attenuation = 1.0;

			// 0: Directional, 1: Point, 2: Spot
			int type = int(light.position.w);

			if (type == 0) { // Directional
				light_dir = normalize(-light.direction.xyz);
			}
			else { // Point or Spot
				vec3 dist_vec = light.position.xyz - v_world_position;
				float dist = length(dist_vec);
				if (dist > light.direction.w) continue; // Range check
				light_dir = normalize(dist_vec);

				// Linear falloff
				attenuation = max(0.0, 1.0 - dist / light.direction.w);
				attenuation *= attenuation;

				if (type == 2) { // Spot
					float theta = dot(light_dir, normalize(-light.direction.xyz));
					float inner = light.cone.x;
					float outer = light.cone.y;
					float epsilon = inner - outer;
					float intensity = clamp((theta - outer) / (epsilon + 1e-5), 0.0, 1.0);
					attenuation *= intensity;
				}
			}

			// Shadows
			float shadow_factor = 1.0;
			if (light.cone.z > 0.5) { // Cast shadows
				if (type == 0) {
					vec3 sun_dir = normalize(u_sun_direction.xyz);
					if (dot(light_dir, sun_dir) > 0.99) {
						shadow_factor = shadow;
					}
				}
			}

			calculate_pbr_illumination(light_dir, light.color.rgb * light.color.w, attenuation, shadow_factor,
				N, V, F0, roughness, metallic, base_color, L_diffuse, L_specular);
		}

		// Simple ambient term
		vec3 ambient = base_color * u_sun_ambient.rgb;

		// Emissive term
		vec3 emissive = texture(s_emissive, v_texcoord0).rgb;
		emissive = pow(emissive, vec3(2.2));

		// Occlusion term
		float ao = texture(s_occlusion, v_texcoord0).r;

		vec3 result = (ambient + L_diffuse) * ao + L_specular + emissive;
		frag_color = vec4(result, alpha);

		frag_color = vec4(
			isnan(frag_color.x) ? 0.0 : frag_color.x,
			isnan(frag_color.y) ? 0.0 : frag_color.y,
			isnan(frag_color.z) ? 0.0 : frag_color.z,
			isnan(frag_color.w) ? 0.0 : frag_color.w
		);
	}
}