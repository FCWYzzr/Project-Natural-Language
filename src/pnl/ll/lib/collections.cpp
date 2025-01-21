//
// Created by FCWY on 25-1-8.
//
module;
#include "project-nl.h"

module pnl.ll.collections;


namespace pnl::ll::collections::pmr{


Stack::Stack(MManager* const mem_res) noexcept:
    obj_offset(mem_res),
    dense_pool(mem_res) {}

Stack::Stack(const Stack &other) noexcept = default;

Stack::Stack(Stack &&other) noexcept:
    obj_offset(std::move(other.obj_offset)),
    dense_pool(std::move(other.dense_pool)) {}


void Stack::placeholder_push(const USize size) noexcept {
    obj_offset.push_back(dense_pool.size());
    dense_pool.resize(dense_pool.size() + size);
}

void Stack::waste_top() noexcept {
    assert(!obj_offset.empty(), VM_TEXT("stack underflow"));
    dense_pool.resize(obj_offset.back());
    obj_offset.pop_back();
}

void Stack::waste_since(const std::uint32_t milestone) noexcept {
    if (milestone == obj_offset.size())
        return;
    assert(milestone > obj_offset.size(), VM_TEXT("stack underflow"));
    while(obj_offset.size() > milestone) {
        obj_offset.pop_back();
    }

    waste_top();
}

std::uint32_t Stack::milestone() const noexcept {
    return obj_offset.size();
}

UByte* Stack::operator[](const USize id) noexcept {
    const auto offset = obj_offset[id];
    return &dense_pool[offset];
}

void Stack::reserve(const USize target_size) noexcept {
    dense_pool.resize(target_size);
}

void Stack::concat(const Stack &other) noexcept {

    reserve(dense_pool.size() + other.dense_pool.size());
    const auto base = dense_pool.size();
    obj_offset.reserve(obj_offset.size() + other.obj_offset.size());
    dense_pool.reserve(dense_pool.size() + other.dense_pool.size());

    obj_offset.append_range(other.obj_offset
        | std::views::transform([base](const auto off){return off + base;}));
    dense_pool.append_range(other.dense_pool);
}


}