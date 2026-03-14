// this is common reflections for z1engine
// just include this file in the front of @reflections scope
// like this:
// @reflections: {
//   #include <include/reflections.glsl>
///  ...
// }

	u_model          [invisible] // mat4, set by the engine

	u_shadow_map     [invisible] // sampler2DArray, set by the engine

	u_alpha_cutoff   = float 0.5
	u_alpha_mode     = int 0 // 0: opaque, 1: mask, 2: blend
