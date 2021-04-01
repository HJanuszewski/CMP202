// Using mandelbrot set example by Adam Sampson <a.sampson@abertay.ac.uk> as the base for this class
#include "Mandelbrot.h"

void Mandelbrot::generate(double left, double right, double top, double bot) // regular, non-parallelized version of the funcion, running on the CPU
{

	for (int y = 0; y < height; y++) // those loops should be parallelized, as they go over every pixel, one at a time
	{
		for (int x = 0; x < width; x++)
		{
			std::complex<double> c(left + (x * (right - left) / width), top + (y * (bot - top) / height));
			std::complex<double> z(0.0, 0.0);
			int i = 0;
			
			while (abs(z) < 2.0 && i < iterations)
			{
				z = (z * z) + c;

				++i;
			}

			if (i == iterations)
			{
				Mandelbrot::image[y][x] = 0x000000; //black
			}
			else
			{
				Mandelbrot::image[y][x] = 0xFFFFFF; //while
			}
		}


	}
}
