//
// Created by FCWY on 25-1-20.
//

#ifndef PROJECT_NL_H
#define PROJECT_NL_H

#include <array>
#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <dlfcn.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <optional>
#include <queue>
#include <ranges>
#include <semaphore>
#include <shared_mutex>
#include <span>
#include <stack>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>


#define VM_TEXT(text) U##text

#define u16offsetof(T, M) static_cast<std::uint16_t>(offsetof(T, M))

#endif // PROJECT_NL_H