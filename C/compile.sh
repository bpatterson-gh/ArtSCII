#!/bin/bash
mkdir obj
gcc -I/usr/include -c "src/addimg.c" -o "obj/addimg.o" -std=gnu99 -m64 -fPIC &&
gcc -I/usr/include -c "src/artscii.c" -o "obj/artscii.o" -std=gnu99 -m64 -fPIC &&
gcc -I/usr/include -c "src/charactermatch.c" -o "obj/charactermatch.o" -std=gnu99 -m64 -fPIC &&
gcc -I/usr/include -c "src/convolve.c" -o "obj/convolve.o" -std=gnu99 -m64 -fPIC &&
gcc -I/usr/include -c "src/debug.c" -o "obj/debug.o" -std=gnu99 -m64 -fPIC &&
gcc -I/usr/include -c "src/mult.c" -o "obj/mult.o" -std=gnu99 -m64 -fPIC &&
gcc -I/usr/include -c "src/nocl.c" -o "obj/nocl.o" -std=gnu99 -m64 -fPIC &&
gcc -I/usr/include -c "src/nocl_charactermatch.c" -o "obj/nocl_charactermatch.o" -std=gnu99 -m64 -fPIC &&
gcc -I/usr/include -c "src/nocl_convolve.c" -o "obj/nocl_convolve.o" -std=gnu99 -m64 -fPIC &&
gcc -L/usr/lib -shared -o "artscii.so" "obj/addimg.o" "obj/artscii.o" "obj/charactermatch.o" "obj/convolve.o" "obj/debug.o" "obj/mult.o" "obj/nocl.o" "obj/nocl_charactermatch.o" "obj/nocl_convolve.o" -lOpenCL -std=gnu99 -m64 &&
echo "Compiled artscii.so successfully"
