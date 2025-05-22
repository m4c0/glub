#pragma leco tool
import file;
import hai;
import jason;
import jute;
import print;

// TODO: better errors
struct invalid_magic {};
struct invalid_version {};
struct unsupported_format {};

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
};

int main() {
  metadata md { "example.glb" };

  using namespace jason::ast::nodes;
  auto & root = md.root();

  for (auto & b : cast<array>(root["buffers"])) {
    auto & bd = cast<dict>(b);
    auto len = cast<number>(bd["byteLength"]).integer();
    putln("buffer len: ", len);
  }
  for (auto & b : cast<array>(root["bufferViews"])) {
    auto & bd = cast<dict>(b);
    auto buf = cast<number>(bd["buffer"]).integer();
    auto len = cast<number>(bd["byteLength"]).integer();
    auto ofs = bd.has_key("byteOffset")
      ? cast<number>(bd["byteOffset"]).integer()
      : 0;
    putln("buffer view len: ", len, ", ofs: ", ofs, ", id: ", buf);
  }
  for (auto & a : cast<array>(root["accessors"])) {
    auto & ad = cast<dict>(a);
    auto bv = ad.has_key("bufferView")
      ? cast<number>(ad["bufferView"]).integer()
      : 0;
    auto ofs = ad.has_key("byteOffset")
      ? cast<number>(ad["byteOffset"]).integer()
      : 0;
    auto ctype = cast<number>(ad["componentType"]).integer();
    auto count = cast<number>(ad["count"]).integer();
    auto type = cast<string>(ad["type"]).str();
    putln("accessor view: ", bv, ", ofs: ", ofs, ", ctype: ", ctype, ", count: ", count, ", type: ", type);
  }

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

      auto & attr = cast<dict>(pd["attributes"]);
      for (auto &[k, v]: attr) {
        auto vv = cast<number>(v).integer();
        putln("attr ", k, ": ", vv);
      }

      if (pd.has_key("indices")) {
        auto ind = cast<number>(pd["indices"]).integer();
        putln("idx: ", ind);
      }
    }
  }
}
