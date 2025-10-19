module glub;
import hai;

[[noreturn]] static inline void die(const char * msg) {
  throw glub::error { msg };
}

static auto & accessor(const glub::t & t, int n) {
  if (n < 0 || n >= t.accessors.size()) die("accessor index out-of-bounds");
  auto & a = t.accessors[n];
  if (a.normalised) die("normalised accessors are not supported");
  return a;
}
static auto & accessor_vec2(const glub::t & t, int n) {
  auto & acc = accessor(t, n);
  if (acc.component_type != glub::accessor_comp_type::flt) die("unsupported accessor component type");
  if (acc.type != "VEC2") die("unsupported accessor type");
  return acc;
}
static auto & accessor_vec3(const glub::t & t, int n) {
  auto & acc = accessor(t, n);
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

      for (auto &[key, a] : p.attributes) {
        if (key != "POSITION") continue;
        res.v_count += accessor_vec3(t, a).count;
      }

      res.i_count += ::accessor(t, p.indices).count;
    }
  }
  return res;
}

void glub::mesh_counts::for_each(const t & t, hai::fn<void, mesh_counts> fn) {
  for (auto & m : t.meshes) {
    for (auto & p : m.primitives) {
      mesh_counts res {};
      for (auto &[key, a] : p.attributes) {
        if (key != "POSITION") continue;
        res.v_count = ::accessor(t, a).count;
      }
      res.i_count = ::accessor(t, p.indices).count;
      fn(res);
    }
  }
}

void glub::load_all_indices(const glub::t & t, unsigned short * ptr) {
  for (auto & m : t.meshes) {
    for (auto & p : m.primitives) {
      if (p.mode != glub::primitive_mode::triangles) die("unsupported primitive mode");
      if (p.indices < 0) die("missing indices in mesh");

      auto & acc = ::accessor(t, p.indices);
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
      if (p.mode != glub::primitive_mode::triangles) die("unsupported primitive mode");
      if (p.indices < 0) die("missing indices in mesh");

      for (auto &[key, a] : p.attributes) {
        if (key != "NORMAL") continue;

        auto & acc = accessor_vec3(t, a);
        auto & bv = buffer_view_array_buffer(t, acc.buffer_view);
        auto buf = cast<dotz::vec3>(acc, bv, t);
        for (auto i = 0; i < acc.count; i++) *ptr++ = *buf++;
      }
    }
  }
}
void glub::load_all_uvs(const glub::t & t, dotz::vec2 * ptr) {
  for (auto & m : t.meshes) {
    for (auto & p : m.primitives) {
      if (p.mode != glub::primitive_mode::triangles) die("unsupported primitive mode");
      if (p.indices < 0) die("missing indices in mesh");

      for (auto &[key, a] : p.attributes) {
        if (key != "TEXCOORD_0") continue;

        auto & acc = accessor_vec2(t, a);
        auto & bv = buffer_view_array_buffer(t, acc.buffer_view);
        auto buf = cast<dotz::vec2>(acc, bv, t);
        for (auto i = 0; i < acc.count; i++) *ptr++ = *buf++;
      }
    }
  }
}
void glub::load_all_vertices(const glub::t & t, dotz::vec3 * ptr) {
  for (auto & m : t.meshes) {
    for (auto & p : m.primitives) {
      if (p.mode != glub::primitive_mode::triangles) die("unsupported primitive mode");
      if (p.indices < 0) die("missing indices in mesh");

      for (auto &[key, a] : p.attributes) {
        if (key != "POSITION") continue;

        auto & acc = accessor_vec3(t, a);
        auto & bv = buffer_view_array_buffer(t, acc.buffer_view);
        auto buf = cast<dotz::vec3>(acc, bv, t);
        for (auto i = 0; i < acc.count; i++) *ptr++ = *buf++;
      }
    }
  }
}
