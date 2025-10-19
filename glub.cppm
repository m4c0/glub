#pragma leco add_impl glub_parse
#pragma leco add_impl glub_vertex
export module glub;
export import :objects;
import dotz;

namespace glub {
  export struct error {
    const char * what;
  };

  export t parse(const char * data, unsigned size);

  export struct mesh_counts {
    unsigned v_count {};
    unsigned i_count {};

    static mesh_counts for_all_meshes(const t & t);
    static void for_each(const t & t, hai::fn<void, mesh_counts> fn);
  };

  export void load_all_indices(const t & t, unsigned short * ptr);
  export void load_all_normals(const t & t, dotz::vec3 * ptr);
  export void load_all_uvs(const t & t, dotz::vec2 * ptr);
  export void load_all_vertices(const t & t, dotz::vec3 * ptr);
}
