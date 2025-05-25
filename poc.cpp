#pragma leco tool
import file;
import hai;
import jason;
import jute;
import print;

// TODO: better errors
struct invalid_magic {};
struct invalid_parameter {};
struct invalid_version {};
struct unsupported_format {};

enum class type { SCALAR, VEC2, VEC3, VEC4, MAT2, MAT3, MAT4 };
[[nodiscard]] static constexpr type parse_type(jute::view str) {
  if (str == "SCALAR") return type::SCALAR;
  if (str == "VEC2")   return type::VEC2;
  if (str == "VEC3")   return type::VEC3;
  if (str == "VEC4")   return type::VEC4;
  if (str == "MAT2")   return type::MAT2;
  if (str == "MAT3")   return type::MAT3;
  if (str == "MAT4")   return type::MAT4;
  throw invalid_parameter {};
}
[[nodiscard]] static constexpr type parse_type(const jason::ast::nodes::dict & ad) {
  using namespace jason::ast::nodes;
  return parse_type(*cast<string>(ad["type"]).str());
}

enum class comp_type { BYTE, UBYTE, SHORT, USHORT, UINT, FLOAT };
[[nodiscard]] static constexpr comp_type parse_comp_type(int n) {
  if (n == 5120) return comp_type::BYTE;
  if (n == 5121) return comp_type::UBYTE;
  if (n == 5122) return comp_type::SHORT;
  if (n == 5123) return comp_type::USHORT;
  if (n == 5125) return comp_type::UINT;
  if (n == 5126) return comp_type::FLOAT;
  throw invalid_parameter {};
}
[[nodiscard]] static constexpr comp_type parse_comp_type(const jason::ast::nodes::dict & ad) {
  using namespace jason::ast::nodes;
  return parse_comp_type(cast<number>(ad["componentType"]).integer());
}

struct accessor {
  jute::view buf_view;
  comp_type ctype;
  type type;
  int count;
};

struct primitive {
  accessor position;
  accessor normal;
  accessor uv0;
  accessor indices;
};

struct mesh {
  hai::array<primitive> prims;
};

class metadata {
  hai::array<char> m_json_src;
  jason::ast::node_ptr m_json;

  hai::array<char> m_buf;
 
public:
  explicit metadata(const char * filename) {
    auto f = file::open_for_reading("example.glb");
    if (f.read_u32() != 'FTlg') throw invalid_magic {};
    if (f.read_u32() != 2) throw invalid_version {};
    f.read_u32(); // Length of whole file (TODO: validate)

    m_json_src = hai::array<char> { f.read_u32() };
    if (f.read_u32() != 'NOSJ') throw invalid_magic {};
    f.read(m_json_src);

    m_json = jason::parse(m_json_src);

    // TODO: validate "asset" version, etc

    using namespace jason::ast::nodes;
    auto & root = cast<dict>(m_json);
    auto & buffers = cast<array>(root["buffers"]);
    if (buffers.size() != 1) throw unsupported_format {};
    auto & buffer = cast<dict>(buffers[0]);
    unsigned buf_len = cast<number>(buffer["byteLength"]).integer();

    m_buf = hai::array<char> { buf_len };
    if (m_buf.size() > f.read_u32()) throw invalid_magic {};
    if (f.read_u32() != '\0NIB') throw invalid_magic {};
    f.read(m_buf);
  }
  
  auto & root() const {
    using namespace jason::ast::nodes;
    return cast<dict>(m_json);
  }

  auto buffer_view(unsigned id) const {
    using namespace jason::ast::nodes;

    auto & bv = cast<array>(root()["bufferViews"]);
    auto & bd = cast<dict>(bv[id]);

    auto buf = cast<number>(bd["buffer"]).integer();
    unsigned len = cast<number>(bd["byteLength"]).integer();
    unsigned ofs = bd.has_key("byteOffset")
      ? cast<number>(bd["byteOffset"]).integer()
      : 0;

    // TODO: validate stride
    if (buf != 0) throw invalid_parameter {};
    if (ofs + len > m_buf.size()) throw invalid_parameter {};

    return jute::view { m_buf.begin() + ofs, len };
  }

  [[nodiscard]] ::accessor accessor(unsigned id) const {
    using namespace jason::ast::nodes;

    auto & a = cast<array>(root()["accessors"]);
    auto & ad = cast<dict>(a[id]);
    auto bv = ad.has_key("bufferView")
      ? cast<number>(ad["bufferView"]).integer()
      : 0;
    auto ofs = ad.has_key("byteOffset")
      ? cast<number>(ad["byteOffset"]).integer()
      : 0;

    return {
      .buf_view = buffer_view(bv).subview(ofs).after,
      .ctype = parse_comp_type(ad),
      .type = parse_type(ad),
      .count = cast<number>(ad["count"]).integer(),
    };
  }
  [[nodiscard]] ::accessor accessor(unsigned id, type t, comp_type ct) const {
    auto a = accessor(id);
    if (a.type != t) throw invalid_parameter {};
    if (a.ctype != ct) throw invalid_parameter {};
    return a;
  }

  [[nodiscard]] ::primitive primitive(const jason::ast::nodes::dict & pd) const {
    using namespace jason::ast::nodes;

    if (pd.has_key("topology")) throw invalid_parameter {};

    ::primitive prim {};

    auto & attr = cast<dict>(pd["attributes"]);
    for (auto &[k, v]: attr) {
      auto vv = cast<number>(v).integer();
      if      (*k == "POSITION")   prim.position = accessor(vv, type::VEC3, comp_type::FLOAT);
      else if (*k == "NORMAL")     prim.normal   = accessor(vv, type::VEC3, comp_type::FLOAT);
      else if (*k == "TEXCOORD_0") prim.uv0      = accessor(vv, type::VEC2, comp_type::FLOAT);
      else if (*k == "JOINTS_0")   errln("TODO: support for joints");
      else if (*k == "WEIGHTS_0")  errln("TODO: support for weights");
      else throw invalid_parameter {};
    }

    if (pd.has_key("indices")) {
      auto ind = cast<number>(pd["indices"]).integer();
      prim.indices = accessor(ind, type::SCALAR, comp_type::USHORT);
    }

    return prim;
  }
  [[nodiscard]] ::mesh mesh(unsigned id) const {
    using namespace jason::ast::nodes;

    auto & meshes = cast<array>(root()["meshes"]);
    auto & md = cast<dict>(meshes[id]);
    auto & prim = cast<array>(md["primitives"]);

    ::mesh m { .prims { prim.size() } };
    for (auto i = 0; i < prim.size(); i++) {
      m.prims[i] = primitive(cast<dict>(prim[i]));
    }
    return m;
  }
};

static void dump_node(const metadata & meta, int idx, int indent = 0) {
  using namespace jason::ast::nodes;

  auto & nodes = cast<array>(meta.root()["nodes"]);
  auto & nd = cast<dict>(nodes[idx]);
  if (nd.has_key("name")) {
    putf("%*s", indent, "");
    putln("node: ", cast<string>(nd["name"]).str());
  }
  if (nd.has_key("skin")) {
    putf("%*s", indent, "");
    auto sid = cast<number>(nd["skin"]).integer();
    auto & skins = cast<array>(meta.root()["skins"]);
    auto & skin = cast<dict>(skins[sid]);

    auto & joints = cast<array>(skin["joints"]);
    put("skin: ", sid, " joints: ", joints.size());

    if (skin.has_key("skeleton")) {
      auto sc = cast<number>(skin["skeleton"]).integer();
      put(" root: ", sc);
    }
    if (skin.has_key("inverseBindMatrices")) {
      auto ibm = cast<number>(skin["inverseBindMatrices"]).integer();
      auto a = meta.accessor(ibm, type::MAT4, comp_type::FLOAT);
      put(" ibm count: ", a.count);
    }

    putln();

    for (auto & j : joints) {
      auto jidx = cast<number>(j).integer();
      dump_node(meta, jidx, indent + 2);
    }
  }
  if (nd.has_key("mesh")) {
    auto m = meta.mesh(cast<number>(nd["mesh"]).integer());
    putf("%*s", indent, "");
    putln("mesh with ", m.prims.size(), " primitives");
  }
  if (nd.has_key("children")) {
    putf("%*s", indent, "");
    putln("children:");
    for (auto & node_idx : cast<array>(nd["children"])) {
      dump_node(meta, cast<number>(node_idx).integer(), indent + 2);
    }
  }
}

int main() try {
  metadata meta { "example.glb" };

  using namespace jason::ast::nodes;
  auto & root = meta.root();

  auto sid = cast<number>(root["scene"]).integer();
  auto & scenes = cast<array>(root["scenes"]);
  auto & scene = cast<dict>(scenes[sid]);
  auto & snodes = cast<array>(scene["nodes"]);
  for (auto & node_idx : snodes) {
    dump_node(meta, cast<number>(node_idx).integer());
  }
} catch (...) {
  return 42;
}
