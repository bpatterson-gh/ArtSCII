using Bitmap = System.Drawing.Bitmap;
using Color = System.Drawing.Color;
using ImageSurface = Cairo.ImageSurface;

using System;

namespace ArtSCII
{
    class PixelSet
    {
        private byte[,][] pixels;
        public byte[,][] Pixels {
            get { return pixels; }
            set { SetPixels(value); }
        }

        public uint Width { get; private set; }
        public uint Height { get; private set; }

        /// <summary>
        /// Adds a scalar value to all pixels in a PixelSet.
        /// </summary>
        /// <param name="p">PixelSet</param>
        /// <param name="add">Value to add</param>
        /// <returns>PixelSet</returns>
        public static PixelSet operator +(PixelSet p, int add)
        {
            PixelSet output = new PixelSet(p.Width, p.Height);
            byte[,][] pixels = new byte[3, p.Width][];
            int sum;
            for (int x = 0; x < p.Width; x++)
            {
                pixels[0, x] = new byte[p.Height];
                pixels[1, x] = new byte[p.Height];
                pixels[2, x] = new byte[p.Height];
                for (int y = 0; y < p.Height; y++)
                {
                    sum = p.Pixels[0, x][y] + add;
                    if (sum > 255) sum = 255;
                    else if (sum < 0) sum = 0;
                    pixels[0, x][y] = (byte)sum;

                    sum = p.Pixels[1, x][y] + add;
                    if (sum > 255) sum = 255;
                    else if (sum < 0) sum = 0;
                    pixels[1, x][y] = (byte)sum;

                    sum = p.Pixels[2, x][y] + add;
                    if (sum > 255) sum = 255;
                    else if (sum < 0) sum = 0;
                    pixels[2, x][y] = (byte)sum;
                }
            }
            p.Pixels = pixels;
            return p;
        }

        /// <summary>
        /// Multiplies all pixels in a PixelSet by a scalar value.
        /// </summary>
        /// <param name="p">PixelSet</param>
        /// <param name="mult">Value to multipy by</param>
        /// <returns>PixelSet</returns>
        public static PixelSet operator *(PixelSet p, float mult)
        {
            PixelSet output = new PixelSet(p.Width, p.Height);
            byte[,][] pixels = new byte[3, p.Width][];
            int product;
            for (int x = 0; x < p.Width; x++)
            {
                pixels[0, x] = new byte[p.Height];
                pixels[1, x] = new byte[p.Height];
                pixels[2, x] = new byte[p.Height];
                for (int y = 0; y < p.Height; y++)
                {
                    product = (int)(p.Pixels[0, x][y] * mult);
                    if (product > 255) product = 255;
                    else if (product < 0) product = 0;
                    pixels[0, x][y] = (byte)product;

                    product = (int)(p.Pixels[1, x][y] * mult);
                    if (product > 255) product = 255;
                    else if (product < 0) product = 0;
                    pixels[1, x][y] = (byte)product;

                    product = (int)(p.Pixels[2, x][y] * mult);
                    if (product > 255) product = 255;
                    else if (product < 0) product = 0;
                    pixels[2, x][y] = (byte)product;
                }
            }
            p.Pixels = pixels;
            return p;
        }

        /// <summary>
        /// Check if 2 PixelSets contain the same image.
        /// </summary>
        /// <param name="a">PixelSet A</param>
        /// <param name="b">PixelSet B</param>
        /// <returns>True if the PixelSets contain the same image</returns>
        public static bool operator ==(PixelSet a, PixelSet b)
        {
            if (a.Width != b.Width || a.Height != b.Height) return false;

            for (int x = 0; x < a.Width; x++)
            {
                for (int y = 0; y < a.Height; y++)
                {
                    if (a.pixels[0,x][y] != b.pixels[0,x][y] ||
                        a.pixels[1,x][y] != b.pixels[1,x][y] ||
                        a.pixels[2,x][y] != b.pixels[2,x][y]) return false;
                }
            }
            return true;
        }

        /// <summary>
        /// Check if 2 PixelSets do not contain the same image.
        /// </summary>
        /// <param name="a">PixelSet A</param>
        /// <param name="b">PixelSet B</param>
        /// <returns>True if the PixelSets contain the same image</returns>
        public static bool operator !=(PixelSet a, PixelSet b)
        {
            return !(a == b);            
        }

        /// <summary>
        /// Check another object for equality.
        /// </summary>
        /// <param name="obj">object to compare</param>
        /// <returns>True if the object is a PixelSet containing the same image</returns>
        public override bool Equals(object obj)
        {
            if (!obj.GetType().Equals(GetType())) return false;
            return this == (PixelSet)obj;
        }

        /// <summary>
        /// Shuts the compiler up.
        /// </summary>
        /// <returns>base.GetHashCode()</returns>
        public override int GetHashCode()
        {
            return base.GetHashCode();
        }

        /// <summary>
        /// PixelSet Constructor
        /// </summary>
        /// <param name="width">Image width</param>
        /// <param name="height">Image height</param>
        /// <param name="brightness">Brightness interpreted as a greyscale value</param>
        public PixelSet(uint width, uint height, byte brightness = 0)
        {
            Width = width;
            Height = height;
            pixels = new byte[3, Width][];
            for (uint x = 0; x < Width; x++)
            {
                pixels[0, x] = new byte[Height];
                pixels[1, x] = new byte[Height];
                pixels[2, x] = new byte[Height];
                for (uint y = 0; y < Height; y++)
                {
                    pixels[0, x][y] = brightness;
                    pixels[1, x][y] = brightness;
                    pixels[2, x][y] = brightness;
                }
            }
        }

        /// <summary>
        /// PixelSet Constructor
        /// </summary>
        /// <param name="bmp">Bitmap to copy</param>
        public PixelSet(Bitmap bmp)
        {
            Width = (uint)bmp.Width;
            Height = (uint)bmp.Height;
            pixels = new byte[3, Width][];
            for (uint x = 0; x < Width; x++)
            {
                pixels[0, x] = new byte[Height];
                pixels[1, x] = new byte[Height];
                pixels[2, x] = new byte[Height];
                for (uint y = 0; y < Height; y++)
                {
                    pixels[0, x][y] = bmp.GetPixel((int) x, (int) y).R;
                    pixels[1, x][y] = bmp.GetPixel((int) x, (int) y).G;
                    pixels[2, x][y] = bmp.GetPixel((int) x, (int) y).B;
                }
            }
        }

        /// <summary>
        /// PixelSet Constructor
        /// </summary>
        /// <param name="bmp">ImageSurface to copy</param>
        public PixelSet(ImageSurface img)
        {
            Width = (uint)img.Width;
            Height = (uint)img.Height;
            byte[] data = img.Data;
            pixels = new byte[3, Width][];
            for (uint x = 0; x < Width; x++)
            {
                pixels[0, x] = new byte[Height];
                pixels[1, x] = new byte[Height];
                pixels[2, x] = new byte[Height];
                for (uint y = 0; y < Height; y++)
                {
                    uint i = (x + (Width * y)) * 4;
                    float alpha = data[i + 3] / 255.0f;
                    pixels[0, x][y] = (byte)(data[i] * alpha);
                    pixels[1, x][y] = (byte)(data[i + 1] * alpha);
                    pixels[2, x][y] = (byte)(data[i + 2] * alpha);
                }
            }
        }

        /// <summary>
        /// Inverts the color of a PixelSet.
        /// </summary>
        /// <returns>Inverted PixelSet</returns>
        public PixelSet Invert()
        {
            PixelSet p = new PixelSet(Width, Height);
            for (uint x = 0; x < Width; x++)
            {
                for (uint y = 0; y < Height; y++)
                {
                    p.Pixels[0, x][y] = (byte)(255 - pixels[0, x][y]);
                    p.Pixels[1, x][y] = (byte)(255 - pixels[1, x][y]);
                    p.Pixels[2, x][y] = (byte)(255 - pixels[2, x][y]);
                }
            }
            return p;
        }

        /// <summary>
        /// Converts this PixelSet to a Bitmap.
        /// </summary>
        /// <returns>Bitmap</returns>
        public Bitmap ToBitmap()
        {
            Bitmap bmp = new Bitmap((int) Width, (int) Height);
            for (int x = 0; x < Width; x++)
            {
                for (int y = 0; y < Height; y++)
                {
                    Color c = Color.FromArgb(pixels[0, x][y], pixels[1, x][y], pixels[2, x][y]);
                    bmp.SetPixel(x, y, c);
                }
            }
            return bmp;
        }

        /// <summary>
        /// Serialzes the pixel data in the following format:
        /// [pixels[0, 0][0], pixels[1, 0][0], pixels[2, 0][0],
        ///  pixels[0, 1][0], pixels[1, 1][0], pixels[2, 1][0],
        ///  pixels[0, 0][1], pixels[1, 0][1], pixels[2, 0][1],
        ///  pixels[0, 1][1], pixels[1, 1][1], pixels[2, 1][1]]
        /// </summary>
        /// <returns></returns>
        public byte[] Serialize()
        {
            byte[] bytes = new byte[Width * Height * 3];
            uint i = 0;
            for (uint y = 0; y < Height; y++)
            {
                for (uint x = 0; x < Width; x++)
                {
                    bytes[i++] = pixels[0, x][y];
                    bytes[i++] = pixels[1, x][y];
                    bytes[i++] = pixels[2, x][y];
                }
            }
            return bytes;
        }

        /// <summary>
        /// Sets pixels and recalculates dimensions.
        /// </summary>
        /// <param name="p">Pixels to set</param>
        private void SetPixels(byte[,][] p)
        {
            Width = (uint) p.Length / 3;
            Height = (uint) p[0, 0].Length;
            pixels = p;
        }
    }
}
