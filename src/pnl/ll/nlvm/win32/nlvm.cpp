//
// Created by FCWY on 24-12-28.
//
#include <project-nl.h>
import pnl.ll;
import nlvm.common;
using namespace pnl::ll;
using namespace nlvm;
using namespace std::chrono_literals;

int main(const int argc, char* argv[]){
    try {
        auto process = Process{&memory};
        prepare(process, argc, const_cast<const char**>(argv));
        process.resume();
        process.terminate();
    }catch (std::exception& e) {
        std::println(std::cerr, "VM was killed with message: {}", e.what());
    }
}
