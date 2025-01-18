//
// Created by FCWY on 25-1-2.
//
// ReSharper disable CppDFAUnreadVariable
// ReSharper disable CppDFAUnusedValue
import pnl.ll;
import <iostream>;
import <ranges>;


using namespace pnl::ll;


namespace RTUObject {
    void maker(Thread& thr) noexcept {
        // pop RTUObject cls
        thr.waste();
        // pop RTUObject this
        thr.waste();
    }
    void collector(Thread& thr) noexcept {
        // pop RTUObject this
        thr.waste();
    }
}

namespace RTTObject {
    void maker(Thread& thr) noexcept {
        const auto rtt_cls_addr = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto rtt_this_addr = std::get<VirtualAddress>(thr.vm__pop_top());

        auto& self = thr.deref<>(rtt_this_addr);

        new (&self) runtime::RTTObject {rtt_cls_addr};
    }
    void collector(Thread& thr) noexcept {
        // pop RTTObject this
        thr.waste();
    }
}

namespace Type {
    void maker(Thread& thr) noexcept {
        const auto type_cls_addr = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto type_this_addr = std::get<VirtualAddress>(thr.vm__pop_top());

        auto& self = thr.deref<>(type_this_addr);

        const auto rtt_support = std::get<Bool>(thr.vm__pop_top());
        const auto instance_size = std::get<Int>(thr.vm__pop_top());
        const auto maker = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto collector = std::get<VirtualAddress>(thr.vm__pop_top());

        new (&self) runtime::Type {
            {type_cls_addr},
            rtt_support,
            instance_size,
            maker,
            collector
        };
    }
    void collector(Thread& thr) noexcept {
        // pop Type this
        thr.waste();
    }
}

namespace NamedType {
    void maker(Thread& thr) noexcept {
        const auto n_type_cls_addr = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto n_type_this_addr = std::get<VirtualAddress>(thr.vm__pop_top());

        auto& self = thr.deref<>(n_type_this_addr);

        const auto rtt_support = std::get<Bool>(thr.vm__pop_top());
        const auto instance_size = std::get<Int>(thr.vm__pop_top());
        const auto maker = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto collector = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto name = std::get<VirtualAddress>(thr.vm__pop_top());

        new (&self) runtime::NamedType {
                {{n_type_cls_addr},
                rtt_support,
                instance_size,
                maker,
                collector},
                name
            };
    }
    void collector(Thread& thr) noexcept {
        // pop NamedType this
        thr.waste();
    }
}

namespace ArrayType {
    void maker(Thread& thr) noexcept {
        const auto n_type_cls_addr = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto n_type_this_addr = std::get<VirtualAddress>(thr.vm__pop_top());

        auto& self = thr.deref<runtime::NamedType>(n_type_this_addr);

        const auto rtt_support = std::get<Bool>(thr.vm__pop_top());
        const auto instance_size = std::get<Int>(thr.vm__pop_top());
        const auto maker = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto collector = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto elem_type = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto length = std::get<Long>(thr.vm__pop_top());

        new (&self) runtime::ArrayType {{
            {n_type_cls_addr},
            rtt_support,
            instance_size,
            maker,
            collector},
            elem_type,
            length
        };
    }
    void collector(Thread& thr) noexcept {
        thr.waste();
    }

    void generic_constructor(Thread& thr) noexcept {
        const auto this_addr = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto& arr_type = thr.deref<runtime::ArrayType>(this_addr);
        const auto& elem_type = thr.deref<runtime::Type>(arr_type.elem_type);

        const auto place = std::get<VirtualAddress>(thr.vm__pop_top());
        thr.deref<runtime::RTTObject>(place) = {this_addr};

        for (const auto i: std::views::iota(0, arr_type.length)) {
            thr.eval_stack.emplace(TFlag<VirtualAddress>,
                place.offset_shift(
                    sizeof(runtime::RTTObject) + i * elem_type.instance_size
                )
            );
            thr.call_function(thr.deref<FFamily>(elem_type.maker), 0);
        }
    }
    void generic_destructor(Thread& thr) noexcept {
        const auto place = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto& inst = thr.deref<runtime::RTTObject>(place);
        const auto& arr_type = thr.deref<runtime::ArrayType>(inst.type);
        const auto& elem_type = thr.deref<runtime::Type>(arr_type.elem_type);

        for (const auto i: std::views::iota(0, arr_type.length)) {
            thr.eval_stack.emplace(TFlag<VirtualAddress>,
                place.offset_shift(
                    sizeof(runtime::RTTObject) + i * elem_type.instance_size
                )
            );
            thr.call_function(thr.deref<FFamily>(elem_type.collector), 0);
        }
    }
}

namespace MemberInfo {
    void maker(Thread& thr) noexcept {
        const auto this_addr = std::get<VirtualAddress>(thr.vm__pop_top());

        auto& self = thr.deref<>(this_addr);

        const auto name = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto type = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto offset = std::get<Long>(thr.vm__pop_top());

        new (&self) runtime::MemberInfo {
            name,
            type,
            offset
        };
    }
    void collector(Thread& thr) noexcept {
        thr.waste();
    }
}

namespace Class {
    void maker(Thread& thr) noexcept {
        const auto cls_cls_addr = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto cls_this_addr = std::get<VirtualAddress>(thr.vm__pop_top());

        auto& self = thr.deref<>(cls_this_addr);

        const auto rtt_support = std::get<Bool>(thr.vm__pop_top());
        const auto instance_size = std::get<Int>(thr.vm__pop_top());
        const auto maker = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto collector = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto name = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto members = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto methods = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto static_members = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto static_methods = std::get<VirtualAddress>(thr.vm__pop_top());



        new (&self) runtime::Class {{{{
            cls_cls_addr},
            rtt_support,
            instance_size,
            maker,
            collector},
            name},
            members,
            methods,
            static_members,
            static_methods
        };
    }
    void collector(Thread& thr) noexcept {
        // pop NamedType this
        thr.waste();
    }
}

template<typename T>
static void maker(Thread& thr) noexcept {
    const auto this_addr = std::get<VirtualAddress>(thr.vm__pop_top());

    new (&thr.deref<>(this_addr)) T {std::get<T>(thr.vm__pop_top())};
}

template<typename T>
static void collector(Thread& thr) noexcept {
    thr.waste();
}

#define OP(TYPE) \
namespace TYPE {\
    void maker(Thread& thr) noexcept {\
        ::maker<base::TYPE>(thr);\
    }\
    void collector(Thread& thr) noexcept{\
        ::collector<base::TYPE>(thr);\
    }\
}

OP(Bool)
OP(Char)
OP(Byte)
OP(Int)
OP(Long)
OP(Float)
OP(Double)


namespace Unit {
    void maker(Thread&) noexcept {
        std::unreachable();
    }
    void collector(Thread&) noexcept {
        std::unreachable();
    }
}

namespace Address {
    void maker(Thread& thr) noexcept {
        ::maker<VirtualAddress>(thr);
    }
    void collector(Thread& thr) noexcept{
        ::collector<VirtualAddress>(thr);
    }
}

namespace Instruction {
    void maker(Thread& thr) noexcept {
        const auto this_addr = std::get<VirtualAddress>(thr.vm__pop_top());

        const auto code = static_cast<OPCode>(static_cast<UByte>(std::get<base::Byte>(thr.vm__pop_top())));
        const auto arg = static_cast<std::uint32_t>(std::get<base::Int>(thr.vm__pop_top()));

        new (&thr.deref<>(this_addr)) vm::Instruction {code, arg};

    }
    void collector(Thread& thr) noexcept{
        thr.waste();
    }
}


namespace FOverride {
    void maker(Thread& thr) noexcept {
        const auto type_cls_addr = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto type_this_addr = std::get<VirtualAddress>(thr.vm__pop_top());

        auto& self = thr.deref<>(type_this_addr);

        const auto is_native = std::get<base::Bool>(thr.vm__pop_top());
        const auto target = std::get<VirtualAddress>(thr.vm__pop_top());

        new (&self) runtime::FOverride {
                    {type_cls_addr},
                    is_native,
                    target.content
                };
    }
    void collector(Thread& thr) noexcept {
        // pop FFamily this
        thr.waste();
    }
}

namespace FFamily {
    void maker(Thread& thr) noexcept {
        const auto type_cls_addr = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto type_this_addr = std::get<VirtualAddress>(thr.vm__pop_top());

        auto& self = thr.deref<>(type_this_addr);

        const auto base_addr = std::get<VirtualAddress>(thr.vm__pop_top());
        const auto overrides = std::get<VirtualAddress>(thr.vm__pop_top());

        new (&self) runtime::FFamily {
                {type_cls_addr},
                base_addr,
                overrides
            };
    }
    void collector(Thread& thr) noexcept {
        // pop FFamily this
        thr.waste();
    }
}

