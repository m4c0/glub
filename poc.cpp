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

  auto buffer_view(unsigned id) {
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

  [[nodiscard]] ::accessor accessor(unsigned id) {
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
  [[nodiscard]] ::accessor accessor(unsigned id, type t, comp_type ct) {
    auto a = accessor(id);
    if (a.type != t) throw invalid_parameter {};
    if (a.ctype != ct) throw invalid_parameter {};
    return a;
  }
};

int main() try {
  metadata meta { "example.glb" };

  using namespace jason::ast::nodes;
  auto & root = meta.root();

  auto sid = cast<number>(root["scene"]).integer();
  auto & scenes = cast<array>(root["scenes"]);
  auto & scene = cast<dict>(scenes[sid]);
  auto & snodes = cast<array>(scene["nodes"]);
  for (auto & n : snodes) putln("scene n: ", cast<number>(n).integer());

  auto & nodes = cast<array>(root["nodes"]);
  for (auto & n : nodes) {
    auto & nd = cast<dict>(n);
    auto mesh = cast<number>(nd["mesh"]).integer();
    putln("node mesh: ", mesh);
  }

  auto & meshes = cast<array>(root["meshes"]);
  for (auto & m : meshes) {
    putln("mesh:");
    auto & md = cast<dict>(m);
    auto & prim = cast<array>(md["primitives"]);
    for (auto & p : prim) {
      auto & pd = cast<dict>(p);

      primitive prim {};

      auto & attr = cast<dict>(pd["attributes"]);
      for (auto &[k, v]: attr) {
        auto vv = cast<number>(v).integer();
        if      (*k == "POSITION")   prim.position = meta.accessor(vv, type::VEC3, comp_type::FLOAT);
        else if (*k == "NORMAL")     prim.normal   = meta.accessor(vv, type::VEC3, comp_type::FLOAT);
        else if (*k == "TEXCOORD_0") prim.uv0      = meta.accessor(vv, type::VEC2, comp_type::FLOAT);
        else throw invalid_parameter {};
      }

      if (pd.has_key("indices")) {
        auto ind = cast<number>(pd["indices"]).integer();
        prim.indices = meta.accessor(ind, type::SCALAR, comp_type::USHORT);
      }
    }
  }
} catch (...) {
  return 42;
}
