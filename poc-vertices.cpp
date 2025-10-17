#pragma leco test

import glub;
import jojo;
import print;

int main() {
  auto src = jojo::read("models/BoxAnimated.glb");
  const auto t = glub::parse(src.begin(), src.size());

  if (t.scene < 0) die("missing main scene");
  if (t.scene >= t.scenes.size()) die("main scene index out-of-bounds");

  for (auto node_idx : t.scenes[t.scene].nodes) {
    if (node_idx < 0 || node_idx >= t.nodes.size()) die("node index out-of-bounds");
    auto & node = t.nodes[node_idx];

    putln("node: ", node_idx, " ", node.name);

    for (auto child : node.children) {
      if (child < 0 || child >= t.nodes.size()) die("node index out-of-bounds");
      auto & node = t.nodes[child];

      putln(" node: ", child, " ", node.name);

      for (auto gchild : node.children) {
        if (gchild < 0 || gchild >= t.nodes.size()) die("node index out-of-bounds");
        auto & node = t.nodes[gchild];

        putln("  node: ", gchild, " ", node.name);
      }
    }
  }
}
