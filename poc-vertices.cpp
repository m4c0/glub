#pragma leco tool

import dotz;
import glub;
import jojo;
import jute;
import print;

template<typename T>
static auto cast(auto & acc, auto & bv, auto & t) {
  auto ptr = reinterpret_cast<const T *>(t.data.begin() + acc.byte_offset + bv.byte_offset);
  if ((void *)(ptr + acc.count) > t.data.end()) die("buffer overrun");
  return ptr;
}

int main() {
  auto src = jojo::read("models/BoxAnimated.glb");
  const auto t = glub::parse(src.begin(), src.size());

  for (auto & m : t.meshes) {
    for (auto & p : m.primitives) {

    }
  }
}
