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
  for (auto &[k, v] : cast<dict>(json)) {
    putln(k);
  }
}
