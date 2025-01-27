//
// Created by FCWY on 25-1-23.
//
#include <Windows.h>
#include <project-nl.h>
import pnl.ll;
import nlvm.common;
using namespace pnl::ll;
using namespace nlvm;
using namespace std::chrono_literals;


int WINAPI wWinMain(
    HINSTANCE,
    HINSTANCE,
    LPWSTR,
    int) {
    try {
        int argc;
        const auto w_argv = CommandLineToArgvW(GetCommandLineW(), &argc);

        if( w_argv == nullptr )
            throw std::runtime_error("CommandLineToArgvW failed");


        auto process = Process{&memory};
        prepare(process, argc, const_cast<const wchar_t**>(w_argv));
        process.resume();
        process.terminate();
    }catch (std::exception& e) {
        MessageBox(
            nullptr,
            e.what(),
            "VM was killed",
            MB_OK
        );
    }
    return 0;
}