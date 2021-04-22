#include <iostream>
#include <chrono>
#include <fstream>
#include "Mandelbrot.h"



void write_tga(const char* filename, uint32_t image[height][width] ) // Same write function as the lab example, with very minor changes
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
				image[y][x] & 0xFF, // blue channel
				(image[y][x] >> 8) & 0xFF, // green channel
				(image[y][x] >> 16) & 0xFF, // red channel
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

int main()
{

	std::cout << "Welcome! INSERT THE OPTIONS N STUFF HERE, please wait" << std::endl;
	Mandelbrot* image = new Mandelbrot;
	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now(); // start point of the timing
	image->generate_nested_parallel_for(-2.0, 1.0, 1.125, -1.125);
	
	std::chrono::steady_clock::time_point stop = std::chrono::steady_clock::now(); // stop point of the timing.
	write_tga("output.tga", image->image);
	std::cout << "It took " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << " ms" << std::endl;
	
	return 0;
}