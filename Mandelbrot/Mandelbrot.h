#pragma once
#include <cstdint>
#include <complex>
#include "tbb/parallel_for.h"
#include <vector>
#include <iostream>
#include <mutex>

#define height 1080
#define width 1920
constexpr int iterations = 1000; // At first I wanted to use #define for all 3, but compiler was not happy with this one being used in the while loop for some reason.
// Using mandelbrot set example by Adam Sampson <a.sampson@abertay.ac.uk> as the base for this class
class Mandelbrot
{
public:
	uint32_t image[height][width];
	
	void generate_original(double left,double right,double top, double bot); // The original function that was used in the lab example for generating the set, not parallelised at all.
	bool compute_single_pixel(std::complex<double> c); // A piece of the original function, taking an already calculated c value and returning true/false depending on if that point exists in the set.

	// TODO CHANGE THOSE TO BE OVERLOADED WITH / WITHOUT ATOMIC
	void generate_parallel_for(double left, double right, double top, double bot, uint32_t (&img)[height][width] ); // A parallel version of the function, uses a for loop wrapped in a parallel_for from TBB in order to generate the set.
	void generate_parallel_for(double left, double right, double top, double bot, std::atomic<uint32_t> (&img)[height][width]);// And it's overloaded version with atomic instead of normal 2D array. This overload does not contain mutexes.

	void generate_nested_parallel_for(double left, double right, double top, double bot); // A parallel version of the function using nested parallel_for loops from TBB in order to generate the set.
	void generate_nested_parallel_for(double left, double right, double top, double bot); // And it's overloaded version with atomic 2D array. This overload does not contain mutexes.

	void generate_nested_parallel_for_func(double left, double right, double top, double bot); // A parallel version of the function using nested parallel_for loops from TBB, which call the compute_single_pixel separately. Slowest out of all parallel functions.

	

	std::atomic<uint32_t> image_atomic[height][width];
};

