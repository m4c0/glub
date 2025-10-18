#pragma leco test

import dotz;
import glub;
import jojo;
import jute;
import print;

static auto & accessor(const glub::t & t, int n) {
  if (n < 0 || n >= t.accessors.size()) die("accessor index out-of-bounds");
  return t.accessors[n];
}
static auto & buffer_view(const glub::t & t, int n) {
  if (n < 0 || n >= t.buffer_views.size()) die("buffer views index out-of-bounds");
  return t.buffer_views[n];
}

int main() {
  auto src = jojo::read("models/BoxAnimated.glb");
  const auto t = glub::parse(src.begin(), src.size());

  for (auto & m : t.meshes) {
    for (auto & p : m.primitives) {
      if (p.mode != glub::primitive_mode::triangles) die("unsupported primitive mode");

      for (auto &[key, a] : p.attributes) {
        if (key != "POSITION") continue;

        auto & acc = accessor(t, a);
        if (acc.component_type != glub::accessor_comp_type::flt) die("unsupported accessor component type");
        if (acc.normalised) die("normalised accessors are not supported");
        if (acc.type != "VEC3") die("unsupported accessor type");

        auto & bv = buffer_view(t, acc.buffer_view);
        if (bv.buffer != 0) die("invalid buffer id");
        if (bv.target != glub::buffer_view_type::array_buffer) die("invalid buffer view type");

        auto ptr = reinterpret_cast<const dotz::vec3 *>(t.data.begin() + acc.byte_offset + bv.byte_offset);
        if ((void *)(ptr + acc.count) > t.data.end()) die("buffer overrun");

        putan(acc.count, acc.byte_offset, bv.byte_offset);
      }
    }
  }
}
