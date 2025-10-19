module glub;

[[noreturn]] static inline void die(const char * msg) {
  throw glub::error { msg };
}

static auto & accessor(const glub::t & t, int n) {
  if (n < 0 || n >= t.accessors.size()) die("accessor index out-of-bounds");
  auto & a = t.accessors[n];
  if (a.normalised) die("normalised accessors are not supported");
  return a;
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

static auto & mesh(const glub::t & t, int n) {
  if (n < 0 || n >= t.buffer_views.size()) die("mesh index out-of-bounds");
  return t.meshes[n];
}

template<typename T>
static auto cast(auto & acc, auto & bv, auto & t) {
  auto ptr = reinterpret_cast<const T *>(t.data.begin() + acc.byte_offset + bv.byte_offset);
  if ((void *)(ptr + acc.count) > t.data.end()) die("buffer overrun");
  return ptr;
}

glub::mesh_counts glub::mesh_counts::for_mesh(const glub::t & t, unsigned m) {
  struct mesh_counts res {};
  for (auto & p : ::mesh(t, m).primitives) {
    if (p.mode != glub::primitive_mode::triangles) die("unsupported primitive mode");
    if (p.indices < 0) die("missing indices in mesh");

    for (auto &[key, a] : p.attributes) {
      if (key != "POSITION") continue;
      res.v_count += accessor_vec3(t, a).count;
    }

    res.i_count += ::accessor(t, p.indices).count;
  }
  return res;
}

