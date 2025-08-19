// this is a common fragment shader attributes for z1engine
// just include this file in the front of @stage: frag scope
// like this:
// @stage: frag {
//   #include <common/frag_attrs.glsl>
///  ...
// }

layout(location = 0) out vec4 frag_color;

layout(location = 0) in vec3 v_world_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec2 v_texcoord0;
layout(location = 3) in vec2 v_texcoord1;
layout(location = 4) in vec4 v_color;
