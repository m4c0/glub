#version 450
#extension GL_GOOGLE_include_directive : require
#include "../glslinc/3d.glsl"

layout(location = 0) in vec3 pos;

const float fov_rad = radians(90);
const float far = 100.0;
const float near = 0.01;

const vec3 cam = vec3(0, 0, 5);
const vec3 up = vec3(0, 1, 0);
const float aspect = 1.6;

void main() {
  mat4 proj = projection_matrix(fov_rad, aspect, near, far);
  mat4 view = view_matrix(cam, radians(180), up);

  vec4 pvec = vec4(pos.x, -pos.y, pos.z, 1);
  gl_Position = pvec * view * proj;
}
