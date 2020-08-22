using System.Collections.Generic;

namespace ArtSCII
{
    /// <summary>
    /// Performs Kernel Convolutions.
    /// </summary>
    static class Convolver
    {
        /// <summary>
        /// Contains information about an image filter.
        /// </summary>
        public class Kernel
        {
            /// <summary>
            /// A 2D array of pixel weights.
            /// </summary>
            public float[,] pixels;

            /// <summary>
            /// This value is multiplied by the kernel's output.
            /// It is used to control the output brightness.
            /// </summary>
            public float mult;

            /// <summary>
            /// Size of the kernel.
            /// </summary>
            public uint width, height;

            /// <summary>
            /// This will invert the output of the kernel after the new color is chosen.
            /// It is used to ensure a consistent background color between all the kernels.
            /// </summary>
            public bool invert;

            /// <summary>
            /// Creates a flat array of kernel values.
            /// </summary>
            /// <returns>Kernel values in row-major format</returns>
            public unsafe float[] Serialize()
            {
                float[] floats = new float[width * height];
                int i = 0;
                for (int y = 0; y < height; y++)
                {
                    for (int x = 0; x < width; x++)
                    {
                        floats[i++] = pixels[x, y];
                    }
                }
                return floats;
            }
        }


        /// <summary>
        /// List of Kernels to generate filtered images with.
        /// </summary>
        public static Kernel[] Kernels { get { return kernels.ToArray(); } }
        private static readonly List<Kernel> kernels = GenKernels();
        private static List<Kernel> GenKernels()
        {
            List<Kernel> ks = new List<Kernel>();
            Kernel k;

            // Unfiltered
            k = new Kernel
            {
                pixels = new float[,] { {  0f,  0f,  0f },
                                        {  0f,  1f,  0f },
                                        {  0f,  0f,  0f } },
                mult = 1f,
                width = 3,
                height = 3,
                invert = false
            };
            ks.Add(k);//*/

            // Sharpen
            k = new Kernel
            {
                pixels = new float[,] { {  0f, -1f,  0f },
                                        { -1f,  5f, -1f },
                                        {  0f, -1f,  0f } },
                mult = 1f,
                width = 3,
                height = 3,
                invert = false
            };
            ks.Add(k);//*/

            // Edge Detection (Horizontal / Vertical)
            k = new Kernel
            {
                pixels = new float[,] { { -1, -1, -1 },
                                        { -1,  8, -1 },
                                        { -1, -1, -1 } },
                mult = 50f,
                width = 3,
                height = 3,
                invert = false
            };
            ks.Add(k);

            // Gaussian Blur
            k = new Kernel
            {
                pixels = new float[,] { {  1,  2,  1 },
                                        {  2,  4,  2 },
                                        {  1,  2,  1 } },
                mult = 1f / 16f,
                width = 3,
                height = 3,
                invert = false
            };
            ks.Add(k);//*/

            return ks;
        }
    }
}
