// this is a common quad/screen shader for z1engine
// just include this file in the @stage: vert scope
// like this:
// @stage: vert {
//   #include <common/quad.glsl>
// }

layout(location = 0) in vec2 a_position;

layout(location = 0) out vec2 v_uv;

void main() {
	gl_Position = vec4(a_position, 0.0, 1.0);
	v_uv = a_position * 0.5 + 0.5;
}
