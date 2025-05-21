#pragma leco tool
import file;
import hai;
import jason;
import jute;
import print;

struct invalid_magic {};
struct invalid_version {};

int main() {
  auto f = file::open_for_reading("example.glb");
  if (f.read_u32() != 'FTlg') throw invalid_magic {};
  if (f.read_u32() != 2) throw invalid_version {};
  f.read_u32(); // Length (TODO: validate)

  // TODO: validate length
  hai::array<char> json_src { f.read_u32() };
  if (f.read_u32() != 'NOSJ') throw invalid_magic {};
  f.read(json_src);

  using namespace jason::ast::nodes;
  auto json = jason::parse(json_src);
  auto & root = cast<dict>(json);

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
  for (auto &n : snodes) putln("n ", cast<number>(n).integer());

  for (auto &[k, v] : root) {
    putln(k);
  }
}
