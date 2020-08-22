using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using Bitmap = System.Drawing.Bitmap;
using Color = System.Drawing.Color;
using File = System.IO.File;
using Font = System.Drawing.Font;
using FontStyle = System.Drawing.FontStyle;
using FStream = System.IO.FileStream;
using Graphics = System.Drawing.Graphics;
using ImageFormat = System.Drawing.Imaging.ImageFormat;
using PointF = System.Drawing.PointF;
using SolidBrush = System.Drawing.SolidBrush;

namespace ArtSCII
{
    class Program
    {
        static string inPath = string.Empty;
        static string outPath = string.Empty;
        static string fontName = string.Empty;
        static int fontSize = 8;
        public static uint charWidth, charHeight;
        static float scale = 1;
        static uint logMode = 3;
        static bool html;
        public static bool grey, openCL;
        static ImageFormat outputFmt;
        static AsciiFont asciiFont;

        /// <summary>
        /// Parses command line arguments and sets values.
        /// Also checks the validity of certain arguments.
        /// </summary>
        /// <param name="args">Command line arguments</param>
        /// <returns>Error string or empty string if successful. The error string can be "NoError" to
        /// signify that the program should not continue, but no error should be printed.</returns>
        static string ParseArgs(string[] args, out Bitmap input, out FStream output)
        {
            input = null;
            output = null;
            int size = args.Length;
            if (size == 0)
            {
                args = new string[] { "help" };
                size = 1;
            }
            for (int i = 0; i < size; i++)
            {
                switch (args[i].ToLower())
                {
                    case "-font":
                        fontName = args[++i];
                        break;
                    case "-fontsize":
                        if (!int.TryParse(args[++i], out fontSize)) return "Font size must be an integer.";
                        break;
                    case "-grey":
                        grey = true;
                        break;
                    case "help":
                    case "-help":
                    case "-h":
                        Console.WriteLine("\nUsage: ArtSCII \"input\" \"output\" [optional parameters]\n" +
                            " input | File path to a BMP, GIF, JPG, PNG, or TIFF file.\n" +
                            " output | File path to save the output. The extension determines the output type.\n" +
                            "        | Valid extensions are .BMP .GIF .HTM .HTML .JPG .JPEG .PNG .TIF and .TIFF\n" +
                            "        | If no matching extension is found, BMP format will be used.\n" +
                            "        | Please note that it is not recommended to generate HTML files from large images for performance reasons.\n" +
                            " optional parameters:\n" +
                            "  -font \"name\" | Specifies the font used. If this is not supplied or cannot be found, a generic monospace font is used.\n" +
                            "  -fontsize <n> | Sets the font size in pixels. <n> Must be an integer. Default is 8 pixels.\n" +
                            "  -grey | Produces a greyscale output.\n" +
                            "  -logmode <n> | Specifies the console log mode. 0 = Silent, 1 = Errors only, 2 = Errors and Warnings, 3 = All. Default is 3.\n" +
                            "  -scale <n> | Scales the output by <n>."
                            );
                        return "NoError";
                    case "-logmode":
                        if (!uint.TryParse(args[++i], out logMode) || logMode > 3) return "Log mode must be a number between 0 and 3.";
                        break;
                    case "-scale":
                        if (!float.TryParse(args[++i], out scale)) return "Scale must be a number.";
                        scale = 1 / scale;
                        break;
                    default:
                        if (inPath == string.Empty) inPath = args[i];
                        else if (outPath == string.Empty) outPath = args[i];
                        break;
                }
            }
            if (outPath == string.Empty) return "You must provide an input and output file path.";
            if (fontSize * scale < 4) return "The scale is too large for this font size. Either increase the font size or decrease the scale.";

            SetOutputFileType(outPath);

            return OpenFiles(out input, out output);
        }

        /// <summary>
        /// Sets the output file type based on the file extension given.
        /// </summary>
        /// <param name="outPath">Output file path</param>
        static void SetOutputFileType(string outPath)
        {
            outPath = outPath.ToLower();
            html = (outPath.EndsWith(".htm") || outPath.EndsWith(".html"));
            if (html) return;
            else if (outPath.EndsWith(".bmp")) outputFmt = ImageFormat.Bmp;
            else if (outPath.EndsWith(".gif")) outputFmt = ImageFormat.Gif;
            else if (outPath.EndsWith(".jpg")) outputFmt = ImageFormat.Jpeg;
            else if (outPath.EndsWith(".jpeg")) outputFmt = ImageFormat.Jpeg;
            else if (outPath.EndsWith(".png")) outputFmt = ImageFormat.Png;
            else if (outPath.EndsWith(".tif")) outputFmt = ImageFormat.Tiff;
            else if (outPath.EndsWith(".tiff")) outputFmt = ImageFormat.Tiff;
            else
            {
                Log(LogType.Warning, "Could not detect output file type. Defaulting to BMP.");
                outputFmt = ImageFormat.Bmp;
            }
        }

        /// <summary>
        /// Opens the necessary files and handles a bunch of exceptions.
        /// </summary>
        /// <param name="bmp">Bitmap representing the input image</param>
        /// <param name="fstream">Output file stream</param>
        static string OpenFiles(out Bitmap bmp, out FStream fstream)
        {
            bmp = null;
            fstream = null;
            try
            {
                bmp = (Bitmap)Bitmap.FromFile(inPath);
                if (grey) bmp = ToGreyscale(bmp);
                if (File.Exists(outPath)) File.Delete(outPath);
                fstream = File.OpenWrite(outPath);
            }
            catch (OutOfMemoryException)
            {
                return "The file at \"" + inPath + "\" is not a valid BMP, GIF, JPEG, PNG, or TIFF format.";
            }
            catch (UnauthorizedAccessException)
            {
                return "You do not have access to \"" + outPath + "\".";
            }
            catch (System.IO.DirectoryNotFoundException)
            {
                return "The path: \"" + outPath + "\" is invalid. The directory does not exist.";
            }
            catch (System.IO.FileNotFoundException)
            {
                return "The path: \"" + inPath + "\" does not exist.";
            }
            catch (System.IO.PathTooLongException)
            {
                return "The path: \"" + outPath + "\" is too long.";
            }
            catch (Exception e)
            {
                if (e is ArgumentException || e is NotSupportedException)
                {
                    return "The path: " + (bmp == null ? "\"" + inPath + "\"" : "\"" + outPath + "\"") +
                        " is invalid.";
                }
                throw;
            }
            return string.Empty;
        }

        /// <summary>
        /// Converts an image to greyscale.
        /// </summary>
        /// <param name="bmp">Image to convert</param>
        /// <returns>Greyscale Bitmap</returns>
        static Bitmap ToGreyscale(Bitmap bmp)
        {
            Bitmap grey = new Bitmap(bmp.Width, bmp.Height);
            Color c;
            int pixel;
            for (int x = 0; x < bmp.Width; x++)
            {
                for (int y = 0; y < bmp.Height; y++)
                {
                    c = bmp.GetPixel(x, y);
                    pixel = (int)Math.Min((Math.Max(Math.Max(c.R, c.G), c.B) +
                        Math.Min(Math.Min(c.R, c.G), c.B) * 0.5f), 255);
                    grey.SetPixel(x, y, Color.FromArgb(pixel, pixel, pixel));
                }
            }
            return grey;
        }

        /// <summary>
        /// Creates a Bitmap containing the ASCII output.
        /// </summary>
        /// <param name="inputW">Width of the input image</param>
        /// <param name="inputH">Height of the input image</param>
        /// <param name="ascii">List of ASCII characters and colors</param>
        /// <param name="fontName">Font name</param>
        /// <param name="fontSize">Font size</param>
        /// <returns>Bitmap</returns>
        static Bitmap FormatOutputBmp(int inputW, int inputH, List<Tuple<char, Color>> ascii, string fontName, int fontSize)
        {
            Bitmap bmp = new PixelSet((uint)inputW, (uint)inputH, 0x11).ToBitmap();
            SolidBrush b = new SolidBrush(Color.White);
            Graphics g = Graphics.FromImage(bmp);

            float lineW = -1;
            int numLines = 1;
            for (int i = 0; i < ascii.Count; i++)
            {
                if (ascii[i].Item1 == '\n')
                {
                    if (numLines < 2) lineW = i;
                    numLines++;
                }
            }
            if (lineW == -1) lineW = ascii.Count;

            float padW = (inputW - (lineW * charWidth)) * 0.5f;
            PointF textPos = new PointF(padW, (inputH - (charHeight * numLines)) * 0.5f);

            for (int i = 0; i < ascii.Count; i++)
            {
                string str = string.Empty + ascii[i].Item1;
                b.Color = ascii[i].Item2;

                g.DrawString(str, asciiFont.Font, b, textPos);

                if (str == "\n")
                {
                    textPos.X = padW;
                    textPos.Y += charHeight;
                }
                else textPos.X += charWidth;
            }

            g.Flush();
            b.Dispose();

            return bmp;
        }

        /// <summary>
        /// Creates an HTML file containing the ASCII output.
        /// </summary>
        /// <param name="ascii">List of ASCII characters and colors</param>
        /// <returns>bytes of an HTML file</returns>
        static byte[] FormatOutputHtm(List<Tuple<char, Color>> ascii)
        {
            Dictionary<Color, int> colorCounts = new Dictionary<Color, int>();
            Dictionary<Color, string> colorIDs = new Dictionary<Color, string>();
            Color c;
            for (int i = 0; i < ascii.Count(); i++)
            {
                c = ascii[i].Item2;
                if (colorCounts.ContainsKey(c))
                {
                    colorCounts[c]++;
                    if (!colorIDs.ContainsKey(c)) colorIDs.Add(c, i.ToString("x"));
                }
                else colorCounts.Add(c, 1);
            }
            string html = "<!-- Generated by ArtSCII on " + DateTime.Now.ToString() + " -->\n" +
            "<!-- For more information, visit https://bpatterson.dev/projects/ArtSCII -->\n\n" +
            "<!DOCTYPE html><html><head><style>\n";
            Color[] colors = colorCounts.Keys.ToArray();
            for (int i = 0; i < colors.Length; i++)
            {
                c = colors[i];
                if (!colorIDs.ContainsKey(c)) continue;
                html += ".c" + colorIDs[c] + "{color:#" +
                    c.R.ToString("x2") + c.G.ToString("x2") + c.B.ToString("x2") + ";}\n";
            }
            html += "\nbody{font-family:\"" + asciiFont.FontName + "\",monospace;" +
                "font-size:" + (int)(asciiFont.FontSize / scale) + 
                "px;background-color:#111111;white-space:pre;}\n</style></head><body>\n";
            Color last = Color.FromArgb(0, 0, 0, 0);
            string colorStr;
            string ch;
            bool newLine = true;
            for (int i = 0; i < ascii.Count; i++)
            {
                ch = ascii[i].Item1.ToString();
                if (ch == "&") ch = "&amp;";
                else if (ch == ">") ch = "&gt;";
                else if (ch == "<") ch = "&lt;";
                else if (ch == "\"") ch = "&quot;";
                else if (ch == "\'") ch = "&#39;";
                c = ascii[i].Item2;
                if (ch == "\n")
                {
                    html += "</span><br>";
                    last = Color.FromArgb(0, 0, 0, 0);
                    newLine = true;
                }
                else if (!newLine && c == last) html += ch;
                else
                {
                    newLine = false;
                    if (colorIDs.ContainsKey(c)) colorStr = "<span class='c" + colorIDs[c] + "'>";
                    else colorStr = "<span style='color:#" +
                            c.R.ToString("x2") + c.G.ToString("x2") + c.B.ToString("x2") + ";'>";

                    if (last.A == 0) html += colorStr + ch;
                    else html += "</span>" + colorStr + ch;
                    last = c;
                }
            }
            html += "\n</body></html>\n";
            byte[] bytes = Encoding.UTF8.GetBytes(html);
            return bytes;
        }

        /// <summary>
        /// Log Types. Error types are sent to Console.Error, others are sent to Console.
        /// </summary>
        public enum LogType
        {
            Done,
            Error,
            Info,
            Warning,
        }

        /// <summary>
        /// Logs messages to the console.
        /// </summary>
        /// <param name="type">Type of message</param>
        /// <param name="message">Message to log. Values such as {0} are handled appropriately.</param>
        /// <param name="consoleParams">Used to handle values such as {0} in message.</param>
        public static void Log(LogType type, string message, params object[] consoleParams)
        {
            if (logMode == 0 || 
                (logMode == 1 && type != LogType.Error) ||
                (logMode == 2 && type != LogType.Error && type != LogType.Warning)) return;
            ConsoleColor dftColor = Console.ForegroundColor;
            switch (type) {
                case LogType.Done:
                    Console.ForegroundColor = ConsoleColor.Green;
                    Console.Write("✓ ");
                    break;
                case LogType.Error:
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.Error.Write("X ");
                    break;
                case LogType.Info:
                    Console.ForegroundColor = ConsoleColor.Blue;
                    Console.Write("■ ");
                    break;
                case LogType.Warning:
                    Console.ForegroundColor = ConsoleColor.Yellow;
                    Console.Write("! ");
                    break;
            }
            Console.ForegroundColor = dftColor;
            if (type == LogType.Error) Console.Error.WriteLine(message, consoleParams);
            else Console.WriteLine(message, consoleParams);
        }

        /// <summary>
        /// Converts an image to ASCII art.
        /// </summary>
        /// <param name="args">Command line arguments</param>
        static void Main(string[] args)
        {
            Bitmap input;
            FStream output;

            string err = ParseArgs(args, out input, out output);
            if (err.Length > 0)
            {
                if (err != "NoError") Log(LogType.Error, err);
                return;
            }

            openCL = OCL.OCL_Init();
            if (!openCL) return;
            asciiFont = new AsciiFont(fontName, (int)(fontSize * scale));
            Log(LogType.Info, "Font is \"{0}\" ({1}px)", asciiFont.FontName, (int)(fontSize * scale));

            var en = asciiFont.characters.Values.GetEnumerator();
            en.MoveNext();
            charWidth = en.Current.Width;
            charHeight = en.Current.Height;

            Log(LogType.Info, "Converting to ascii...");
            List<Tuple<char, Color>> ascii = OCL.ToAscii((new PixelSet(input) * 0.75f) + 64, asciiFont);
            if (!openCL) return;

            Log(LogType.Info, "Saving \"{0}\"...", output.Name);
            if (html)
            {
                byte[] bytes = FormatOutputHtm(ascii);
                output.Write(bytes, 0, bytes.Length);
            }
            else
            {
                Bitmap outBmp = FormatOutputBmp(input.Width, input.Height, ascii, fontName, (int)(fontSize * scale));
                outBmp.Save(output, outputFmt);
                outBmp.Dispose();
            }

            output.Close();
            output.Dispose();
            input.Dispose();
            OCL.OCL_Cleanup();
        }
    }
}
