module glub;
import hai;

[[noreturn]] static inline void die(const char * msg) {
  throw glub::error { msg };
}

static auto & accessor_vec2(const glub::t & t, int n) {
  auto & acc = t.accessors[n];
  if (acc.component_type != glub::accessor_comp_type::flt) die("unsupported accessor component type");
  if (acc.type != "VEC2") die("unsupported accessor type");
  return acc;
}
static auto & accessor_vec3(const glub::t & t, int n) {
  auto & acc = t.accessors[n];
  if (acc.component_type != glub::accessor_comp_type::flt) die("unsupported accessor component type");
  if (acc.type != "VEC3") die("unsupported accessor type");
  return acc;
}

static auto & buffer_view(const glub::t & t, int n) {
  if (n < 0 || n >= t.buffer_views.size()) die("buffer views index out-of-bounds");
  auto & bv = t.buffer_views[n];
  if (bv.buffer != 0) die("invalid buffer id");
  return bv;
}
static auto & buffer_view_array_buffer(const glub::t & t, int n) {
  auto & bv = buffer_view(t, n);
  if (bv.target != glub::buffer_view_type::array_buffer) die("invalid buffer view type");
  return bv;
}
static auto & buffer_view_element_array_buffer(const glub::t & t, int n) {
  auto & bv = buffer_view(t, n);
  if (bv.target != glub::buffer_view_type::element_array_buffer) die("invalid buffer view type");
  return bv;
}

template<typename T>
static auto cast(auto & acc, auto & bv, auto & t) {
  auto ptr = reinterpret_cast<const T *>(t.data.begin() + acc.byte_offset + bv.byte_offset);
  if ((void *)(ptr + acc.count) > t.data.end()) die("buffer overrun");
  return ptr;
}

glub::mesh_counts glub::mesh_counts::for_all_meshes(const glub::t & t) {
  struct mesh_counts res {};
  for (auto & m : t.meshes) {
    for (auto & p : m.primitives) {
      if (p.mode != glub::primitive_mode::triangles) die("unsupported primitive mode");
      if (p.indices < 0) die("missing indices in mesh");

      res.v_count += accessor_vec3(t, p.position).count;
      res.i_count += t.accessors[p.indices].count;
    }
  }
  return res;
}

void glub::mesh_counts::for_each(const t & t, hai::fn<void, mesh_counts> fn) {
  for (auto & m : t.meshes) {
    for (auto & p : m.primitives) {
      mesh_counts res {};
      res.v_count = t.accessors[p.position].count;
      res.i_count = t.accessors[p.indices].count;
      fn(res);
    }
  }
}

void glub::load_all_indices(const glub::t & t, unsigned short * ptr) {
  for (auto & m : t.meshes) {
    for (auto & p : m.primitives) {
      auto & acc = t.accessors[p.indices];
      if (acc.type != "SCALAR") die("unsupported accessor type");
      if (acc.component_type == glub::accessor_comp_type::unsigned_shrt) {
        auto & bv = buffer_view_element_array_buffer(t, acc.buffer_view);
        auto buf = cast<unsigned short>(acc, bv, t);
        for (auto i = 0; i < acc.count; i++) *ptr++ = *buf++;
      } else die("unsupported accessor component type");
    }
  }
}
void glub::load_all_normals(const glub::t & t, dotz::vec3 * ptr) {
  for (auto & m : t.meshes) {
    for (auto & p : m.primitives) {
      auto & acc = accessor_vec3(t, p.normal);
      auto & bv = buffer_view_array_buffer(t, acc.buffer_view);
      auto buf = cast<dotz::vec3>(acc, bv, t);
      for (auto i = 0; i < acc.count; i++) *ptr++ = *buf++;
    }
  }
}
void glub::load_all_uvs(const glub::t & t, dotz::vec2 * ptr) {
  for (auto & m : t.meshes) {
    for (auto & p : m.primitives) {
      auto & acc = accessor_vec2(t, p.texcoord_0);
      auto & bv = buffer_view_array_buffer(t, acc.buffer_view);
      auto buf = cast<dotz::vec2>(acc, bv, t);
      for (auto i = 0; i < acc.count; i++) *ptr++ = *buf++;
    }
  }
}
void glub::load_all_vertices(const glub::t & t, dotz::vec3 * ptr) {
  for (auto & m : t.meshes) {
    for (auto & p : m.primitives) {
      auto & acc = accessor_vec3(t, p.position);
      auto & bv = buffer_view_array_buffer(t, acc.buffer_view);
      auto buf = cast<dotz::vec3>(acc, bv, t);
      for (auto i = 0; i < acc.count; i++) *ptr++ = *buf++;
    }
  }
}
