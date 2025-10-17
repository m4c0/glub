#pragma leco test

import jason;
import jojo;
import jute;
import hai;
import print;
import traits;

using namespace traits::ints;

struct error {
  const char * what;
};

struct header {
  uint32_t magic;
  uint32_t version;
  uint32_t length;
};
static_assert(sizeof(header) == 12);

struct chunk {
  uint32_t length;
  uint32_t type;
};

struct attribute {
  jute::heap key {};
  int accessor = -1;
};
struct primitive {
  hai::array<attribute> attributes;
  int indices = -1;
  int material = -1;
  int mode = 4;
  hai::array<jute::heap> targets {};
};
struct mesh {
  jute::heap name {};
  hai::array<primitive> primitives {};
  hai::array<float> weights {};
};
struct node {
  jute::heap name {};
  int camera = -1;
  int mesh = -1;
  int skin = -1;
  hai::array<int> children {};
  hai::array<float> matrix {};
  hai::array<float> rotation {};
  hai::array<float> scale {};
  hai::array<float> translation {};
  hai::array<float> weights {};
};
struct scene {
  jute::heap name {};
  hai::array<int> nodes {};
};
struct t {
  int scene = -1;
  hai::array<::mesh> meshes {};
  hai::array<::node> nodes {};
  hai::array<::scene> scenes {};
};

int main() try {
  auto raw = jojo::read("example.glb");

  header * hdr = reinterpret_cast<header *>(raw.begin());
  if (hdr->magic != 'FTlg') throw error { "invalid magic" };
  if (hdr->version != 2) throw error { "invalid version" };
  if (hdr->length != raw.size()) throw error { "invalid length" };

  chunk * json = reinterpret_cast<chunk *>(hdr + 1);
  if (json->type != 'NOSJ') throw error { "invalid chunk 0 type - expecting JSON" };

  char * json_data = reinterpret_cast<char *>(json + 1);

  chunk * bin = reinterpret_cast<chunk *>(json_data + json->length);
  if (reinterpret_cast<char *>(bin) > raw.end()) throw error { "json goes past file end" };
  if (bin->type != '\0NIB') throw error { "invalid chunk 1 type - expecting BIN" };

  char * bin_data = reinterpret_cast<char *>(bin + 1);
  if (bin_data + bin->length > raw.end()) throw error { "data goes past file end" };
  
  auto meta = jason::parse(jute::view { json_data, json->length });
  auto data = jute::view { bin_data, bin->length };
  
  using namespace jason::ast::nodes;
  auto & root = cast<dict>(meta);

  t t {};

  const auto parse_floats = [&](auto & n, jute::view key, auto & lst) {
    if (n.has_key(key)) {
      auto & list = cast<array>(n[key]);
      lst.set_capacity(list.size());
      for (auto j = 0; j < list.size(); j++) {
        lst[j] = cast<number>(list[j]).real();
      }
    }
  };
  const auto parse_ints = [&](auto & n, jute::view key, auto & lst) {
    if (n.has_key(key)) {
      auto & list = cast<array>(n[key]);
      lst.set_capacity(list.size());
      for (auto j = 0; j < list.size(); j++) {
        lst[j] = cast<number>(list[j]).integer();
      }
    }
  };
  const auto parse_int = [&](auto & n, jute::view key, auto & v) {
    if (n.has_key(key)) v = cast<number>(n[key]).integer();
  };
  const auto parse_string = [&](auto & n, jute::view key, auto & v) {
    if (n.has_key(key)) v = cast<string>(n[key]).str();
  };

  if (root.has_key("meshes")) {
    auto & meshes = cast<array>(root["meshes"]);
    t.meshes.set_capacity(meshes.size());
    for (auto i = 0; i < meshes.size(); i++) {
      auto & n = cast<dict>(meshes[i]);
      parse_floats(n, "weights", t.meshes[i].weights);
      parse_string(n, "name",    t.meshes[i].name);

      auto & prims = cast<array>(n["primitives"]);
      t.meshes[i].primitives.set_capacity(prims.size());
      for (auto j = 0; j < t.meshes[i].primitives.size(); j++) {
        auto & prim = cast<dict>(prims[j]);

        auto & attrs = cast<dict>(prim["attributes"]);
        t.meshes[i].primitives[j].attributes.set_capacity(attrs.size());
        auto ptr = t.meshes[i].primitives[j].attributes.begin();
        for (auto &[k, v] : attrs) *ptr++ = { k, cast<number>(v).integer() };

        parse_int(prim, "indices",  t.meshes[i].primitives[j].indices);
        parse_int(prim, "material", t.meshes[i].primitives[j].material);
        parse_int(prim, "mode",     t.meshes[i].primitives[j].mode);

        if (prim.has_key("targets")) throw error { "unsupported: mesh.primitive.targets" };
      }
    }
  }
  if (root.has_key("nodes")) {
    auto & nodes = cast<array>(root["nodes"]);
    t.nodes.set_capacity(nodes.size());
    for (auto i = 0; i < nodes.size(); i++) {
      auto & n = cast<dict>(nodes[i]);
      parse_int   (n, "camera",      t.nodes[i].camera);
      parse_ints  (n, "children",    t.nodes[i].children);
      parse_int   (n, "skin",        t.nodes[i].skin);
      parse_floats(n, "matrix",      t.nodes[i].matrix);
      parse_int   (n, "mesh",        t.nodes[i].mesh);
      parse_floats(n, "rotation",    t.nodes[i].rotation);
      parse_floats(n, "scale",       t.nodes[i].scale);
      parse_floats(n, "translation", t.nodes[i].translation);
      parse_floats(n, "weights",     t.nodes[i].weights);
      parse_string(n, "name",        t.nodes[i].name);

      if (!t.nodes[i].matrix.size())      t.nodes[i].matrix      = hai::array<float>::make(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
      if (!t.nodes[i].rotation.size())    t.nodes[i].rotation    = hai::array<float>::make(0, 0, 0, 1);
      if (!t.nodes[i].scale.size())       t.nodes[i].scale       = hai::array<float>::make(1, 1, 1);
      if (!t.nodes[i].translation.size()) t.nodes[i].translation = hai::array<float>::make(0, 0, 0);
    }
  }
  if (root.has_key("scene")) t.scene = cast<number>(root["scene"]).integer();
  if (root.has_key("scenes")) {
    auto & scenes = cast<array>(root["scenes"]);
    t.scenes.set_capacity(scenes.size());
    for (auto i = 0; i < scenes.size(); i++) {
      auto & s = cast<dict>(scenes[i]);
      parse_string(s, "name", t.scenes[i].name);
      parse_ints(s, "nodes", t.scenes[i].nodes);
    }
  }

  const auto list = [](auto & ls) {
    for (auto l : ls) put(l, " ");
    putln();
  };

  putln("meshes:");
  for (auto & s : t.meshes) {
    putln("- ", s.name);
    put("  weights: "); list(s.weights);
    putln("  primitives:");
    for (auto & r : s.primitives) {
      putln("  - ind:", r.indices, " mat:", r.material, " mode:", r.mode);
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
  putln();

  for (auto &[k, v] : root) {
    putln(k);
  }
} catch (const jason::error & e) {
  errln(e.what);
  return 1;
} catch (const error & e) {
  errln(e.what);
  return 1;
}
