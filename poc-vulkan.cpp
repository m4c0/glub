#pragma leco app
#pragma leco add_shader "poc-vulkan.frag"
#pragma leco add_shader "poc-vulkan.vert"

import glub;
import hai;
import silog;
import sitime;
import vapp;
import voo;

struct upc {
  float time;
  float aspect;
};

static auto load_buffer(const vee::device_memory::type mem, const glub::metadata & meta, int offset, int len) {
  const auto & data = meta.buffer(0);

  auto m = voo::memiter<char>(mem);
  for (auto i = 0; i < len; i++) {
    m[i] = data[i + offset];
  }
}

struct i : public vapp {
  void run() override {
    main_loop("poc", [&](auto & dq, auto & sw) {
      auto pd = dq.physical_device();

      glub::metadata meta { "models/BoxAnimated.glb" };

      auto bvs = meta.buffer_views();
      hai::array<voo::host_buffer> bufs { bvs.size() };
      for (auto i = 0; i < bvs.size(); i++) {
        auto [ofs, len] = bvs[i];
        bufs[i] = voo::host_buffer { pd, len, vee::buffer_usage::vertex_buffer, vee::buffer_usage::index_buffer };
        load_buffer(bufs[i].memory(), meta, ofs, len);
      }

      auto pl = vee::create_pipeline_layout(
        vee::vertex_push_constant_range<upc>()
      );
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
      sitime::stopwatch t {};

      extent_loop(dq.queue(), sw, [&] {
        upc pc {
          .time = t.millis() / 1000.0f,
          .aspect = sw.aspect(),
        };

        sw.queue_one_time_submit(dq.queue(), [&](auto pcb) {
          auto scb = sw.cmd_render_pass({ *pcb });

          vee::cmd_bind_gr_pipeline(*scb, *ppl);
          vee::cmd_push_vertex_constants(*scb, *pl, &pc);
          glub::visit_meshes(scene, [&](auto m) {
            auto mesh = meta.mesh(m);

            for (auto & prim : mesh.prims) {
              if (!prim.position.count) throw 0; // TODO errors
              if (!prim.indices.count) throw 0; // TODO errors

              auto buf = bufs[prim.position.buffer_view].buffer();
              auto ofs = prim.position.offset;
              vee::cmd_bind_vertex_buffers(*scb, 0, buf, ofs);

              buf = bufs[prim.indices.buffer_view].buffer();
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
