// Using mandelbrot set example by Adam Sampson <a.sampson@abertay.ac.uk> as the base for this class
#include "Mandelbrot.h"

// ---------- NON-PARALLEL FUNCTIONS ----------

void Mandelbrot::generate_original(double left, double right, double top, double bot) // regular, non-parallelized version of the funcion, running on the CPU
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

bool Mandelbrot::compute_single_pixel(std::complex<double> c)
{
	std::complex<double> z(0.0, 0.0);
	int it = 0;
	while (abs(z) < 2.0 && it < iterations)
	{
		z = (z * z) + c; it++;
	}
	if (it == iterations)
		return true;
	else return false;
}

// ---------- PARALLEL FUNCTIONS (UNIQUE LOCK MUTEX) ----------

void Mandelbrot::generate_parallel_for(double left, double right, double top, double bot)
{

	std::mutex image_mut;
	tbb::parallel_for(0, height, [&](int i) {

		for (int x = 0; x < width; x++)
		{
			std::complex<double> c(left + (x * (right - left) / width), top + (i * (bot - top) / height));
			std::complex<double> z(0.0, 0.0);
			int it = 0;

			while (abs(z) < 2.0 && it < iterations)
			{
				z = (z * z) + c;

				++it;
			}

			if (it == iterations)
			{
				std::unique_lock<std::mutex> lock(image_mut);
				Mandelbrot::image[i][x] = 0x000000; //black
			}
			else
			{
				std::unique_lock<std::mutex> lock(image_mut);
				Mandelbrot::image[i][x] = 0xFFFFFF; //while
			}
		}
		});



}

void Mandelbrot::generate_nested_parallel_for(double left, double right, double top, double bot)
{

	std::mutex image_mut;
	tbb::parallel_for(0, height, [&](int i) {
		
		tbb::parallel_for(0, width, [&](int j) {
			
			std::complex<double> c(left + (j * (right - left) / width), top + (i * (bot - top) / height));
			std::complex<double> z(0.0, 0.0);
			int it = 0;

			while (abs(z) < 2.0 && it < iterations)
			{
				z = (z * z) + c;

				++it;
			}

			if (it == iterations)
			{
				std::unique_lock<std::mutex> lock(image_mut);
				Mandelbrot::image[i][j] = 0x000000; //black
				
			}
			else
			{
				std::unique_lock<std::mutex> lock(image_mut);
				Mandelbrot::image[i][j] = 0xFFFFFF; //while
				
			} 
			});
		});
	


}

void Mandelbrot::generate_nested_parallel_for_func(double left, double right, double top, double bot)
{

	std::mutex image_mut;
	tbb::parallel_for(0, height, [&](int i) {

		tbb::parallel_for(0, width, [&](int j) {

			if (Mandelbrot::compute_single_pixel((left + (j * (right - left) / width), top + (i * (bot - top) / height))))
			{
				std::unique_lock<std::mutex> lock(image_mut);
				Mandelbrot::image[i][j] = 0x000000; //black

			}
			else
			{
				std::unique_lock<std::mutex> lock(image_mut);
				Mandelbrot::image[i][j] = 0xFFFFFF; //while

			}
			});
		});



}

void Mandelbrot::generate_atomic_parallel_for(double left, double right, double top, double bot)
{

	
	tbb::parallel_for(0, height, [&](int i) {

		for (int x = 0; x < width; x++)
		{
			std::complex<double> c(left + (x * (right - left) / width), top + (i * (bot - top) / height));
			std::complex<double> z(0.0, 0.0);
			int it = 0;

			while (abs(z) < 2.0 && it < iterations)
			{
				z = (z * z) + c;

				++it;
			}

			if (it == iterations)
			{
				Mandelbrot::image_atomic[i][x] = 0x000000; //black
			}
			else
			{
				Mandelbrot::image_atomic[i][x] = 0xFFFFFF; //while
			}
		}
		});



}

void Mandelbrot::generate_atomic_nested_parallel_for(double left, double right, double top, double bot)
{

	
	tbb::parallel_for(0, height, [&](int i) {

		tbb::parallel_for(0, width, [&](int j) {

			std::complex<double> c(left + (j * (right - left) / width), top + (i * (bot - top) / height));
			std::complex<double> z(0.0, 0.0);
			int it = 0;

			while (abs(z) < 2.0 && it < iterations)
			{
				z = (z * z) + c;

				++it;
			}

			if (it == iterations)
			{
				
				Mandelbrot::image_atomic[i][j] = 0x000000; //black

			}
			else
			{
				
				Mandelbrot::image_atomic[i][j] = 0xFFFFFF; //while

			}
			});
		});



}

