#pragma once
#include <cstdint>
#include <complex>
#include "tbb/parallel_for.h"
#include <vector>
#include <iostream>
#include <mutex>
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
	
	void generate_original(double values[4], uint32_t bg_colour, uint32_t fg_colour); // The original function that was used in the lab example for generating the set, not parallelised at all.
	
	void write_tga(const char* filename, bool atomic);
	
	void write_tga_thread(const char* filename, bool atomic);

	
	void generate_parallel_for(double values[4], uint32_t (&img)[height][width], uint32_t bg_colour, uint32_t fg_colour ); // A parallel version of the function, uses a for loop wrapped in a parallel_for from TBB in order to generate the set.
	void generate_parallel_for(double values[4], std::atomic<uint32_t> (&img)[height][width], uint32_t bg_colour, uint32_t fg_colour);// And it's overloaded version with atomic instead of normal 2D array. This overload does not contain mutexes.

	void generate_nested_parallel_for(double values[4], uint32_t(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour); // A parallel version of the function using nested parallel_for loops from TBB in order to generate the set.
	void generate_nested_parallel_for(double values[4], std::atomic<uint32_t>(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour); // And it's overloaded version with atomic 2D array. This overload does not contain mutexes.

	void generate_nested_parallel_for_func(double values[4],  uint32_t(&img)[height][width], uint32_t bg_colour, uint32_t fg_colour); // A parallel version of the function using nested parallel_for loops from TBB, which call the compute_single_pixel separately. Slowest out of all parallel functions.
	void generate_nested_parallel_for_func(double values[4],  std::atomic<uint32_t> (&img)[height][width], uint32_t bg_colour, uint32_t fg_colour);
	

	///////////////////////////////////////////////////////////////////////////////////////////
	//TODO ADD SIGNALING (SEMAPHORES OR STH) FOR THE SAVE TO FILE THREAD SO YOU MEET ALL REQS//
	///////////////////////////////////////////////////////////////////////////////////////////
};

