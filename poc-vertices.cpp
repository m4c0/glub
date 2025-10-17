#pragma leco test

import glub;
import jojo;
import print;

static void recurse(const glub::t & t, int n) {
  if (n < 0 || n >= t.nodes.size()) die("node index out-of-bounds");
  auto & node = t.nodes[n];

  put(n, " ");
  if (node.matrix.size()) put("m");
  if (node.rotation.size()) put("r");
  if (node.scale.size()) put("s");
  if (node.translation.size()) put("t");
  putln();

  for (auto child : node.children) {
    recurse(t, child);
  }
}

int main() {
  auto src = jojo::read("models/BoxAnimated.glb");
  const auto t = glub::parse(src.begin(), src.size());

  if (t.scene < 0) die("missing main scene");
  if (t.scene >= t.scenes.size()) die("main scene index out-of-bounds");

  for (auto n : t.scenes[t.scene].nodes) {
    recurse(t, n);
  }
}
