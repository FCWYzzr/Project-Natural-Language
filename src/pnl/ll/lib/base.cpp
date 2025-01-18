//
// Created by FCWY on 25-1-8.
//
module pnl.ll.base;
void pnl::ll::conditions::assert(const bool condition, const Str &err_desc) noexcept {
    if (!condition) [[unlikely]] {
        std::wcerr << err_desc;
        std::abort();
    }
}

void pnl::ll::conditions::assert(const bool condition, const NStr &err_desc) noexcept {
    if (!condition) [[unlikely]] {
        std::cerr << err_desc;
        std::abort();
    }
}
