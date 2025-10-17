#pragma leco test

import glub;
import jason;
import jojo;
import jute;
import hai;
import print;
import traits;

using namespace glub;
using namespace traits::ints;

int main() try {
  auto raw = jojo::read("example.glb");
  auto t = glub::parse(raw.begin(), raw.size());

  const auto list = [](auto & ls) {
    for (auto l : ls) put(l, " ");
    putln();
  };

  putln("accessors:");
  for (auto & s : t.accessors) {
    putln("- ", s.name, " typ:", s.type, " bv:", s.buffer_view, " ofs:", s.byte_offset,
        " ctyp:", static_cast<int>(s.component_type), " norm:", s.normalised, " count:", s.count);
    put("  min:"); for (auto & q : s.min) put(" ", q); putln();
    put("  max:"); for (auto & q : s.max) put(" ", q); putln();
  }
  putln("buffers:");
  for (auto & s : t.buffers) {
    putln("- ", s.name, " len:", s.byte_length, " uri:", s.uri);
  }
  putln("buffer views:");
  for (auto & s : t.buffer_views) {
    putln("- ", s.name, " buf:", s.buffer, " ofs:", s.byte_offset, " len:", s.byte_length,
        " stride:", s.byte_stride, " target:", static_cast<int>(s.target));
  }
  putln("meshes:");
  for (auto & s : t.meshes) {
    putln("- ", s.name);
    put("  weights: "); list(s.weights);
    putln("  primitives:");
    for (auto & r : s.primitives) {
      putln("  - ind:", r.indices, " mat:", r.material, " mode:", static_cast<int>(r.mode));
      putln("    attributes:");
      for (auto & q : r.attributes) putln("    - ", q.key, " = ", q.accessor);
      putln("    targets:");
      for (auto & q : r.targets) putln("    - ", q);
    }
  }
  putln("nodes:");
  for (auto & n : t.nodes) {
    put("- ", n.name, " ");
    putln("cam:", n.camera, " mesh:", n.mesh, " skin:", n.skin);
    put("  children: ");    list(n.children);
    put("  matrix: ");      list(n.matrix);
    put("  rotation: ");    list(n.rotation);
    put("  scale: ");       list(n.scale);
    put("  translation: "); list(n.translation);
    put("  weights: ");     list(n.weights);
  }
  putln("scene: ", t.scene);
  putln("scenes:");
  for (auto & s : t.scenes) {
    putln("- ", s.name);
    put("  nodes: "); list(s.nodes);
  }
  putln("skins:");
  for (auto & s : t.skins) {
    putln("- ", s.name, " ibm:", s.inverse_bind_mat, " skel:", s.skeleton);
    put("  joints:"); for (auto n : s.joints) put(" ", n); putln();
  }
  putln();
} catch (const jason::error & e) {
  errln(e.what);
  return 1;
} catch (const error & e) {
  errln(e.what);
  return 1;
}
