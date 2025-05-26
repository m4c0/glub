#pragma leco app

import glub;
import vapp;
import voo;

static auto load_buffer(const vee::physical_device pd, const glub::metadata & meta) {
  const auto & data = meta.buffer(0);

  auto mem = vee::create_host_memory(pd, data.size());
  auto m = voo::memiter<char>(*mem);
  for (auto i = 0; i < data.size(); i++) {
    m[i] = data[i];
  }
  return mem;
}

struct i : public vapp {
  void run() override {
    main_loop("poc", [&](auto & dq, auto & sw) {
      auto pd = dq.physical_device();

      glub::metadata meta { "models/BoxAnimated.glb" };
      auto mem = load_buffer(pd, meta);

      extent_loop(dq.queue(), sw, [&] {
        sw.queue_one_time_submit(dq.queue(), [&](auto pcb) {
          auto scb = sw.cmd_render_pass({ *pcb });
        });
      });
    });
  }
} i;
