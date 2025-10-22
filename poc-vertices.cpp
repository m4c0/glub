#pragma leco tool

import dotz;
import glub;
import jojo;
import jute;
import print;

template<typename T>
static auto cast(auto & acc, auto & t) {
  auto & bv = t.buffer_views[acc.buffer_view];
  auto ptr = reinterpret_cast<const T *>(t.data.begin() + acc.byte_offset + bv.byte_offset);
  if ((void *)(ptr + acc.count) > t.data.end()) die("buffer overrun");
  return ptr;
}

int main() {
  auto src = jojo::read("models/BoxAnimated.glb");
  const auto t = glub::parse(src.begin(), src.size());

  for (auto & m : t.meshes) {
    for (auto & p : m.primitives) {
      put(t.accessors[p.accessors.position].count);
      put(' ');
      putln(t.accessors[p.indices].count);
    }
  }
  for (auto & m : t.meshes) {
    for (auto & p : m.primitives) {
      auto & a = p.accessors;
      if (a.position >= 0) {
        auto & acc = t.accessors[a.position];
        auto p = cast<dotz::vec3>(acc, t);
        for (auto i = 0; i < acc.count; i++) {
        }
      }
      if (a.normal >= 0) {
        auto & acc = t.accessors[a.normal];
        auto p = cast<dotz::vec3>(acc, t);
        for (auto i = 0; i < acc.count; i++) {
        }
      }
      if (a.texcoord_0 >= 0) {
        auto & acc = t.accessors[a.texcoord_0];
        auto p = cast<dotz::vec2>(acc, t);
        for (auto i = 0; i < acc.count; i++) {
        }
      }
    }
  }
}
