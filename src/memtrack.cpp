#include "memtrack.hpp"

#include <array>
#include <cassert>
#include <cstring>
#include <list>
#include <memory>
#include <vector>

#include <fmt/core.h>

struct TrackedRegion {
    void* ptr;
    size_t size;
};

struct RegionMemory {
    std::unique_ptr<std::byte[]> data;
    size_t size;
};

struct Snapshot {
    std::vector<RegionMemory> regions;
};

constexpr size_t ChunkSize = 64;
using Chunk = std::array<std::byte, ChunkSize>;

std::array<TrackedRegion, 8> tracked_regions = {};

std::list<Snapshot>& get_snapshots()
{
    static std::list<Snapshot> snaps;
    return snaps;
}

size_t num_tracked_regions()
{
    size_t i = 0;
    while (i < tracked_regions.size() && tracked_regions[i].ptr) {
        i++;
    }
    return i;
}

namespace memtrack {

uint32_t track(void* ptr, size_t size)
{
    const auto idx = num_tracked_regions();
    fmt::println("track {} bytes", size);
    assert(idx < tracked_regions.size());
    tracked_regions[idx] = TrackedRegion { ptr, size };
    return static_cast<uint32_t>(idx);
}

uint32_t save()
{
    const auto id = static_cast<uint32_t>(get_snapshots().size());
    auto& snap = get_snapshots().emplace_back();
    const auto num_regions = num_tracked_regions();
    snap.regions = std::vector<RegionMemory>(num_regions);
    for (size_t i = 0; i < num_regions; ++i) {
        auto& region = tracked_regions[i];
        snap.regions[i].data = std::make_unique<std::byte[]>(region.size);
        snap.regions[i].size = region.size;
        std::memcpy(snap.regions[i].data.get(), region.ptr, region.size);
    }
    return id;
}

void restore(uint32_t snapshot_id)
{
    assert(snapshot_id < get_snapshots().size());
    auto snap = get_snapshots().begin();
    std::advance(snap, snapshot_id);

    const auto num_regions = num_tracked_regions();
    assert(snap->regions.size() <= num_regions);
    for (size_t i = 0; i < num_regions; ++i) {
        auto& region = tracked_regions[i];
        assert(region.size == snap->regions[i].size);
        std::memcpy(region.ptr, snap->regions[i].data.get(), region.size);
    }
}

void restore_to(uint32_t track_id, uint32_t snapshot_id, size_t offset, size_t size, void* dest)
{
    assert(snapshot_id < get_snapshots().size());
    auto snap = get_snapshots().begin();
    std::advance(snap, snapshot_id);

    assert(track_id < snap->regions.size());
    std::memcpy(dest, snap->regions[track_id].data.get() + offset, size);
}

void overwrite(uint32_t id)
{
    assert(id < get_snapshots().size());
    auto snap = get_snapshots().begin();
    std::advance(snap, id);

    const auto num_regions = num_tracked_regions();
    assert(snap->regions.size() == num_regions);
    for (size_t i = 0; i < num_regions; ++i) {
        auto& region = tracked_regions[i];
        assert(region.size == snap->regions[i].size);
        std::memcpy(snap->regions[i].data.get(), region.ptr, region.size);
    }
}

}
