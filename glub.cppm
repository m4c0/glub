#pragma leco add_impl glub_parse
export module glub;
export import :objects;

namespace glub {
  export struct error {
    const char * what;
  };

  export t parse(const char * data, unsigned size);
}
