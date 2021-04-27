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
	
public:
	uint32_t image[height][width];
	std::atomic<uint32_t> image_atomic[height][width];
	std::mutex line_mutex[height];
	std::condition_variable write_condition[height]; // create a condition variable for each line that will
	bool line_completed[height] = { false }; //initialize the entire array to be false, since no line has been generated yet.

	void generate_original(double values[4], uint32_t bg_colour, uint32_t fg_colour); // The original function that was used in the lab example for generating the set, not parallelised at all.
	
	void write_tga(const char* filename, bool atomic);
	void write_tga_thread(const char* filename, bool atomic);

	
	template<typename T> void generate_parallel_for(double values[4], T (&img)[height][width], uint32_t bg_colour, uint32_t fg_colour );
	template<typename T> void limited_generate_parallel_for(double values[4], T(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour, int threads); 
	
	template<typename T> void generate_nested_parallel_for(double values[4], T(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour); // A parallel version of the function using nested parallel_for loops from TBB in order to generate the set.
	template<typename T> void limited_generate_nested_parallel_for(double values[4], T(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour, int threads); 
	
	template<typename T> void generate_nested_parallel_for_func(double values[4],  T(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour); // A parallel version of the function using nested parallel_for loops from TBB, which call the compute_single_pixel separately. Slowest out of all parallel functions.
	template<typename T> void limited_generate_nested_parallel_for_func(double values[4], T(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour,int threads); 
	

};

