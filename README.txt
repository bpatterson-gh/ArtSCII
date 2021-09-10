ArtSCII, created by Bruce Patterson (contact@bpatterson.dev)
Generates high-quality ASCII art from images.

Requirements:
  64-bit Windows with .NET Framework version 4.5 OR 64-bit Linux with Mono

Usage: ArtSCII "input" "output" [optional parameters]
 input | File path to a BMP, GIF, JPG, PNG, or TIFF file.
 output | File path to save the output. The extension determines the output type.
        | Valid extensions are .BMP .GIF .HTM .HTML .JPG .JPEG .PNG .TIF and .TIFF
        | If no matching extension is found, BMP format will be used.
        | Please note that it is not recommended to generate HTML files from large images for performance reasons.
 optional parameters:
  -font "name" | Specifies the font used. If this is not supplied or cannot be found, a generic monospace font is used.
  -fontsize <n> | Sets the font size in pixels. <n> Must be an integer. Default is 12 pixels.
  -grey | Produces a greyscale output.
  -logmode <n> | Specifies the console log mode. 0 = Silent, 1 = Errors only, 2 = Errors and Warnings, 3 = All. Default is 3.
  -nocl | Disables OpenCL.
  -overlap <n> | Specifies the overlap multiplier. This will increase or decrease the font size used to render the output without changing the character spacing. Cannot be used with HTML outputs.
  -scale <n> | Scales the output by <n>.

	  
  This information can also be found by running ArtSCII with no arguments.
