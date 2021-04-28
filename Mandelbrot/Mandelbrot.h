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
constexpr int iterations = 500; 

// Using mandelbrot set example by Adam Sampson <a.sampson@abertay.ac.uk> as the base for this class
class Mandelbrot
{
private:
	
	bool compute_single_pixel(std::complex<double> c); // A piece of the original function, taking an already calculated c value and returning true/false depending on if that point exists in the set.
	
public:

	uint32_t image[height][width];
	std::atomic<uint32_t> image_atomic[height][width];
	
	std::mutex line_mutex[height];
	std::condition_variable write_condition[height]; // create a condition variable for each line that will
	bool line_completed[height] = { false }; //initialize the entire array to be false, since no line has been generated yet.

	void generate_original(double values[4], uint32_t bg_colour, uint32_t fg_colour); // The original function that was used in the lab example for generating the set, not parallelised at all.
	
	
	

	template<typename T> void generate_parallel_for(double values[4], T (&img)[height][width], uint32_t bg_colour, uint32_t fg_colour );
	template<typename T> void generate_parallel_for(double values[4], T(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour, int threads); 
	
	template<typename T> void generate_nested_parallel_for(double values[4], T(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour); 
	template<typename T> void generate_nested_parallel_for(double values[4], T(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour, int threads); 
	
	template<typename T> void generate_nested_parallel_for_func(double values[4],  T(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour); 
	template<typename T> void generate_nested_parallel_for_func(double values[4], T(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour,int threads); 
	

	

};



/// The code below was originally in Mandelbrot.cpp, however it needed to be moved into this file, since linker was giving me errors about the templated functions


// ---------- NON-PARALLEL FUNCTIONS ----------

void Mandelbrot::generate_original(double values[4], uint32_t bg_colour, uint32_t fg_colour) // regular, non-parallelized version of the funcion
{

	for (int y = 0; y < height; y++) // those loops will be parallelized, as they go over every pixel, one at a time
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


bool Mandelbrot::compute_single_pixel(std::complex<double> c) // this is the function called by the generate_nested_parallel_for_func. It decides if a single pixes does or does not belong to the set
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

template<> void Mandelbrot::generate_parallel_for(double values[4], std::uint32_t(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour) // generates the set with TBB's parallel_for using unique-lock for safe sharing of the array, with no manual thread  limit
{

	std::mutex image_mut; // mutex for sharing the array
	tbb::parallel_for(0, height, [&](int i) { // a for loop, but parallel, form TBB

		std::lock_guard<std::mutex> lg(line_mutex[i]); // obtain the mutex that is checked by the file-wriing thread
		for (int x = 0; x < width; x++) // non-parallel for loop, each task will generate an entire row.
		{
			
			std::complex<double> c(values[0] + (x * (values[1] - values[0]) / width), values[2] + (i * (values[3] - values[2]) / height)); //regular Mandelbrot set math
			std::complex<double> z(0.0, 0.0);
			int it = 0;

			while (abs(z) < 2.0 && it < iterations)
			{
				z = (z * z) + c;

				++it;
			}

			if (it == iterations) // if the point is in the set
			{
				std::unique_lock<std::mutex> lock(image_mut); //lock the mutex used for sharing the array until this object is desrtoyed.
				img[i][x] = fg_colour; //actually change the value in the array
				 
			}
			else
			{
				std::unique_lock<std::mutex> lock(image_mut);
				img[i][x] = bg_colour;
			}
		}
		
		Mandelbrot::line_completed[i] = true; //set the bool array that file-writing thread will consult
		Mandelbrot::write_condition[i].notify_one(); //notify the file-writing thread that a line has been completed (potentially the line that the thread is waiting for
		});



}

template<> void Mandelbrot::generate_parallel_for<std::atomic<std::uint32_t>> (double values[4], std::atomic<std::uint32_t>(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour) //generate the set with TBB's parallel_for, using a atomic array and with no manual thread limit
{
	// this, and some other specialisations of functions do not have the array sharing mutex, since they use the atomic version of the array, with which the mutex is not needed
	
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
				img[i][x] = fg_colour;

			}
			else
			{
				img[i][x] = bg_colour;
			}
		}
		
		Mandelbrot::line_completed[i] = true;
		Mandelbrot::write_condition[i].notify_one();
		});



}

template<> void Mandelbrot::generate_parallel_for<std::uint32_t>(double values[4], std::uint32_t(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour, int threads)
{

	std::mutex image_mut; 
	tbb::task_arena thread_limit(threads); 
	thread_limit.execute([&] {
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
			
			Mandelbrot::line_completed[i] = true;
			Mandelbrot::write_condition[i].notify_one();
			});
		});


}

template<> void Mandelbrot::generate_parallel_for<std::atomic<std::uint32_t>>(double values[4], std::atomic<std::uint32_t>(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour, int threads)
{

	
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
					
					img[i][x] = fg_colour;

				}
				else
				{
					
					img[i][x] = bg_colour;
				}
			}
			// here insert the thingy
			Mandelbrot::line_completed[i] = true;
			Mandelbrot::write_condition[i].notify_one();
			});
		});


}

template<> void Mandelbrot::generate_nested_parallel_for<std::uint32_t>(double values[4], std::uint32_t(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour)
{

	std::mutex image_mut;
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



}

template<> void Mandelbrot::generate_nested_parallel_for<std::atomic<std::uint32_t>>(double values[4], std::atomic<std::uint32_t>(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour)
{

	
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
				
				img[i][j] = fg_colour;

			}
			else
			{
				
				img[i][j] = bg_colour;

			}
			});
		Mandelbrot::line_completed[i] = true;
		Mandelbrot::write_condition[i].notify_one();
		});



}

template<> void Mandelbrot::generate_nested_parallel_for<std::uint32_t>(double values[4], std::uint32_t(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour, int threads)
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

template<> void Mandelbrot::generate_nested_parallel_for<std::atomic<std::uint32_t>>(double values[4], std::atomic<std::uint32_t>(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour, int threads)
{

	
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
					
					img[i][j] = fg_colour;

				}
				else
				{
					
					img[i][j] = bg_colour;

				}
				});
			Mandelbrot::line_completed[i] = true;
			Mandelbrot::write_condition[i].notify_one();
			});
		});




}

template<> void Mandelbrot::generate_nested_parallel_for_func<std::uint32_t>(double values[4], std::uint32_t(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour)
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

template<> void Mandelbrot::generate_nested_parallel_for_func<std::atomic<std::uint32_t>>(double values[4], std::atomic<std::uint32_t>(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour)
{

	
	tbb::parallel_for(0, height, [&](int i) {
		std::lock_guard<std::mutex> lg(line_mutex[i]);
		tbb::parallel_for(0, width, [&](int j) {

			if (Mandelbrot::compute_single_pixel((values[0] + (j * (values[1] - values[0]) / width), values[2] + (i * (values[3] - values[2]) / height))))
			{
				
				img[i][j] = fg_colour;

			}
			else
			{
				
				img[i][j] = bg_colour;

			}
			});
		Mandelbrot::line_completed[i] = true;
		Mandelbrot::write_condition[i].notify_one();
		});



}

template<> void Mandelbrot::generate_nested_parallel_for_func<std::uint32_t>(double values[4], std::uint32_t(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour, int threads)
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

template<> void Mandelbrot::generate_nested_parallel_for_func<std::atomic<std::uint32_t>>(double values[4], std::atomic<std::uint32_t>(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour, int threads)
{

	
	tbb::task_arena thread_limit(threads);
	thread_limit.execute([&] {

		tbb::parallel_for(0, height, [&](int i) {
			std::lock_guard<std::mutex> lg(line_mutex[i]);
			tbb::parallel_for(0, width, [&](int j) {

				if (Mandelbrot::compute_single_pixel((values[0] + (j * (values[1] - values[0]) / width), values[2] + (i * (values[3] - values[2]) / height))))
				{
					
					img[i][j] = fg_colour;

				}
				else
				{
					
					img[i][j] = bg_colour;

				}
				});
			Mandelbrot::line_completed[i] = true;
			Mandelbrot::write_condition[i].notify_one();
			});

		});



}