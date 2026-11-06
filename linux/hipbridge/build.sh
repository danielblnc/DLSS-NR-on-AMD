#!/bin/sh
# Builds the Wine builtin amdhip64_7.dll (PE) + amdhip64_7.so (unix side) into out/.
set -e
cd "$(dirname "$0")"
mkdir -p inc out/x86_64-windows out/x86_64-unix
ln -sfn /usr/include/wine inc/wine
gcc -shared -fPIC -O2 -I/usr/include/wine/windows -I/usr/include -DWINE_UNIX_LIB \
    -o out/x86_64-unix/amdhip64_7.so unix.c -ldl
winegcc -b x86_64-windows -shared -O2 -I/usr/include/wine/windows -Iinc \
    amdhip64_7.spec pe.c -o out/x86_64-windows/amdhip64_7.dll -lntdll -lkernel32
winebuild --builtin out/x86_64-windows/amdhip64_7.dll
winegcc -b x86_64-windows -O2 -I/usr/include/wine/windows hiptest.c -o out/hiptest.exe -luser32
