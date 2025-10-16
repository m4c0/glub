#pragma leco test

import jason;
import jojo;
import jute;
import print;
import traits;

using namespace traits::ints;

struct error {
  const char * msg;
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

} catch (const error & e) {
  errln(e.msg);
  return 1;
}
