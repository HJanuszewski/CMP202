#include <chrono>
#include "Mandelbrot.h"





int main()
{
	int selection = 0;
	bool atomic,colour;
	double args[4];
	uint32_t bg_colour, fg_colour;
	std::cout << "Welcome! Please make your selection!:" << std::endl << "1) Generate Mandelbrot set using the lab, non-parallel example" << std::endl << "2) Generate Mandelbrot set using single parallel_for" << std::endl << "3) Generate Mandelbrot set using nested parallel_for" << std::endl;
	std::cin >> selection;
	if (selection != 1)
	{
		std::cout << "Would you like to use unique_lock mutex or atomic variable for safe sharing of resources?" << std::endl << "0) unique_lock" << std::endl << "1) aotmic" << std::endl;
		std::cin >> atomic;
	}
	std::cout << "Do you want custom colours to be used?" << std::endl << "0) No" << std::endl << "1) Yes" << std::endl;
	std::cin >> colour;
	if (colour)
	{
		std::cout << "Please input hex value for the background colour (e.g. 0xFFFFFF) ";
		std::cin >> bg_colour;
		std::cout << std::endl << "Please input hex value for the foreground colour (e.g. 0x000000) ";
		std::cin >> fg_colour;
	}

	std::cout << "Please input the left,right,top and bottom values to be used when creating the set:" << std::endl;
	std::cout << "Left: ";
	std::cin >> args[0];
	std::cout << "Right: ";
	std::cin >> args[1];
	std::cout << "Top: ";
	std::cin >> args[2];
	std::cout << "Bottom: ";
	std::cin >> args[3];


	Mandelbrot* image = new Mandelbrot;
	switch (selection)
	{
	case 1:
		image->generate_original(args);
		break;
	case 2:
		if (atomic) 
		{
			image->generate_parallel_for(args,image->image_atomic);
		}
		else
		{
			image->generate_parallel_for(args, image->image);
		}
		break;
	case 3:
		if (atomic)
		{
			image->generate_nested_parallel_for(args, image->image_atomic);
		}
		else
		{
			image->generate_nested_parallel_for(args, image->image);
		}
		break;
	case 4:
		if (atomic)
		{
			image->generate_nested_parallel_for_func(args, image->image_atomic);
		}
		else
		{
			image->generate_nested_parallel_for_func(args, image->image);
		}
	default:
		break;
	}


	//Mandelbrot* image = new Mandelbrot;
	//std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now(); // start point of the timing
	//image->generate_nested_parallel_for(-2.0, 1.0, 1.125, -1.125,image->image);
	
	//std::chrono::steady_clock::time_point stop = std::chrono::steady_clock::now(); // stop point of the timing.
	//image->write_tga("output.tga", image->image);
//	std::cout << "It took " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << " ms" << std::endl;
	
	return 0;
}