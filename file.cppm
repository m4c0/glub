module;
#include <stdio.h>

export module file;
import no;

static_assert(sizeof(unsigned) == 4);

export class file : no::no {
  FILE * m_f {};

  file(const char * name, const char * mode)
    : m_f { fopen(name, mode) }
  {
    if (!m_f) throw not_found {};
  }

public:
  struct not_found {};
  struct read_error {};
  
  auto read_u32() {
    unsigned res {};
    if (fread(&res, sizeof(unsigned), 1, m_f) != 1) throw read_error {};
    return res;
  }

  [[nodiscard]] static file open_for_reading(const char * name) {
    return file { name, "rb" };
  }
};

