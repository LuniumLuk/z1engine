// this is common reflections for z1engine
// just include this file in the front of @reflections scope
// like this:
// @reflections: {
//   #include <common/reflections.glsl>
///  ...
// }

// u_projview      [invisible] // mat4, set by the engine
u_model         [invisible] // mat4, set by the engine
// u_sun_direction [invisible] // vec3, set by the engine
// u_sun_intensity [invisible] // vec3, set by the engine
// u_cam_position  [invisible] // vec3, set by the engine
u_shadow_map     [invisible] // sampler2DArray, set by the engine
