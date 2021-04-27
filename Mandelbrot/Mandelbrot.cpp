// Using mandelbrot set example by Adam Sampson <a.sampson@abertay.ac.uk> as the base for this class
#include "Mandelbrot.h"

// ---------- NON-PARALLEL FUNCTIONS ----------

void Mandelbrot::generate_original(double values[4], uint32_t bg_colour, uint32_t fg_colour) // regular, non-parallelized version of the funcion, running on the CPU
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
				Mandelbrot::image[y][x] = bg_colour;
			}
			else
			{
				Mandelbrot::image[y][x] = fg_colour;
			}
		}


	}
}

void Mandelbrot::write_tga(const char* name, bool atomic) // Same write function as the lab example, with very minor changes
{
	std::ofstream outfile(name, std::ofstream::binary);


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
	if (atomic)
	{
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				uint8_t pixel[3] = {
					 Mandelbrot::image[y][x] & 0xFF, // blue channel
					(Mandelbrot::image[y][x] >> 8) & 0xFF, // green channel
					(Mandelbrot::image[y][x] >> 16) & 0xFF, // red channel
				};
				outfile.write((const char*)pixel, 3);
			}
		}
	}
	else
	{
		for (int y = 0; y < height; ++y)
		{
			for (int x = 0; x < width; ++x)
			{
				uint8_t pixel[3] = {
					 Mandelbrot::image_atomic[y][x] & 0xFF, // blue channel
					(Mandelbrot::image_atomic[y][x] >> 8) & 0xFF, // green channel
					(Mandelbrot::image_atomic[y][x] >> 16) & 0xFF, // red channel
				};
				outfile.write((const char*)pixel, 3);
			}
		}
	}
	outfile.close();
	if (!outfile)
	{
		// An error has occurred at some point since we opened the file.
		std::cout << "Error writing to " << name << std::endl;
		exit(1);
	}
}

void Mandelbrot::write_tga_thread(const char* name, bool atomic)
{
	std::ofstream outfile(name, std::ofstream::binary);


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
	if (atomic)
	{
		//insert some code that it supposed to handle signaling
		// in this case, we need to wait for the LINE OR PIXEL (fugure out later or implement both) to finish being done before they are to be saved
		for (int y = 0; y < height; y++)
		{
			//block until the line with the ID of y is completed
			for (int x = 0; x < width; x++)
			{
				uint8_t pixel[3] = {
					 Mandelbrot::image_atomic[y][x] & 0xFF, // blue channel
					(Mandelbrot::image_atomic[y][x] >> 8) & 0xFF, // green channel
					(Mandelbrot::image_atomic[y][x] >> 16) & 0xFF, // red channel
				};
				outfile.write((const char*)pixel, 3);
			}
		}
	}
	else
	{

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

void Mandelbrot::generate_parallel_for(double values[4], uint32_t (&img)[height][width], uint32_t bg_colour, uint32_t fg_colour)
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
				img[i][x] = fg_colour; 
			}
			else
			{
				std::unique_lock<std::mutex> lock(image_mut);
				img[i][x] = bg_colour;
			}
		}
		});



}

void Mandelbrot::generate_parallel_for(double values[4], std::atomic<uint32_t> (&img)[height][width], uint32_t bg_colour, uint32_t fg_colour)
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
				
				img[i][x] = fg_colour; 
			}
			else
			{
				
				img[i][x] = bg_colour; 
			}
		}
		});



}

void Mandelbrot::generate_nested_parallel_for(double values[4],  uint32_t (&img)[height][width], uint32_t bg_colour, uint32_t fg_colour)
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
				img[i][j] = fg_colour;

			}
			else
			{
				std::unique_lock<std::mutex> lock(image_mut);
				img[i][j] = bg_colour; 

			}
			});
		});



}

void Mandelbrot::generate_nested_parallel_for( double values[4],  std::atomic<uint32_t>(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour)
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
				
				img[i][j] = fg_colour;
				
			}
			else
			{
				
				img[i][j] = bg_colour; 
				
			} 
			});
		});
	


}

void Mandelbrot::generate_nested_parallel_for_func(double values[4],  uint32_t(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour)
{

	std::mutex image_mut;
	tbb::parallel_for(0, height, [&](int i) {

		tbb::parallel_for(0, width, [&](int j) {

			if (Mandelbrot::compute_single_pixel((values[0] + (j * (values[1]- values[0]) / width), values[2]+ (i * (values[3]- values[2]) / height))))
			{
				std::unique_lock<std::mutex> lock(image_mut);
				img[i][j] = fg_colour;

			}
			else
			{
				std::unique_lock<std::mutex> lock(image_mut);
				img[i][j] = bg_colour; 

			}
			});
		});



}

void Mandelbrot::generate_nested_parallel_for_func(double values[4], std::atomic<uint32_t>(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour)
{

	
	tbb::parallel_for(0, height, [&](int i) {

		tbb::parallel_for(0, width, [&](int j) {

			if (Mandelbrot::compute_single_pixel((values[0] + (j * (values[1]- values[0]) / width), values[2]+ (i * (values[3]- values[2]) / height))))
			{
				img[i][j] = fg_colour;

			}
			else
			{
				img[i][j] = bg_colour;

			}
			});
		});



}


