using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;

using Color = System.Drawing.Color;

namespace ArtSCII
{
    class OCL
    {
        private const int maxImageBufSize = 65536 - 17; // ~64KB buffer (due to CLR array constraints)
        [StructLayout(LayoutKind.Sequential)]
        public unsafe struct CImageInfo
        {
            public uint width;
            public uint height;
            public int bufSize;
            public bool final;
            public fixed byte buffer[maxImageBufSize];
        }

        private const int maxKernelBufSize = 256; // 1 KB buffer
        [StructLayout(LayoutKind.Sequential)]
        public unsafe struct CKernelInfo
        {
            public uint width;
            public uint height;
            public float mult;
            public bool invert;
            public uint bufSize;
            public fixed float buffer[maxKernelBufSize];
        }

        [DllImport("artscii.dll")]
        public static extern bool OCL_Init();
        [DllImport("artscii.dll")]
        public static extern void OCL_Cleanup();

        [DllImport("artscii.dll")]
        private static unsafe extern bool OCL_ToAscii(IntPtr imgBufs, byte* outChars, byte* outColors,
            IntPtr kernels, uint numKernels, IntPtr charBufs, int numChars, byte* charMap);

        /// <summary>
        /// Creates image buffers to send to artscii.dll.
        /// </summary>
        /// <param name="p">Source image</param>
        /// <returns>Array of CImageInfo buffers</returns>
        public static unsafe CImageInfo[] MakeImageBuffers(PixelSet p)
        {
            byte[] img = p.Serialize();
            int numBufs = (img.Length / maxImageBufSize) + 1, ix = 0;
            CImageInfo[] buffers = new CImageInfo[numBufs];
            for (int i = 0; i < numBufs; i++)
            {
                buffers[i].width = p.Width;
                buffers[i].height = p.Height;
                if (i == (numBufs - 1) && (img.Length % maxImageBufSize) > 0)
                {
                    buffers[i].bufSize = img.Length % maxImageBufSize;
                }
                else buffers[i].bufSize = maxImageBufSize;
                for (int j = 0; j < buffers[i].bufSize; j++, ix++)
                {
                    buffers[i].buffer[j] = img[ix];
                }
                buffers[i].final = ix == img.Length;
            }
            return buffers;
        }

        /// <summary>
        /// Creates image buffers to send to artscii.dll.
        /// </summary>
        /// <param name="p">List of source images</param>
        /// <returns>Array of CImageInfo buffers</returns>
        public static unsafe CImageInfo[] MakeMultiImageBuffers(PixelSet[] p)
        {
            int maxBufs = (int)(((p[0].Width * p[0].Height * 3) /
                    (float)maxImageBufSize) + 0.9999f) * p.Length;
            CImageInfo[] buffers = new CImageInfo[maxBufs];
            int i = 0;
            for (int ps = 0; ps < p.Length; ps++)
            {
                byte[] img = p[ps].Serialize();
                int numBufs = (img.Length / maxImageBufSize) + 1, ix = 0;
                numBufs += numBufs * ps;
                for (; i < numBufs; i++)
                {
                    buffers[i].width = p[ps].Width;
                    buffers[i].height = p[ps].Height;
                    if (i == (numBufs - 1) && (img.Length % maxImageBufSize) > 0)
                    {
                        buffers[i].bufSize = img.Length % maxImageBufSize;
                    }
                    else buffers[i].bufSize = maxImageBufSize;
                    for (int j = 0; j < buffers[i].bufSize; j++, ix++)
                    {
                        buffers[i].buffer[j] = img[ix];
                    }
                    buffers[i].final = ix == img.Length;
                }
            }
            return buffers;
        }

        /// <summary>
        /// Creates Kernel buffers to send to artscii.dll.
        /// </summary>
        /// <param name="kernels">List of Kernels</param>
        /// <returns>Array of CKernelInfo buffers</returns>
        private static unsafe CKernelInfo[] MakeKernelBuffers(Convolver.Kernel[] kernels)
        {
            CKernelInfo[] buffers = new CKernelInfo[kernels.Length];
            for (int i = 0; i < kernels.Length; i++)
            {
                float[] k = kernels[i].Serialize();
                buffers[i].width = kernels[i].width;
                buffers[i].height = kernels[i].height;
                buffers[i].mult = kernels[i].mult;
                buffers[i].invert = kernels[i].invert;
                buffers[i].bufSize = (uint)k.Length;
                for (int j = 0; j < k.Length; j++)
                {
                    buffers[i].buffer[j] = k[j];
                }
            }
            return buffers;
        }

        /// <summary>
        /// Calls artscii.dll to generate a list of ASCII characters and colors from an image.
        /// </summary>
        /// <param name="p">Input image</param>
        /// <param name="font">Font to render</param>
        /// <returns>List of ASCII characters and colors</returns>
        public static unsafe List<Tuple<char, Color>> ToAscii(PixelSet p, AsciiFont font)
        {
            int outLen = (int)(((p.Width / Program.charWidth) + 1) *
                               ((p.Height / Program.charHeight)));
            List<Tuple<char, Color>> output = new List<Tuple<char, Color>>();
            fixed (CImageInfo* i = MakeImageBuffers(p))
            {
                fixed (byte* o = new byte[outLen])
                {
                    fixed (CKernelInfo* k = MakeKernelBuffers(Convolver.Kernels))
                    {
                        fixed (CImageInfo* ch = MakeMultiImageBuffers(font.GetCharacterPixels()))
                        {
                            fixed (byte* m = font.GetCharacterMap())
                            {
                                fixed (byte* cl = new byte[outLen * 3])
                                {
                                    Program.openCL = OCL_ToAscii((IntPtr)i, o, cl, (IntPtr)k, (uint)Convolver.Kernels.Length,
                                                                 (IntPtr)ch, font.characters.Count, m);
                                    for (int x = 0, c = 0; x < outLen; x++, c += 3)
                                    {
                                        output.Add(new Tuple<char, Color>((char)o[x], Color.FromArgb(cl[c], cl[c + 1], cl[c + 2])));
                                    }
                                }
                            }
                        }
                    }
                }
            }
            return output;
        }
    }
}
