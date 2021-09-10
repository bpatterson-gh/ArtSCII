using System;
using System.Collections.Generic;

using Bitmap = System.Drawing.Bitmap;
using Context = Cairo.Context;
using FontFamily = System.Drawing.FontFamily;
using FontSlant = Cairo.FontSlant;
using FontStyle = System.Drawing.FontStyle;
using FontWeight = Cairo.FontWeight;
using Format = Cairo.Format;
using Graphics = System.Drawing.Graphics;
using GraphicsUnit = System.Drawing.GraphicsUnit;
using ImageSurface = Cairo.ImageSurface;
using TextExtents = Cairo.TextExtents;

namespace ArtSCII
{
    class AsciiFont
    {
        /// <summary>
        /// Creates a string of all characters from 1,33 - 255.
        /// The first character is the 'missing character', which is used as a filter in CreateBitmaps.
        /// </summary>
        /// <returns>ASCII String</returns>
        private static string InitAsciiString() {
            string s = string.Empty + (char)1;
            for (int c = 32; c < 256; c++)
            {
                s += (char)c;
            }
            return s;
        }
        public static string asciiString = InitAsciiString();

        public Dictionary<char, PixelSet> characters;

        /// <summary>
        /// Gets a map of array indexes to characters.
        /// </summary>
        /// <returns>Array of character values</returns>
        public byte[] GetCharacterMap()
        {
            byte[] map = new byte[characters.Count];
            char[] arr = new char[characters.Count];
            characters.Keys.CopyTo(arr, 0);
            for (int c = 0; c < characters.Count; c++)
            {
                map[c] = (byte)arr[c];
            }
            return map;
        }

        /// <summary>
        /// Gets a list of all PixelSets that are associated to characters.
        /// </summary>
        /// <returns>Array of PixelSets</returns>
        public PixelSet[] GetCharacterPixels()
        {
            PixelSet[] chars = new PixelSet[characters.Count];
            characters.Values.CopyTo(chars, 0);
            return chars;
        }

        public string FontName { get; private set; }
        public int FontSize { get; private set; }
        public System.Drawing.Font Font { get; private set; }

        /// <summary>
        /// Creates a Font with the given parameters. A generic
        /// monospace font will be used if the name cannot be found.
        /// This program currently only works properly for monospace fonts.
        /// </summary>
        /// <param name="fontStyle">Font style</param>
        /// <returns>Font</returns>
        private void CreateFont(FontStyle fontStyle)
        {
            Font = null;
            FontFamily[] fonts = FontFamily.Families;
            // Search for the user-defined font
            for (int i = 0; i < fonts.Length; i++)
            {
                if (fonts[i].Name.ToLower() == FontName.ToLower())
                {
                    Font = new System.Drawing.Font(fonts[i], FontSize, fontStyle, GraphicsUnit.Pixel);
                    break;
                }

            }
            // If found, make sure it is a monospaced font
            if (Font != null)
            {
                Bitmap tmp = new Bitmap(FontSize * 2, FontSize * 2);
                Graphics g = Graphics.FromImage(tmp);
                if (g.MeasureString("!", Font).Width != g.MeasureString("@", Font).Width)
                {
                    Program.Log(Program.LogType.Warning, "The chosen font is not monospaced. Reverting to a generic monospaced font.");
                    Font.Dispose();
                    Font = null;
                }
                g.Dispose();
                tmp.Dispose();
            }
            // If not found or not monospaced, use a generic font
            if (Font == null)
            {
                if (FontName != string.Empty) Program.Log(Program.LogType.Warning, "The chosen font cannot be found. Reverting to a generic monospaced font.");
                Font = new System.Drawing.Font(FontFamily.GenericMonospace, FontSize, fontStyle, GraphicsUnit.Pixel);
                FontName = Font.FontFamily.Name;
            }
        }

        /// <summary>
        /// Creates a bitmap for each character in the font.
        /// Characters that the font does not include will not be assigned a bitmap.
        /// </summary>
        private void CreateBitmaps() {
            characters = new Dictionary<char, PixelSet>();
            ImageSurface surface = new ImageSurface(Format.RGB24, 1, 1);
            Context context = new Context(surface);
            context.SelectFontFace(FontName, FontSlant.Normal, FontWeight.Normal);
            context.SetFontSize(Font.Size);
            TextExtents ext = context.TextExtents("@");
            int charW = (int)Math.Ceiling(ext.Width);
            int charH = (int)Math.Ceiling(ext.Height);
            context.Dispose();
            surface.Dispose();

            surface = new ImageSurface(Format.RGB24, charW, charH);
            context = new Context(surface);
            PixelSet missingChar = null;
            for (int i = 0; i < asciiString.Length; i++) {
                // Render one character into a PixelSet
                string asciiChar = string.Empty + asciiString[i];
                context.SelectFontFace(FontName, FontSlant.Normal, FontWeight.Normal);
                context.SetFontSize(Font.Size);
                context.SetSourceRGB(0.067, 0.067, 0.067);
                context.Paint();
                context.SetSourceRGB(1, 1, 1);
                ext = context.TextExtents(asciiChar);
                context.MoveTo(ext.XBearing, ext.YBearing * -1);
                context.ShowText(asciiChar);
                PixelSet ch = new PixelSet(surface);

                // Filter out characters the font doesn't include
                // The first character is always unprintable, and serves as
                // a reference for what unprintable characters look like in this font
                if (i == 0) {
                    missingChar = ch;
                    continue;
                }
                else if (ch == missingChar) continue;
                characters.Add(asciiString[i], ch);
            }
            context.Dispose();
            surface.Dispose();

            // Add the space manually if it wasn't included
            if (!characters.ContainsKey(' '))
            {
                var en = characters.Values.GetEnumerator();
                en.MoveNext();
                characters.Add(' ', new PixelSet(en.Current.Width, en.Current.Height));
            }
        }

        /// <summary>
        /// AsciiFont Constructor
        /// </summary>
        /// <param name="fontName">Font name</param>
        /// <param name="fontSize">Font size in pixels</param>
        /// <param name="fontStyle">Optional FontStyle</param>
        public AsciiFont(string fontName, int fontSize, FontStyle fontStyle = FontStyle.Regular)
        {
            FontName = fontName;
            FontSize = fontSize;
            CreateFont(fontStyle);
            CreateBitmaps();
        }

        /// <summary>
        /// AsciiFont Destructor
        /// </summary>
        ~AsciiFont()
        {
            Font.Dispose();
        }
    }
}
