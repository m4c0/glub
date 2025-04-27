#pragma leco tool
import file;
import print;

struct invalid_magic {};
struct invalid_version {};

int main() {
  auto f = file::open_for_reading("example.glb");
  if (f.read_u32() != 'FTlg') throw invalid_magic {};
  if (f.read_u32() != 2) throw invalid_version {};
  auto len = f.read_u32();
  putln(len);
}
