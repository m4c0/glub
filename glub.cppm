export module glub;
import file;
import jason;
import jute;
import hai;

export namespace glub {
  // TODO: better errors
  struct invalid_magic {};
  struct invalid_parameter {};
  struct invalid_version {};
  struct unsupported_format {};
  struct unsupported_feature {
    const char * name;
  };
  
  enum class type { SCALAR, VEC2, VEC3, VEC4, MAT2, MAT3, MAT4 };
  [[nodiscard]] constexpr type parse_type(jute::view str) {
    if (str == "SCALAR") return type::SCALAR;
    if (str == "VEC2")   return type::VEC2;
    if (str == "VEC3")   return type::VEC3;
    if (str == "VEC4")   return type::VEC4;
    if (str == "MAT2")   return type::MAT2;
    if (str == "MAT3")   return type::MAT3;
    if (str == "MAT4")   return type::MAT4;
    throw invalid_parameter {};
  }
  [[nodiscard]] constexpr type parse_type(const jason::ast::nodes::dict & ad) {
    using namespace jason::ast::nodes;
    return parse_type(*cast<string>(ad["type"]).str());
  }
  
  enum class comp_type { BYTE, UBYTE, SHORT, USHORT, UINT, FLOAT };
  [[nodiscard]] constexpr comp_type parse_comp_type(int n) {
    if (n == 5120) return comp_type::BYTE;
    if (n == 5121) return comp_type::UBYTE;
    if (n == 5122) return comp_type::SHORT;
    if (n == 5123) return comp_type::USHORT;
    if (n == 5125) return comp_type::UINT;
    if (n == 5126) return comp_type::FLOAT;
    throw invalid_parameter {};
  }
  [[nodiscard]] constexpr comp_type parse_comp_type(const jason::ast::nodes::dict & ad) {
    using namespace jason::ast::nodes;
    return parse_comp_type(cast<number>(ad["componentType"]).integer());
  }
  
  struct buffer_view {
    unsigned ofs;
    unsigned len;
  };
  
  struct accessor {
    buffer_view buf_view;
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
  
  struct node {
    hai::array<node> children {};
  };

  class metadata {
    hai::array<char> m_json_src;
    jason::ast::node_ptr m_json;
  
    hai::array<char> m_buf;
   
  public:
    explicit metadata(const char * filename) {
      auto f = file::open_for_reading(filename);
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
      if (buffers.size() != 1) throw unsupported_feature { "Multiple buffers" };
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

    auto & buffer(unsigned id) const {
      if (id != 0) throw unsupported_feature { "Multiple buffers" };
      return m_buf;
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
      if (buf != 0) throw unsupported_feature { "Multiple buffers" };
      if (ofs + len > m_buf.size()) throw invalid_parameter {};
  
      return glub::buffer_view { ofs, len };
    }
    auto buffer_views() const {
      using namespace jason::ast::nodes;

      auto sz = cast<array>(root()["bufferViews"]).size();
      hai::array<glub::buffer_view> res { sz };
      for (auto i = 0; i < sz; i++) res[i] = buffer_view(i);
      return res;
    }
  
    [[nodiscard]] glub::accessor accessor(unsigned id) const {
      using namespace jason::ast::nodes;
  
      auto & a = cast<array>(root()["accessors"]);
      auto & ad = cast<dict>(a[id]);
      auto bv = ad.has_key("bufferView")
        ? cast<number>(ad["bufferView"]).integer()
        : 0;
      auto ofs = ad.has_key("byteOffset")
        ? cast<number>(ad["byteOffset"]).integer()
        : 0;
  
      auto [bv_ofs, len] = buffer_view(bv);
  
      return {
        .buf_view = { bv_ofs + ofs, len - ofs },
        .ctype = parse_comp_type(ad),
        .type = parse_type(ad),
        .count = cast<number>(ad["count"]).integer(),
      };
    }
    [[nodiscard]] glub::accessor accessor(unsigned id, type t, comp_type ct) const {
      auto a = accessor(id);
      if (a.type != t) throw invalid_parameter {};
      if (a.ctype != ct) throw invalid_parameter {};
      return a;
    }
  
    [[nodiscard]] glub::primitive primitive(const jason::ast::nodes::dict & pd) const {
      using namespace jason::ast::nodes;
  
      if (pd.has_key("topology")) throw unsupported_feature { "Mesh primitive topologies" };
  
      glub::primitive prim {};
  
      auto & attr = cast<dict>(pd["attributes"]);
      for (auto &[k, v]: attr) {
        auto vv = cast<number>(v).integer();
        if      (*k == "POSITION")   prim.position = accessor(vv, type::VEC3, comp_type::FLOAT);
        else if (*k == "NORMAL")     prim.normal   = accessor(vv, type::VEC3, comp_type::FLOAT);
        else if (*k == "TEXCOORD_0") prim.uv0      = accessor(vv, type::VEC2, comp_type::FLOAT);
        else if (*k == "JOINTS_0")   throw unsupported_feature { "Mesh with joints" };
        else if (*k == "WEIGHTS_0")  throw unsupported_feature { "Mesh with weights" };
        else throw invalid_parameter {};
      }
  
      if (pd.has_key("indices")) {
        auto ind = cast<number>(pd["indices"]).integer();
        prim.indices = accessor(ind, type::SCALAR, comp_type::USHORT);
      }
  
      return prim;
    }
    [[nodiscard]] glub::mesh mesh(unsigned id) const {
      using namespace jason::ast::nodes;
  
      auto & meshes = cast<array>(root()["meshes"]);
      auto & md = cast<dict>(meshes[id]);
      auto & prim = cast<array>(md["primitives"]);
  
      if (md.has_key("weights")) throw unsupported_feature { "Morphing" };
  
      glub::mesh m { .prims { prim.size() } };
      for (auto i = 0; i < prim.size(); i++) {
        m.prims[i] = primitive(cast<dict>(prim[i]));
      }
  
      return m;
    }

    [[nodiscard]] glub::node node(unsigned idx) const {
      using namespace jason::ast::nodes;

      auto & nodes = cast<array>(root()["nodes"]);
      auto & nd = cast<dict>(nodes[idx]);

      glub::node n {};

      if (nd.has_key("children")) {
        auto & nc = cast<array>(nd["children"]);
        n.children.set_capacity(nc.size());
        for (auto i = 0; i < nc.size(); i++) {
          auto ncid = cast<number>(nc[i]).integer();
          n.children[i] = node(ncid);
        }
      }

      return n;
    }
    [[nodiscard]] glub::node node() const {
      using namespace jason::ast::nodes;

      auto sid = cast<number>(root()["scene"]).integer();
      auto & scenes = cast<array>(root()["scenes"]);
      if (scenes.size() != 1) throw unsupported_feature { "Multiple scenes" };

      auto & scene = cast<dict>(scenes[sid]);
      auto & snodes = cast<array>(scene["nodes"]);
      if (snodes.size() != 1) throw unsupported_feature { "Scene with multiple nodes" };

      auto nid = cast<number>(snodes[0]).integer();
      return node(nid);
    }
  };
}

