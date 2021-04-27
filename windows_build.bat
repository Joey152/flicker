@echo off
setlocal
    set CC="clang-cl"
    set CFLAGS="--target=x86_64-pc-windows-msvc"
    meson . build
endlocal
