// Shadow pass fragment output for specular-glossiness materials
// Uses s_diffuse instead of s_base_color for alpha test

void main() {
	if (u_alpha_mode == 1) { // Mask
		float alpha = texture(s_diffuse, v_texcoord0).a;
		if (alpha < u_alpha_cutoff) {
			discard;
		}
	}
}
