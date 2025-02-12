#include <cstddef>
#include <cstdint>

namespace memtrack {
uint32_t track(void* ptr, size_t size); // returns track id
uint32_t save(); // save and return new snapshot id
void restore(uint32_t snapshot_id);
void restore_to(uint32_t track_id, uint32_t snapshot_id, size_t offset, size_t size, void* dest);
void overwrite(uint32_t id);
}
