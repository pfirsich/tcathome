#include <cstddef>
#include <cstdint>

namespace memtrack {
void track(void* ptr, size_t size);
uint32_t save(); // save and return id of snapshot
void restore(uint32_t id);
}
