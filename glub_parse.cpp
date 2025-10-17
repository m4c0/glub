module glub;
import jason;
import jute;
import hai;
import traits;

namespace glub {
  struct header {
    unsigned magic;
    unsigned version;
    unsigned length;
  };
  static_assert(sizeof(header) == 12);

  struct chunk {
    unsigned length;
    unsigned type;
  };
}

glub::t glub::parse(const char * raw, unsigned size) {
  auto hdr = reinterpret_cast<const header *>(raw);
  if (hdr->magic != 'FTlg') throw error { "invalid magic" };
  if (hdr->version != 2) throw error { "invalid version" };
  if (hdr->length != size) throw error { "invalid length" };

  auto json = reinterpret_cast<const chunk *>(hdr + 1);
  if (json->type != 'NOSJ') throw error { "invalid chunk 0 type - expecting JSON" };

  auto json_data = reinterpret_cast<const char *>(json + 1);

  auto bin = reinterpret_cast<const chunk *>(json_data + json->length);
  if (reinterpret_cast<const char *>(bin) > raw + size) throw error { "json goes past file end" };
  if (bin->type != '\0NIB') throw error { "invalid chunk 1 type - expecting BIN" };

  auto bin_data = reinterpret_cast<const char *>(bin + 1);
  if (bin_data + bin->length > raw + size) throw error { "data goes past file end" };
  
  auto meta = jason::parse(jute::view { json_data, json->length });
  auto data = jute::view { bin_data, bin->length };
  
  using namespace jason::ast::nodes;
  auto & root = cast<dict>(meta);

  t t { .data = data };

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
    });
  }
  if (root.has_key("scene")) t.scene = cast<number>(root["scene"]).integer();
  if (root.has_key("scenes")) {
    iter(root, "scenes", t.scenes, [&](auto & n, auto & o) {
      parse_string(n, "name",  o.name);
      parse_ints  (n, "nodes", o.nodes);
    });
  }
  if (root.has_key("skins")) {
    iter(root, "skins", t.skins, [&](auto & n, auto & o) {
      parse_int   (n, "inverseBindMatrices", o.inverse_bind_mat);
      parse_int   (n, "skeleton",            o.skeleton);
      parse_ints  (n, "joints",              o.joints);
      parse_string(n, "name",                o.name);
    });
  }

  return t;
}
