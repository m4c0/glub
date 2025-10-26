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
  const auto parse_float = [&](auto & n, jute::view key, auto & v) {
    if (n.has_key(key)) v = cast<number>(n[key]).real();
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

  const auto parse_texinfo = [&](auto & n, jute::view key, auto & v) {
    if (!n.has_key(key)) return;
    auto & ti = cast<dict>(n[key]);
    v.index = cast<number>(ti["index"]).integer();
    parse_int(ti, "texCoord", v.tex_coord);
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
  if (root.has_key("animations")) {
    iter(root, "animations", t.animations, [&](auto & n, auto & o) {
      parse_string(n, "name", o.name);

      iter(n, "channels", o.channels, [&](auto & n, auto & o) {
        o.sampler = cast<number>(n["sampler"]).integer();
        auto & p = cast<dict>(n["target"]);
        parse_int(p, "node", o.target_node);
        o.target_path = cast<string>(p["path"]).str();
      });
      iter(n, "samplers", o.samplers, [&](auto & n, auto & o) {
        o.input = cast<number>(n["input"]).integer();
        parse_string(n, "interpolation", o.interpolation);
        o.output = cast<number>(n["output"]).integer();
      });
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
  if (root.has_key("images")) {
    iter(root, "images", t.images, [&](auto & n, auto & o) {
      parse_string(n, "uri",        o.uri);
      parse_string(n, "mimeType",   o.mime_type);
      parse_int   (n, "bufferView", o.buffer_view);
      parse_string(n, "name",       o.name);
    });
  }
  if (root.has_key("materials")) {
    iter(root, "materials", t.materials, [&](auto & n, auto & o) {
      parse_string(n, "name", o.name);
      if (n.has_key("pbrMetallicRoughness")) {
        auto & pbr = cast<dict>(n["pbrMetallicRoughness"]);
        parse_floats(pbr, "baseColorFactor", o.base_colour_factor);
        parse_texinfo(pbr, "baseColorTexture", o.base_colour_texture);
        parse_float(pbr, "metallicFactor", o.metallic_factor);
        parse_float(pbr, "roughnessFactor", o.roughness_factor);
        parse_texinfo(pbr, "metallicRoughnessTexture", o.metallic_roughness_texture);
      }
      parse_texinfo(n, "normalTexture", o.normal_texture);
      parse_texinfo(n, "occlusionTexture", o.occlusion_texture);
      parse_texinfo(n, "emissiveTexture", o.emissive_texture);
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
        for (auto &[k, v] : attrs) {
          auto vi = cast<number>(v).integer();
          *ptr++ = { k, vi };
        }

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
  if (root.has_key("textures")) {
    iter(root, "textures", t.textures, [&](auto & n, auto & o) {
      parse_int   (n, "sample", o.sample);
      parse_int   (n, "source", o.source);
      parse_string(n, "name",   o.name);
    });
  }

  for (auto & a : t.accessors) {
    if (a.normalised) throw error { "normalised accessors are not supported" };
    if (a.buffer_view < 0 || a.buffer_view >= t.buffer_views.size()) throw error { "invalid buffer view index" };
    // TODO: assert buffer don't go over data size
  }

  for (auto & bv : t.buffer_views) {
    if (bv.buffer < 0 || bv.buffer >= t.buffers.size()) throw error { "invalid buffer index" };
    // TODO: assert buffer don't go over data size
  }

  for (auto & i : t.images) {
    if (i.buffer_view < 0 || i.buffer_view >= t.buffer_views.size()) throw error { "invalid image buffer view" };
    if (i.uri.size()) throw error { "images with uri are not supported" };
  }

  for (auto & m : t.materials) {
    if (m.base_colour_factor.size() == 0) m.base_colour_factor = hai::array<float>::make(1, 1, 1, 1);
    if (m.base_colour_texture.tex_coord != 0) throw error { "material coord is not supported" };
    if (m.base_colour_texture.index >= static_cast<int>(t.textures.size())) throw error { "invalid base colour texture index" };

    if (m.normal_texture.tex_coord != 0) throw error { "material coord is not supported" };
    if (m.normal_texture.index >= static_cast<int>(t.textures.size())) throw error { "invalid base normal texture index" };
  }

  for (auto & m : t.meshes) {
    for (auto & p : m.primitives) {
      if (p.mode != glub::primitive_mode::triangles) throw error { "unsupported primitive mode" };

      if (p.indices < 0) throw error { "missing indices in mesh" };
      if (p.indices >= t.accessors.size()) throw error { "invalid accessor index" };
      auto p_index = &t.accessors[p.indices];
      if (p_index->type != "SCALAR") throw error { "unsupported accessor type" };
      if (p_index->component_type != glub::accessor_comp_type::unsigned_shrt) throw error { "unsupported accessor component type" };

      int count = -1;
      for (auto &[k, a] : p.attributes) {
        if (a < 0 || a > t.accessors.size()) throw error { "invalid accessor index" };

        auto & aa = t.accessors[a];
        if (count == -1) count = aa.count;
        else if (aa.count != count) throw error { "mismatching attribute accessor size" };

        if (aa.component_type != glub::accessor_comp_type::flt) throw error { "unsupported accessor component type" };

        if (k == "POSITION") {
          if (aa.type != "VEC3") throw error { "unsupported accessor type" };
          p.accessors.position = a;
        } else if (k == "NORMAL") {
          if (aa.type != "VEC3") throw error { "unsupported accessor type" };
          p.accessors.normal = a;
        } else if (k == "TEXCOORD_0") {
          if (aa.type != "VEC2") throw error { "unsupported accessor type" };
          p.accessors.texcoord_0 = a;
        }
      }
    }
  }

  for (auto & n : t.nodes) {
    auto trs = n.translation.size() || n.rotation.size() || n.scale.size();
    if (n.matrix.size() && trs) throw error { "node with both matrix and TRS" };
    if (n.matrix.size() && n.matrix.size() != 16) throw error { "invalid node matrix size" };
    if (n.translation.size() && n.translation.size() != 3) throw error { "invalid node translation size" };
    if (n.rotation.size() && n.rotation.size() != 4) throw error { "invalid node rotation size" };
    if (n.scale.size() && n.scale.size() != 3) throw error { "invalid node scale size" };
  }

  for (auto & x : t.textures) {
    if (x.source < 0 || x.source >= t.images.size()) throw error { "invalid texture source" };
  }

  return t;
}
