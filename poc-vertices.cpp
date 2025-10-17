#pragma leco test

import glub;
import jojo;
import jute;
import print;

enum class access_type : int {
  indice,
  vec3,
};

static void buffer_view(const glub::t & t, int n, int offset, int size, glub::buffer_view_type type) {
  if (n < 0 || n >= t.buffer_views.size()) die("buffer view index out-of-bounds");
  auto & bv = t.buffer_views[n];

  if (bv.target != type) die("invalid buffer view type");

  putan(bv.buffer, bv.byte_offset + offset, bv.byte_stride, size);
}

static void access(const glub::t & t, int n, access_type type) {
  if (n < 0 || n >= t.accessors.size()) die("accessor index out-of-bounds");
  auto & acc = t.accessors[n];

  if (acc.normalised) die("accessor with normalised data is not supported");

  switch (type) {
    using enum access_type;
    case indice:
      if (acc.type != "SCALAR") die("invalid accessor type for indices");
      switch (acc.component_type) {
        using enum glub::accessor_comp_type;
        case unsigned_shrt:
          buffer_view(t, acc.buffer_view, acc.byte_offset, acc.count * 2, glub::buffer_view_type::element_array_buffer);
          break;
        case unsigned_int:
          buffer_view(t, acc.buffer_view, acc.byte_offset, acc.count * 4, glub::buffer_view_type::element_array_buffer);
          break;
        default: die("invalid accessor component type for indices");
      }
      break;
    case vec3:
      if (acc.type != "VEC3") die("invalid accessor type for vec3");
      if (acc.component_type != glub::accessor_comp_type::flt)
        die("invalid accessor component type for vec3");

      buffer_view(t, acc.buffer_view, acc.byte_offset, acc.count * 4 * 3, glub::buffer_view_type::array_buffer);
      break;
  }
}

static void mesh(const glub::t & t, int n) {
  if (n < 0 || n >= t.meshes.size()) die("mesh index out-of-bounds");
  auto & mesh = t.meshes[n];

  for (auto & p : mesh.primitives) {
    if (p.mode != glub::primitive_mode::triangles) die("unsupported primitive type");

    for (auto &[k, v]: p.attributes) {
      if (k == "POSITION") access(t, v, access_type::vec3);
    }
    if (p.indices >= 0) access(t, p.indices, access_type::indice);
  }
}
static void recurse(const glub::t & t, int n) {
  if (n < 0 || n >= t.nodes.size()) die("node index out-of-bounds");
  auto & node = t.nodes[n];

  if (node.mesh >= 0) mesh(t, node.mesh);

  for (auto child : node.children) {
    recurse(t, child);
  }
}

int main() {
  auto src = jojo::read("models/BoxAnimated.glb");
  const auto t = glub::parse(src.begin(), src.size());

  if (t.scene < 0) die("missing main scene");
  if (t.scene >= t.scenes.size()) die("main scene index out-of-bounds");

  for (auto n : t.scenes[t.scene].nodes) {
    recurse(t, n);
  }
}
