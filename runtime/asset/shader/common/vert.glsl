// this is a common vertex shader for z1engine
// just include this file in the vertex shader main() function
// like this:
// #include <common/vert.glsl>

vec3 world_pos = (u_model * vec4(a_pos, 1.0)).xyz;
gl_Position = u_projview * vec4(world_pos, 1.0);
v_world_pos = world_pos;
v_normal = mat3(transpose(inverse(u_model))) * a_normal;
v_tex_coord = a_tex_coord;
v_color = a_color;