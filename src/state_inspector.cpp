#include <unordered_map>
#include <vector>
#include <string>
#include <cstring>

#include "imgui.h"
#include <fmt/core.h>

struct TypeMeta {
    size_t size = 0;
    size_t alignment = 0;
};

struct FieldDesc {
    std::string name;
    std::string type;
    size_t array_size = 0;
    size_t offset = 0;
};

struct TypeDesc {
    std::string name;
    std::vector<FieldDesc> fields;
    TypeMeta meta = {};
};

using TypeInfo = std::unordered_map<std::string, TypeDesc>;

TypeMeta get_builtin_type_meta(std::string_view name)
{
    if (name == "bool") {
        return { .size = 1, .alignment = 1 };
    } else if (name == "u32") {
        return { .size = 4, .alignment = 4 };
    } else if (name == "float") {
        return { .size = 4, .alignment = 4 };
    }
    return {};
}

TypeMeta get_meta(const TypeInfo& type_info, const std::string& type_name)
{
    const auto builtin = get_builtin_type_meta(type_name);
    if (builtin.size) {
        return builtin;
    }
    // Not a builtin type
    return type_info.at(type_name).meta;
}

size_t align(size_t offset, size_t alignment)
{
    const auto r = offset % alignment;
    return r > 0 ? offset + (alignment - r) : offset;
}

TypeMeta init_type_meta(TypeInfo& type_info, const std::string& name)
{
    auto& desc = type_info[name];
    if (desc.meta.size) {
        return desc.meta; // nothing to do
    }
    size_t offset = 0;
    for (auto& field : desc.fields) {
        if (!get_builtin_type_meta(field.type).size) {
            init_type_meta(type_info, field.type);
        }
        const auto field_meta = get_meta(type_info, field.type);
        assert(field_meta.size && field_meta.alignment);
        offset = align(offset, field_meta.alignment);
        field.offset = offset;

        const auto count = field.array_size ? field.array_size : 1;
        offset += field_meta.size * count;

        desc.meta.alignment = field_meta.alignment > desc.meta.alignment ? field_meta.alignment
                                                                         : desc.meta.alignment;
    }
    offset = align(offset, desc.meta.alignment);
    desc.meta.size = offset;
    return desc.meta;
}

// Hard-Coded for now
TypeInfo& get_type_info()
{
    static TypeInfo type_info;
    if (type_info.empty()) {
        type_info["Vec2"] = TypeDesc {
            .name = "Vec2",
            .fields = {
                FieldDesc { "x", "float" },
                FieldDesc { "y", "float" },
            },
        };
        type_info["Player"] = TypeDesc {
            .name = "Player",
            .fields = {
                FieldDesc { "pos", "Vec2" },
                FieldDesc { "sprite", "u32" },
            },
        };
        type_info["Chicken"] = TypeDesc {
            .name = "Chicken",
            .fields = {
                FieldDesc { "pos", "Vec2" },
                FieldDesc { "speed", "float" },
                FieldDesc { "anger", "float" },
                FieldDesc { "is_friendly", "bool" },
                FieldDesc { "alive", "bool" },
                FieldDesc { "sprite", "u32" },
            },
        };
        type_info["State"] = TypeDesc {
            .name = "State",
            .fields = {
                FieldDesc { "player", "Player" },
                FieldDesc { "chickens", "Chicken", 16 },
                FieldDesc { "num_chickens", "u32" },
                FieldDesc { "debug_circle_sprite", "u32" },
            },
        };
        init_type_meta(type_info, "State");
    }
    return type_info;
}

template <typename T>
T read(const std::byte* ptr)
{
    T v;
    std::memcpy(&v, ptr, sizeof(T));
    return v;
}

void show_variable(const TypeInfo& type_info, const std::string& type_name,
    const std::string& var_name, const std::byte* data)
{
    if (type_name == "bool") {
        ImGui::BulletText("(bool) %s: %s", var_name.c_str(), read<bool>(data) ? "true" : "false");
    } else if (type_name == "u32") {
        ImGui::BulletText("(u32) %s: %u", var_name.c_str(), read<uint32_t>(data));
    } else if (type_name == "float") {
        ImGui::BulletText("(float) %s: %f", var_name.c_str(), read<float>(data));
    } else {
        if (ImGui::TreeNodeEx(var_name.c_str(), ImGuiTreeNodeFlags_DefaultOpen, "(%s) %s",
                type_name.c_str(), var_name.c_str())) {

            const auto& desc = type_info.at(type_name);
            assert(!desc.fields.empty());
            for (const auto& field : desc.fields) {
                const auto field_type_meta = get_meta(type_info, field.type);
                assert(field_type_meta.size);
                if (field.array_size) {
                    for (size_t i = 0; i < field.array_size; ++i) {
                        const auto field_name = fmt::format("{}[{}]", field.name, i);
                        show_variable(type_info, field.type, field_name,
                            data + field.offset + i * field_type_meta.size);
                    }
                } else {
                    show_variable(type_info, field.type, field.name, data + field.offset);
                }
            }
            ImGui::TreePop();
        }
    }
}

void show_state_inspector(const void* state)
{
    const auto& type_info = get_type_info();
    ImGui::Begin("State Inspector", nullptr, 0);
    show_variable(type_info, "State", "state", reinterpret_cast<const std::byte*>(state));
    ImGui::End();
}
