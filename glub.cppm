#pragma leco add_impl glub_parse
#pragma leco add_impl glub_vertex
export module glub;
export import :objects;

namespace glub {
  export struct error {
    const char * what;
  };

  export t parse(const char * data, unsigned size);

  export struct mesh_counts {
    unsigned v_count {};
    unsigned i_count {};

    static mesh_counts for_all_meshes(const t & t);
    static mesh_counts for_mesh(const t & t, unsigned m);
  };
}
