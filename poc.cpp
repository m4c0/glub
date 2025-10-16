#pragma leco tool
import glub_old;
import hai;
import jason;
import jute;
import print;

using namespace glub;

static void dump_node(const metadata & meta, int idx, int indent = 0) {
  using namespace jason::ast::nodes;

  putln();
  putf("%*s", indent, "");
  putln("node id ", idx);

  auto & nodes = cast<array>(meta.root()["nodes"]);
  auto & nd = cast<dict>(nodes[idx]);
  if (nd.has_key("name")) {
    putf("%*s", indent, "");
    putln("node: ", cast<string>(nd["name"]).str());
  }

  if (nd.has_key("matrix"))      { putf("%*s", indent, ""); putln("has matrix");      }
  if (nd.has_key("rotation"))    { putf("%*s", indent, ""); putln("has rotation");    }
  if (nd.has_key("scale"))       { putf("%*s", indent, ""); putln("has scale");       }
  if (nd.has_key("translation")) { putf("%*s", indent, ""); putln("has translation"); }
  if (nd.has_key("weights"))     { putf("%*s", indent, ""); putln("has weights");     }

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
  metadata meta { "models/BoxAnimated.glb" };

  using namespace jason::ast::nodes;
  auto & root = meta.root();

  putln("root keys:");
  for (auto &[k, _]: root) putln("- ", k);

  auto sid = cast<number>(root["scene"]).integer();
  auto & scenes = cast<array>(root["scenes"]);
  auto & scene = cast<dict>(scenes[sid]);
  auto & snodes = cast<array>(scene["nodes"]);
  for (auto & node_idx : snodes) {
    dump_node(meta, cast<number>(node_idx).integer());
  }

  putln();
  for (auto & a : meta.animations()) {
    putln("animation");
    for (auto & c : a.channels) {
      put("  channel for node: ", c.node, " on: ");
      switch (c.path) {
        case path::WEIGHTS:     putln("weight");      break;
        case path::TRANSLATION: putln("translation"); break;
        case path::ROTATION:    putln("rotation");    break;
        case path::SCALE:       putln("scale");       break;
      }
      for (auto [v, t] : c.samples) {
        putfn("  - %f %f %f %f @%f", v.x, v.y, v.z, v.w, t);
      }
    }
  }
} catch (unsupported_feature f) {
  errln("Unsupported feature: ", f.name);
} catch (...) {
  return 42;
}
