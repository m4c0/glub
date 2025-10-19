export module glub:objects;
import hai;
import jute;

export namespace glub {
  enum class accessor_comp_type : int {
    nil = -1,
    byte = 5120,
    unsigned_byte = 5121,
    shrt = 5122,
    unsigned_shrt = 5123,
    unsigned_int = 5125,
    flt = 5126,
  };
  enum class buffer_view_type : int {
    nil = -1,
    array_buffer = 34962,
    element_array_buffer = 34963,
  };
  enum class primitive_mode : int {
    points = 0,
    lines = 1,
    line_loop = 2,
    line_strip = 3,
    triangles = 4,
    triangle_strip = 5,
    triangle_fan = 6,
  };

  struct accessor {
    int buffer_view = -1;
    int byte_offset = 0;
    accessor_comp_type component_type;
    bool normalised = false;
    int count;
    jute::heap type;
    hai::array<float> max {};
    hai::array<float> min {};
    jute::heap name {};
  };
  struct attribute {
    jute::heap key {};
    int accessor = -1;
  };
  struct buffer {
    jute::heap uri {};
    int byte_length;
    jute::heap name {};
  };
  struct buffer_view {
    int buffer;
    int byte_offset = 0;
    int byte_length;
    int byte_stride = -1;
    buffer_view_type target = buffer_view_type::nil;
    jute::heap name {};
  };
  struct primitive {
    hai::array<attribute> attributes;
    int indices = -1;
    int material = -1;
    primitive_mode mode = primitive_mode::triangles;
    hai::array<jute::heap> targets {};
  };
  struct material {
    jute::heap name {};
  };
  struct mesh {
    jute::heap name {};
    hai::array<primitive> primitives {};
    hai::array<float> weights {};
  };
  struct node {
    jute::heap name {};
    int camera = -1;
    int mesh = -1;
    int skin = -1;
    hai::array<int> children {};
    hai::array<float> matrix {};
    hai::array<float> rotation {};
    hai::array<float> scale {};
    hai::array<float> translation {};
    hai::array<float> weights {};
  };
  struct scene {
    jute::heap name {};
    hai::array<int> nodes {};
  };
  struct skin {
    int inverse_bind_mat = -1;
    int skeleton = -1;
    hai::array<int> joints {};
    jute::heap name {};
  };
  struct t {
    hai::array<accessor> accessors {};
    hai::array<buffer> buffers {};
    hai::array<buffer_view> buffer_views {};
    hai::array<material> materials {};
    hai::array<mesh> meshes {};
    hai::array<node> nodes {};
    hai::array<scene> scenes {};
    hai::array<skin> skins {};
    int scene = -1;

    jute::view data;
  };
}
