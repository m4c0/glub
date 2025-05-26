#pragma leco app
#pragma leco add_shader "poc-vulkan.frag"
#pragma leco add_shader "poc-vulkan.vert"

import glub;
import hai;
import silog;
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

      auto bvs = meta.buffer_views();
      hai::array<vee::buffer> bufs { bvs.size() };
      for (auto i = 0; i < bvs.size(); i++) {
        auto [ofs, len] = bvs[i];
        bufs[i] = vee::create_buffer(len, vee::buffer_usage::vertex_buffer, vee::buffer_usage::index_buffer);
        vee::bind_buffer_memory(*bufs[i], *mem, ofs);
      }

      auto pl = vee::create_pipeline_layout();
      auto ppl = vee::create_graphics_pipeline({
        .pipeline_layout = *pl,
        .render_pass = dq.render_pass(),
        .extent = sw.extent(),
        .shaders {
          voo::shader("poc-vulkan.vert.spv").pipeline_vert_stage(),
          voo::shader("poc-vulkan.frag.spv").pipeline_frag_stage(),
        },
        .bindings {
          vee::vertex_input_bind(3 * sizeof(float)),
        },
        .attributes {
          vee::vertex_attribute_vec3(0, 0),
        },
      });

      auto scene = meta.main_scene();

      extent_loop(dq.queue(), sw, [&] {
        sw.queue_one_time_submit(dq.queue(), [&](auto pcb) {
          auto scb = sw.cmd_render_pass({ *pcb });

          vee::cmd_bind_gr_pipeline(*scb, *ppl);
          glub::visit_meshes(scene, [&](auto m) {
            auto mesh = meta.mesh(m);

            for (auto & prim : mesh.prims) {
              if (!prim.position.count) throw 0; // TODO errors
              if (!prim.indices.count) throw 0; // TODO errors

              auto buf = *bufs[prim.position.buffer_view];
              auto ofs = prim.position.offset;
              vee::cmd_bind_vertex_buffers(*scb, 0, buf, ofs);

              buf = *bufs[prim.indices.buffer_view];
              ofs = prim.indices.offset;
              vee::cmd_bind_index_buffer_u16(*scb, buf, ofs);
              vee::cmd_draw(*scb, prim.indices.count);
            }
          });
        });
      });
    });
  }
} i;
