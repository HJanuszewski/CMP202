// Using mandelbrot set example by Adam Sampson <a.sampson@abertay.ac.uk> as the base for this class
#include "Mandelbrot.h"

// ---------- NON-PARALLEL FUNCTIONS ----------

void Mandelbrot::generate_original(double values[4]) // regular, non-parallelized version of the funcion, running on the CPU
{

	for (int y = 0; y < height; y++) // those loops should be parallelized, as they go over every pixel, one at a time
	{
		for (int x = 0; x < width; x++)
		{
			std::complex<double> c(values[0] + (x * (values[1] - values[0]) / width), values[2] + (y * (values[3] - values[2]) / height));
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

void Mandelbrot::write_tga(const char* filename, uint32_t img[height][width]) // Same write function as the lab example, with very minor changes
{
	std::ofstream outfile(filename, std::ofstream::binary);


	uint8_t header[18] = {
		0, // no image ID
		0, // no colour map
		2, // uncompressed 24-bit image
		0, 0, 0, 0, 0, // empty colour map specification
		0, 0, // X origin
		0, 0, // Y origin
		width & 0xFF, (width >> 8) & 0xFF, // width
		height & 0xFF, (height >> 8) & 0xFF, // height
		24, // bits per pixel
		0, // image descriptor
	};
	outfile.write((const char*)header, 18);

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			uint8_t pixel[3] = {
				img[y][x] & 0xFF, // blue channel
				(img[y][x] >> 8) & 0xFF, // green channel
				(img[y][x] >> 16) & 0xFF, // red channel
			};
			outfile.write((const char*)pixel, 3);
		}
	}

	outfile.close();
	if (!outfile)
	{
		// An error has occurred at some point since we opened the file.
		std::cout << "Error writing to " << filename << std::endl;
		exit(1);
	}
}

void Mandelbrot::write_tga(const char* filename, std::atomic<uint32_t> img[height][width]) // Same write function as the lab example, with very minor changes
{
	std::ofstream outfile(filename, std::ofstream::binary);


	uint8_t header[18] = {
		0, // no image ID
		0, // no colour map
		2, // uncompressed 24-bit image
		0, 0, 0, 0, 0, // empty colour map specification
		0, 0, // X origin
		0, 0, // Y origin
		width & 0xFF, (width >> 8) & 0xFF, // width
		height & 0xFF, (height >> 8) & 0xFF, // height
		24, // bits per pixel
		0, // image descriptor
	};
	outfile.write((const char*)header, 18);

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			uint8_t pixel[3] = {
				img[y][x] & 0xFF, // blue channel
				(img[y][x] >> 8) & 0xFF, // green channel
				(img[y][x] >> 16) & 0xFF, // red channel
			};
			outfile.write((const char*)pixel, 3);
		}
	}

	outfile.close();
	if (!outfile)
	{
		// An error has occurred at some point since we opened the file.
		std::cout << "Error writing to " << filename << std::endl;
		exit(1);
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

// ---------- PARALLEL FUNCTIONS -----------

void Mandelbrot::generate_parallel_for(double values[4], uint32_t (&img)[height][width])
{

	std::mutex image_mut;
	tbb::parallel_for(0, height, [&](int i) {

		for (int x = 0; x < width; x++)
		{
			std::complex<double> c(values[0] + (x * (values[1] - values[0]) / width), values[2]+ (i * (values[3]- values[2]) / height));
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
				img[i][x] = 0x5efc03; //black
			}
			else// cokolwiek bo komputer to i tak ignoruje
			{
				std::unique_lock<std::mutex> lock(image_mut);
				img[i][x] = 0xFFFFFF; //while
			}
		}
		});



}

void Mandelbrot::generate_parallel_for(double values[4], std::atomic<uint32_t> (&img)[height][width])
{
	
	
	tbb::parallel_for(0, height, [&](int i) {

		for (int x = 0; x < width; x++)
		{
			std::complex<double> c(values[0] + (x * (values[1] - values[0]) / width), values[2]+ (i * (values[3] - values[2]) / height));
			std::complex<double> z(0.0, 0.0);
			int it = 0;

			while (abs(z) < 2.0 && it < iterations)
			{
				z = (z * z) + c;

				++it;
			}

			if (it == iterations)
			{
				
				img[i][x] = 0x5efc03; //black
			}
			else
			{
				
				img[i][x] = 0xFFFFFF; //while
			}
		}
		});



}

void Mandelbrot::generate_nested_parallel_for(double values[4],  uint32_t (&img)[height][width])
{

	std::mutex image_mut;
	tbb::parallel_for(0, height, [&](int i) {

		tbb::parallel_for(0, width, [&](int j) {

			std::complex<double> c(values[0] + (j * (values[1]- values[0]) / width), values[2]+ (i * (values[3]- values[2]) / height));
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
				img[i][j] = 0x5efc03; //black

			}
			else
			{
				std::unique_lock<std::mutex> lock(image_mut);
				img[i][j] = 0xFFFFFF; //while

			}
			});
		});



}

void Mandelbrot::generate_nested_parallel_for( double values[4],  std::atomic<uint32_t>(&img)[height][width])
{

	
	tbb::parallel_for(0, height, [&](int i) {
		
		tbb::parallel_for(0, width, [&](int j) {
			
			std::complex<double> c(values[0] + (j * (values[1]- values[0]) / width), values[2]+ (i * (values[3]- values[2]) / height));
			std::complex<double> z(0.0, 0.0);
			int it = 0;

			while (abs(z) < 2.0 && it < iterations)
			{
				z = (z * z) + c;

				++it;
			}

			if (it == iterations)
			{
				
				img[i][j] = 0x000000; //black
				
			}
			else
			{
				
				img[i][j] = 0xFFFFFF; //while
				
			} 
			});
		});
	


}

void Mandelbrot::generate_nested_parallel_for_func(double values[4],  uint32_t(&img)[height][width])
{

	std::mutex image_mut;
	tbb::parallel_for(0, height, [&](int i) {

		tbb::parallel_for(0, width, [&](int j) {

			if (Mandelbrot::compute_single_pixel((values[0] + (j * (values[1]- values[0]) / width), values[2]+ (i * (values[3]- values[2]) / height))))
			{
				std::unique_lock<std::mutex> lock(image_mut);
				img[i][j] = 0x5efc03; //black

			}
			else
			{
				std::unique_lock<std::mutex> lock(image_mut);
				img[i][j] = 0xFFFFFF; //while

			}
			});
		});



}

void Mandelbrot::generate_nested_parallel_for_func(double values[4], std::atomic<uint32_t>(&img)[height][width])
{

	
	tbb::parallel_for(0, height, [&](int i) {

		tbb::parallel_for(0, width, [&](int j) {

			if (Mandelbrot::compute_single_pixel((values[0] + (j * (values[1]- values[0]) / width), values[2]+ (i * (values[3]- values[2]) / height))))
			{
				img[i][j] = 0x000000; //black

			}
			else
			{
				img[i][j] = 0xFFFFFF; //while

			}
			});
		});



}


