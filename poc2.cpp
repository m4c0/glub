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

enum class accessor_comp_type : int {
  nil = -1,
  byte = 5120,
  unsigned_byte = 5121,
  shrt = 5122,
  unsigned_shrt = 5123,
  unsigned_int = 5125,
  flt = 5126,
};
enum class buffer_view_type : int {
  nil = -1,
  array_buffer = 34962,
  element_array_buffer = 34963,
};
enum class primitive_mode : int {
  points = 0,
  lines = 1,
  line_loop = 2,
  line_strip = 3,
  triangles = 4,
  triangle_strip = 5,
  triangle_fan = 6,
};

struct accessor {
  int buffer_view = -1;
  int byte_offset = 0;
  accessor_comp_type component_type;
  bool normalised = false;
  int count;
  jute::heap type;
  hai::array<float> max {};
  hai::array<float> min {};
  jute::heap name {};
};
struct attribute {
  jute::heap key {};
  int accessor = -1;
};
struct buffer {
  jute::heap uri {};
  int byte_length;
  jute::heap name {};
};
struct buffer_view {
  int buffer;
  int byte_offset = 0;
  int byte_length;
  int byte_stride = -1;
  buffer_view_type target = buffer_view_type::nil;
  jute::heap name {};
};
struct primitive {
  hai::array<attribute> attributes;
  int indices = -1;
  int material = -1;
  primitive_mode mode = primitive_mode::triangles;
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
  hai::array<accessor> accessors {};
  hai::array<buffer> buffers {};
  hai::array<buffer_view> buffer_views {};
  hai::array<mesh> meshes {};
  hai::array<node> nodes {};
  hai::array<scene> scenes {};
  int scene = -1;
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

  const auto parse_bool = [&](auto & n, jute::view key, auto & v) {
    if (n.has_key(key)) v = cast<boolean>(n[key]);
  };
  const auto parse_enum = [&](auto & n, jute::view key, auto & v) {
    // TODO: how to validate in a generic way?
    using T = traits::decay<decltype(v)>::type;
    if (n.has_key(key)) v = static_cast<T>(cast<number>(n[key]).integer());
  };
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
  const auto iter = [&](auto & n, jute::view key, auto & dst, auto && fn) {
    auto & src_list = cast<array>(n[key]);
    dst.set_capacity(src_list.size());
    for (auto i = 0; i < src_list.size(); i++) {
      fn(cast<dict>(src_list[i]), dst[i]);
    }
  };

  if (root.has_key("accessors")) {
    iter(root, "accessors", t.accessors, [&](auto & n, auto & o) {
      parse_int(n, "bufferView", o.buffer_view);
      parse_int(n, "byteOffset", o.byte_offset);
      o.component_type = accessor_comp_type { cast<number>(n["componentType"]).integer() };
      parse_bool(n, "normalized", o.normalised);
      o.count = cast<number>(n["count"]).integer();
      o.type = cast<string>(n["type"]).str();
      parse_floats(n, "max", o.max);
      parse_floats(n, "min", o.min);
      parse_string(n, "name", o.name);

      if (n.has_key("sparse")) throw error { "unsupported: accessor.sparse" };
    });
  }
  if (root.has_key("buffers")) {
    iter(root, "buffers", t.buffers, [&](auto & n, auto & o) {
      parse_string(n, "uri", o.uri);
      o.byte_length = cast<number>(n["byteLength"]).integer();
      parse_string(n, "name", o.name);
    });
  }
  if (root.has_key("bufferViews")) {
    iter(root, "bufferViews", t.buffer_views, [&](auto & n, auto & o) {
      o.buffer = cast<number>(n["buffer"]).integer();
      parse_int(n, "byteOffset", o.byte_offset);
      o.byte_length = cast<number>(n["byteLength"]).integer();
      parse_int(n, "byteStride", o.byte_stride);
      parse_enum(n, "target", o.target);
      parse_string(n, "name", o.name);
    });
  }
  if (root.has_key("meshes")) {
    iter(root, "meshes", t.meshes, [&](auto & n, auto & o) {
      parse_floats(n, "weights", o.weights);
      parse_string(n, "name",    o.name);

      iter(n, "primitives", o.primitives, [&](auto & prim, auto & p) {
        auto & attrs = cast<dict>(prim["attributes"]);
        p.attributes.set_capacity(attrs.size());
        auto ptr = p.attributes.begin();
        for (auto &[k, v] : attrs) *ptr++ = { k, cast<number>(v).integer() };

        parse_int (prim, "indices",  p.indices);
        parse_int (prim, "material", p.material);
        parse_enum(prim, "mode",     p.mode);

        if (prim.has_key("targets")) throw error { "unsupported: mesh.primitive.targets" };
      });
    });
  }
  if (root.has_key("nodes")) {
    iter(root, "nodes", t.nodes, [&](auto & n, auto & o) {
      parse_int   (n, "camera",      o.camera);
      parse_ints  (n, "children",    o.children);
      parse_int   (n, "skin",        o.skin);
      parse_floats(n, "matrix",      o.matrix);
      parse_int   (n, "mesh",        o.mesh);
      parse_floats(n, "rotation",    o.rotation);
      parse_floats(n, "scale",       o.scale);
      parse_floats(n, "translation", o.translation);
      parse_floats(n, "weights",     o.weights);
      parse_string(n, "name",        o.name);

      if (!o.matrix.size())      o.matrix      = hai::array<float>::make(1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1);
      if (!o.rotation.size())    o.rotation    = hai::array<float>::make(0, 0, 0, 1);
      if (!o.scale.size())       o.scale       = hai::array<float>::make(1, 1, 1);
      if (!o.translation.size()) o.translation = hai::array<float>::make(0, 0, 0);
    });
  }
  if (root.has_key("scene")) t.scene = cast<number>(root["scene"]).integer();
  if (root.has_key("scenes")) {
    iter(root, "scenes", t.scenes, [&](auto & n, auto & o) {
      parse_string(n, "name",  o.name);
      parse_ints  (n, "nodes", o.nodes);
    });
  }

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
