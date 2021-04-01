#pragma once
#include <cstdint>
#define height 1440
#define width 3440
#define iterations 500;
// Using mandelbrot set example by Adam Sampson <a.sampson@abertay.ac.uk> as the base for this class
class Mandelbrot
{
public:
	uint32_t image[height][width];
	void generate(double left,double right,double top, double bot);
};

