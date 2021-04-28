#pragma once
#include <cstdint>
#include <complex>
#include <tbb/tbb.h>
#include <vector>
#include <iostream>
#include <mutex>
#include <condition_variable>
#include <fstream>
#define height 1080
#define width 1920
constexpr int iterations = 1000; // At first I wanted to use #define for all 3, but compiler was not happy with this one being used in the while loop for some reason.
// Using mandelbrot set example by Adam Sampson <a.sampson@abertay.ac.uk> as the base for this class
class Mandelbrot
{
private:
	
	bool compute_single_pixel(std::complex<double> c); // A piece of the original function, taking an already calculated c value and returning true/false depending on if that point exists in the set.
	const char filename[15] = "Mandelbrot.tga";
	
public:
	
	uint32_t image[height][width];
	std::atomic<uint32_t> image_atomic[height][width];
	std::mutex line_mutex[height];
	std::condition_variable write_condition[height]; // create a condition variable for each line that will
	bool line_completed[height] = { false }; //initialize the entire array to be false, since no line has been generated yet.
	bool atomic;

	void generate_original(double values[4], uint32_t bg_colour, uint32_t fg_colour); // The original function that was used in the lab example for generating the set, not parallelised at all.
	
	void write_tga_thread(const char* name, bool atomic);

	template<typename T> void generate_parallel_for(double values[4], T (&img)[height][width], uint32_t bg_colour, uint32_t fg_colour );
	template<typename T> void generate_parallel_for(double values[4], T(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour, int threads); 
	
	template<typename T> void generate_nested_parallel_for(double values[4], T(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour); // A parallel version of the function using nested parallel_for loops from TBB in order to generate the set.
	template<typename T> void generate_nested_parallel_for(double values[4], T(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour, int threads); 
	
	template<typename T> void generate_nested_parallel_for_func(double values[4],  T(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour); // A parallel version of the function using nested parallel_for loops from TBB, which call the compute_single_pixel separately. Slowest out of all parallel functions.
	template<typename T> void generate_nested_parallel_for_func(double values[4], T(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour,int threads); 
	

};

// Using mandelbrot set example by Adam Sampson <a.sampson@abertay.ac.uk> as the base for this class



/// The code below was originally in Mandelbrot.cpp, however it needed to be moved into this file, since linker was giving me errors about the templated functions


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
				Mandelbrot::image[y][x] = fg_colour;
			}
			else
			{
				Mandelbrot::image[y][x] = bg_colour;
			}
		}


	}
}



//todo: check which ones are actually used clean up the file writing funcitons (don't forget the one in main.cpp)

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


	for (int y = 0; y < height; y++)
	{
		std::unique_lock<std::mutex> ul(Mandelbrot::line_mutex[y]);
		Mandelbrot::write_condition[y].wait(ul, [&] { if (Mandelbrot::line_completed[y]) return true; else return false; });
		//block until the line with the ID of y is completed
		if (atomic)
		{
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
		else
		{
			for (int x = 0; x < width; x++)
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


}



template<typename T>void Mandelbrot::generate_parallel_for(double values[4], T(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour)
{

	std::mutex image_mut;
	tbb::task_arena write(1);
	write.execute([&] {Mandelbrot::write_tga_thread(Mandelbrot::filename, Mandelbrot::atomic); });
	tbb::parallel_for(0, height, [&](int i) {

		std::lock_guard<std::mutex> lg(line_mutex[i]);
		for (int x = 0; x < width; x++)
		{
			
			std::complex<double> c(values[0] + (x * (values[1] - values[0]) / width), values[2] + (i * (values[3] - values[2]) / height));
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
		// here insert the thingy
		Mandelbrot::line_completed[i] = true;
		Mandelbrot::write_condition[i].notify_one();
		});



}


template<typename T>void Mandelbrot::generate_parallel_for(double values[4], T(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour, int threads)
{

	//std::mutex image_mut;  // Mutex used to safely share the 'img' array
	tbb::task_arena thread_limit(threads); // Task arena which is needed by TBB to limit the amount of threads that will run
	thread_limit.execute([&] { // Running the generation code inside a lambda expression in the 'thread_limit' task arena, to limit the thread number to the 'threads' value
		tbb::parallel_for(0, height, [&](int i) { // Parallel for from TBB will run a separate thread for each 'i' value, however starting no more threads than the limit that was imposed by task arena.

			std::lock_guard<std::mutex> lg(line_mutex[i]);
			for (int x = 0; x < width; x++)
			{
				
				std::complex<double> c(values[0] + (x * (values[1] - values[0]) / width), values[2] + (i * (values[3] - values[2]) / height));
				std::complex<double> z(0.0, 0.0);
				int it = 0;

				while (abs(z) < 2.0 && it < iterations)
				{
					z = (z * z) + c;

					++it;
				}

				if (it == iterations)
				{
					//std::unique_lock<std::mutex> lock(image_mut);
					img[i][x] = fg_colour;

				}
				else
				{
					//std::unique_lock<std::mutex> lock(image_mut);
					img[i][x] = bg_colour;
				}
			}
			// here insert the thingy
			Mandelbrot::line_completed[i] = true;
			Mandelbrot::write_condition[i].notify_one();
			});
		});


}


template<typename T> void Mandelbrot::generate_nested_parallel_for(double values[4], T(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour)
{

	//std::mutex image_mut;
	tbb::parallel_for(0, height, [&](int i) {

		std::lock_guard<std::mutex> lg(line_mutex[i]);
		tbb::parallel_for(0, width, [&](int j) {

			
			std::complex<double> c(values[0] + (j * (values[1] - values[0]) / width), values[2] + (i * (values[3] - values[2]) / height));
			std::complex<double> z(0.0, 0.0);
			int it = 0;

			while (abs(z) < 2.0 && it < iterations)
			{
				z = (z * z) + c;

				++it;
			}

			if (it == iterations)
			{
				//std::unique_lock<std::mutex> lock(image_mut);
				img[i][j] = fg_colour;

			}
			else
			{
				//std::unique_lock<std::mutex> lock(image_mut);
				img[i][j] = bg_colour;

			}
		});
		//Mandelbrot::line_completed[i] = true;
		Mandelbrot::write_condition[i].notify_one();
	});



}

template<typename T> void Mandelbrot::generate_nested_parallel_for(double values[4], T(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour, int threads)
{

	std::mutex image_mut;
	tbb::task_arena thread_limit(threads);
	thread_limit.execute([&] {
		tbb::parallel_for(0, height, [&](int i) {

			std::lock_guard<std::mutex> lg(line_mutex[i]);
			tbb::parallel_for(0, width, [&](int j) {

				
				std::complex<double> c(values[0] + (j * (values[1] - values[0]) / width), values[2] + (i * (values[3] - values[2]) / height));
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
			Mandelbrot::line_completed[i] = true;
			Mandelbrot::write_condition[i].notify_one();
			});
		
		});




}


template<typename T> void Mandelbrot::generate_nested_parallel_for_func(double values[4], T(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour)
{

	std::mutex image_mut;
	tbb::parallel_for(0, height, [&](int i) {

		std::lock_guard<std::mutex> lg(line_mutex[i]);
		tbb::parallel_for(0, width, [&](int j) {

			
			if (Mandelbrot::compute_single_pixel((values[0] + (j * (values[1] - values[0]) / width), values[2] + (i * (values[3] - values[2]) / height))))
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
		Mandelbrot::line_completed[i] = true;
		Mandelbrot::write_condition[i].notify_one();
	});



}

template<typename T> void Mandelbrot::generate_nested_parallel_for_func(double values[4], T(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour, int threads)
{

	std::mutex image_mut;
	tbb::task_arena thread_limit(threads);
	thread_limit.execute([&] {

		tbb::parallel_for(0, height, [&](int i) {
			std::lock_guard<std::mutex> lg(line_mutex[i]);
			tbb::parallel_for(0, width, [&](int j) {
				
				if (Mandelbrot::compute_single_pixel((values[0] + (j * (values[1] - values[0]) / width), values[2] + (i * (values[3] - values[2]) / height))))
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
			Mandelbrot::line_completed[i] = true;
			Mandelbrot::write_condition[i].notify_one();
			});
		
		});



}

