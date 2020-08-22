ArtSCII, created by Bruce Patterson (contact@bpatterson.dev)
Generates high-quality ASCII art from images.

Requirements:
  64-bit Windows with .NET Framework version 4.7.2
  OpenCL compatible CPU or GPU
  
Compiling:
  The C# portion of the project is a Visual Studio solution and can be compiled via VS Community 2019.
  The C portion can be compiled with the following gcc commands:
    gcc -I. -c "src\addimg.c" -o "obj\addimg.o" -lmingw32 -lSDL2main -lSDL2 -mwindows -lopencl -std=gnu99 -m64
    gcc -I. -c "src\addscl.c" -o "obj\addscl.o" -lmingw32 -lSDL2main -lSDL2 -mwindows -lopencl -std=gnu99 -m64
    gcc -I. -c "src\artscii.c" -o "obj\artscii.o" -lmingw32 -lSDL2main -lSDL2 -mwindows -lopencl -std=gnu99 -m64
    gcc -I. -c "src\charactermatch.c" -o "obj\charactermatch.o" -lmingw32 -lSDL2main -lSDL2 -mwindows -lopencl -std=gnu99 -m64
    gcc -I. -c "src\compare.c" -o "obj\compare.o" -lmingw32 -lSDL2main -lSDL2 -mwindows -lopencl -std=gnu99 -m64
    gcc -I. -c "src\convolve.c" -o "obj\convolve.o" -lmingw32 -lSDL2main -lSDL2 -mwindows -lopencl -std=gnu99 -m64
    gcc -I. -c "src\debug.c" -o "obj\debug.o" -lmingw32 -lSDL2main -lSDL2 -mwindows -lopencl -std=gnu99 -m64
    gcc -I. -c "src\mult.c" -o "obj\mult.o" -lmingw32 -lSDL2main -lSDL2 -mwindows -lopencl -std=gnu99 -m64
	gcc -L. -shared -o "artscii.dll" "obj\addimg.o" "obj\addscl.o" "obj\artscii.o" "obj\charactermatch.o" "obj\compare.o" "obj\convolve.o" "obj\debug.o" "obj\mult.o" -lmingw32 -lSDL2main -lSDL2 -mwindows -lopencl -std=gnu99 -m64
  These commands are included in C\compile.bat for convenience.

Usage:
  .\ArtSCII "input" "output" [optional parameters]
    input | File path to a BMP, GIF, JPG, PNG, or TIFF file.
    output | File path to save the output. The extension determines the output type.
           | Valid extensions are .BMP .GIF .HTM .HTML .JPG .JPEG .PNG .TIF and .TIFF
           | If no matching extension is found, BMP format will be used.
           | Please note that it is not recommended to generate HTML files from large images for performance reasons.
    optional parameters:
      -font "name" | Specifies the font used. If this is not supplied or cannot be found, a generic monospace font is used.
      -fontsize <n> | Sets the font size in pixels. <n> Must be an integer. Default is 8 pixels.
      -grey | Produces a greyscale output.
      -logmode <n> | Specifies the console log mode. 0 = Silent, 1 = Errors only, 2 = Errors and Warnings, 3 = All. Default is 3.
      -scale <n> | Scales the output by <n>.
	  
  This information can also be found by running artscii in a cmd window with no arguments.
