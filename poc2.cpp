#pragma leco test

import jason;
import jojo;
import jute;
import hai;
import print;
import traits;

using namespace traits::ints;

struct error {
  const char * what;
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

struct scene {
  jute::heap name;
  hai::array<int> nodes {};
};
struct t {
  int scene = -1;
  hai::array<::scene> scenes {};
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
  
  using namespace jason::ast::nodes;
  auto & root = cast<dict>(meta);

  t t {};

  if (root.has_key("scene")) t.scene = cast<number>(root["scene"]).integer();
  if (root.has_key("scenes")) {
    auto & scenes = cast<array>(root["scenes"]);
    t.scenes.set_capacity(scenes.size());
    for (auto i = 0; i < scenes.size(); i++) {
      auto & s = cast<dict>(scenes[i]);
      if (s.has_key("name")) t.scenes[i].name = cast<string>(s["name"]).str();
      if (s.has_key("nodes")) {
        auto & nodes = cast<array>(s["nodes"]);
        t.scenes[i].nodes.set_capacity(nodes.size());
        for (auto j = 0; j < nodes.size(); j++) {
          t.scenes[i].nodes[j] = cast<number>(nodes[j]).integer();
        }
      }
    }
  }

  putln("scene: ", t.scene);
  for (auto & s : t.scenes) {
    putln("scenes:");
    put("- ", s.name);
    for (auto i : s.nodes) put(", ", i);
    putln();
  }
  putln();

  for (auto &[k, v] : root) {
    putln(k);
  }
} catch (const jason::error & e) {
  errln(e.what);
  return 1;
} catch (const error & e) {
  errln(e.what);
  return 1;
}
