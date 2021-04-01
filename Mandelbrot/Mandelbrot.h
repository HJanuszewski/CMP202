#pragma once
#include <cstdint>
#include <complex>
#include<amp.h>
#include <vector>
#define height 1440
#define width 3440
constexpr int iterations = 500; // At first I wanted to use #define for all 3, but compiler was not happy with this one being used in the while loop for some reason.
// Using mandelbrot set example by Adam Sampson <a.sampson@abertay.ac.uk> as the base for this class
class Mandelbrot
{
public:
	//std::vector<std::vector<uint32_t>> image;
	uint32_t image[height][width];
	void generate(double left,double right,double top, double bot);
	void generate_GPU(double left, double right, double top, double bot);
};

