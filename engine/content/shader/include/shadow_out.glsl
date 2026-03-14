// Shadow pass fragment output
// Depth-only pass with alpha mask discard support
// Include this in the fragment stage of a surface shader inside #ifdef VARIANT_SHADOW

void main() {
	// For masked materials, discard fragments below alpha cutoff
	if (u_alpha_mode == 1) { // Mask
		float alpha = texture(s_base_color, v_texcoord0).a;
		if (alpha < u_alpha_cutoff) {
			discard;
		}
	}
	// No color output -- depth-only pass
}
