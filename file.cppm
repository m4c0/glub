module;
#include <stdio.h>

export module file;
import no;

export class file : no::no {
  FILE * m_f {};

  file(const char * name, const char * mode)
    : m_f { fopen(name, mode) }
  {
    if (!m_f) throw not_found {};
  }

public:
  struct not_found {};

  [[nodiscard]] static file open_for_reading(const char * name) {
    return file { name, "rb" };
  }
};

