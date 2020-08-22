using System;
using System.Collections.Generic;

using Bitmap = System.Drawing.Bitmap;
using BitmapFrame = System.Windows.Media.Imaging.BitmapFrame;
using BmpBitmapEncoder = System.Windows.Media.Imaging.BmpBitmapEncoder;
using Color = System.Windows.Media.Color;
using CultureInfo = System.Globalization.CultureInfo;
using DrawingContext = System.Windows.Media.DrawingContext;
using DrawingVisual = System.Windows.Media.DrawingVisual;
using FlowDirection = System.Windows.FlowDirection;
using FormattedText = System.Windows.Media.FormattedText;
using FontFamily = System.Drawing.FontFamily;
using FontStyle = System.Drawing.FontStyle;
using Graphics = System.Drawing.Graphics;
using GraphicsUnit = System.Drawing.GraphicsUnit;
using MemoryStream = System.IO.MemoryStream;
using PixelFormats = System.Windows.Media.PixelFormats;
using Point = System.Windows.Point;
using Rect = System.Windows.Rect;
using RenderTargetBitmap = System.Windows.Media.Imaging.RenderTargetBitmap;
using SolidColorBrush = System.Windows.Media.SolidColorBrush;
using Typeface = System.Windows.Media.Typeface;

namespace ArtSCII
{
    class AsciiFont
    {
        /// <summary>
        /// Creates a string of all characters from 33 to 255.
        /// Each character is on its own line for easier processing.
        /// </summary>
        /// <returns>ASCII String</returns>
        private static string InitAsciiString() {
            string s = string.Empty;
            for (int c = 32; c < 128; c++)
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
        /// <param name="fontName">Name of the font</param>
        /// <param name="fontSize">Size of the font in pixels</param>
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
                Font = new System.Drawing.Font(FontFamily.GenericMonospace, FontSize, fontStyle, GraphicsUnit.Pixel);
                FontName = Font.FontFamily.Name;
            }
        }

        /// <summary>
        /// Creates Bitmap representations of each ASCII character.
        /// </summary>
        private void CreateBitmaps()
        {
            characters = new Dictionary<char, PixelSet>();
            SolidColorBrush black = new SolidColorBrush(Color.FromArgb(255, 0, 0, 0));
            SolidColorBrush white = new SolidColorBrush(Color.FromArgb(255, 255, 255, 255));
            Typeface typeface = new Typeface(Font.Name);

            int charW, charH;
            for (int i = 0; i < asciiString.Length; i++)
            {
                string s = string.Empty + asciiString[i];
                FormattedText text = new FormattedText(s, CultureInfo.InvariantCulture,
                    FlowDirection.LeftToRight, typeface, Font.Size, black, 1);
                charW = (int)Math.Ceiling(text.Width);
                charH = (int)Math.Floor(text.Height);
                DrawingVisual dv = new DrawingVisual();
                DrawingContext dc = dv.RenderOpen();

                dc.DrawRectangle(white, null, new Rect(0, 0, charW, charH));
                dc.DrawText(text, new Point());
                dc.Close();

                if (charW < 4) continue;
                RenderTargetBitmap render = new RenderTargetBitmap(charW, charH, 96, 96, PixelFormats.Pbgra32);
                render.Render(dv);
                MemoryStream mstream = new MemoryStream();
                BmpBitmapEncoder enc = new BmpBitmapEncoder();
                enc.Frames.Add(BitmapFrame.Create(render));
                enc.Save(mstream);
                Bitmap oneChar = new Bitmap(mstream);
                
                characters.Add(asciiString[i], new PixelSet(oneChar).Invert());
                oneChar.Dispose();
                mstream.Dispose();
            }
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
