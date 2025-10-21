module glub;

template<typename T>
static auto cast(auto * acc, auto & bv, auto & t) {
  auto ptr = reinterpret_cast<const T *>(t.data.begin() + acc->byte_offset + bv.byte_offset);
  if ((void *)(ptr + acc->count) > t.data.end()) throw glub::error { "buffer overrun" };
  return ptr;
}
