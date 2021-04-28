#include "Mandelbrot.h"

 void write_tga(const char* name, std::uint32_t img[height][width]) // Same write function as the lab example, with very minor changes
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

	for (int y = 0; y < height; ++y)
	{
		for (int x = 0; x < width; ++x)
		{
			uint8_t pixel[3] = {
				 img[y][x] & 0xFF, // blue channel
				(img[y][x] >> 8) & 0xFF, // green channel
				(img[y][x] >> 16) & 0xFF, // red channel
			};
			outfile.write((const char*)pixel, 3);
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


 void write_tga_thread(const char* name, bool atomic, Mandelbrot* obj)
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
			std::unique_lock<std::mutex> ul(obj->line_mutex[y]); //obtain the mutex and try to lock it
			obj->write_condition[y].wait(ul, [&] { if (obj->line_completed[y]) return true; else return false; }); //check if the line we want to write is done, if not, block until woken up, if yes, proceed
			//block until the line with the ID of y is completed
			if (atomic)
			{
				for (int x = 0; x < width; x++)
				{
					uint8_t pixel[3] = {
						 obj->image_atomic[y][x] & 0xFF, // blue channel
						(obj->image_atomic[y][x] >> 8) & 0xFF, // green channel
						(obj->image_atomic[y][x] >> 16) & 0xFF, // red channel
					};
					outfile.write((const char*)pixel, 3);
				}
			}
			else
			{
				for (int x = 0; x < width; x++)
				{
					uint8_t pixel[3] = {
						 obj->image[y][x] & 0xFF, // blue channel
						(obj->image[y][x] >> 8) & 0xFF, // green channel
						(obj->image[y][x] >> 16) & 0xFF, // red channel
					};
					outfile.write((const char*)pixel, 3);
				}
			}
		}
	
	
}




int main()
{
	int selection = 0;
	int func;
	int threads = 0;
	bool atomic,colour,manual_threads,manual_values;
	double args[4] = { -2.0,1.0,1.125,-1.125 };
	const char filename[15] = "Mandelbrot.tga";
	uint32_t bg_colour = 0xFFFFFF;
	uint32_t fg_colour = 0x000000;

	//'menu' code
	std::cout << "Welcome! Please make your selection!:" << std::endl << "1) Generate Mandelbrot set using the lab, non-parallel example" << std::endl << "2) Generate Mandelbrot set using single parallel_for" << std::endl << "3) Generate Mandelbrot set using nested parallel_for" << std::endl << "4) Generate Mandelbrot set using nested parallel_for calls to a function" << std::endl;
	std::cin >> func;
	selection += 100 * func;
	if (func != 1)
	{
		std::cout << "Would you like to use unique_lock mutex or atomic variable for safe sharing of resources?" << std::endl << "0) unique_lock" << std::endl << "1) aotmic" << std::endl;
		std::cin >> atomic;
		selection += 10 * atomic;

		std::cout << "Do you want to set number of threads generaitng the set manually? " << std::endl << "0) No" << std::endl << "1) Yes" << std::endl;
		std::cin >> manual_threads;
		selection += manual_threads;
		if (manual_threads)
		{
			std::cout << "Please input the number of threads you want to limit TBB to (Note: This will not affect the file-writing thread. This thread runs in all configurations except the original, non parallel one. Writing to file is included in the timing!):" << std::endl;
			std::cin >> threads;

		}
	}
	std::cout << "Do you want custom colours to be used?" << std::endl << "0) No" << std::endl << "1) Yes" << std::endl;
	std::cin >> colour;
	if (colour)
	{
		std::cout << "Please input hex value for the background colour (e.g. 0xFFFFFF) ";
		std::cin >> std::hex >> bg_colour;
		std::cout << std::endl << "Please input hex value for the foreground colour (e.g. 0x000000) ";
		std::cin >> std::hex >> fg_colour;
	}
	
	std::cout << "Do you want to manually input the 4 values used in generation instead of using the default ones? " << std::endl << "0) No" << std::endl << "1) yes " << std::endl;
	std::cin >> manual_values;
	if (manual_values)
	{
		std::cout << "Please input the left,right,top and bottom values to be used when creating the set:" << std::endl << "Original values are (-2.0,1.0,1.125,-1.125)" << std::endl;
		std::cout << "Left: ";
		std::cin >> args[0]; // -2.0
		std::cout << "Right: ";
		std::cin >> args[1]; // 1.0
		std::cout << "Top: ";
		std::cin >> args[2]; // 1.125
		std::cout << "Bottom: ";
		std::cin >> args[3]; // -1.125
	}
	

	Mandelbrot* image = new Mandelbrot; //allocate a new mandelbrot object
	std::chrono::steady_clock::time_point start,stop; // declare the start and stop point variables in the broader scope than start will be initialized in.
	

	switch (selection) // branch off based on user's choices
	{
	case 100: // original code - no parallelization
	{
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();													//start the timer
		image->generate_original(args, bg_colour, fg_colour);																			// generate the set using original, non-parallel code.
		write_tga(filename,image->image);																								// save the generated set into a file
		break;

	}
	case 211: // parallel for - atomic variable - thread limit 
	{
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();													// Start the clock
		std::thread write(write_tga_thread, filename, atomic, image);																	// Start the file-writing thread
		image->generate_parallel_for(args, image->image_atomic, bg_colour, fg_colour,threads);											// Call the generation function, which will call parallel_for, spawning threads (up to the set linit)
		write.join();																													//Wait for the file-writing thread to finish. File-writing thread will only finish after all lines of the set are completed.
		break;
	}
	case 210: // parallel for - atomic variable - no thread limit
	{
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();													// Start the clock
		std::thread write(write_tga_thread, filename, atomic, image);																	// Start the file-writing thread
		image->generate_parallel_for(args, image->image_atomic, bg_colour, fg_colour);													// Call the generation function, which will call parallel_for, spawning threads (TBB will automatically handle how many threads it needs
		write.join();																													//Wait for the file-writing thread to finish. File-writing thread will only finish after all lines of the set are completed.
		break;
	}
	case 201: // parallel for - unique_lock - thread limit
	{
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now(); 
		std::thread write(write_tga_thread, filename, atomic, image); 
		image->generate_parallel_for(args, image->image, bg_colour, fg_colour,threads);
		write.join(); 
		break;
	}
	case 200: // parallel for - unique_lock - no thread limit
	{
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now(); 
		std::thread write(write_tga_thread, filename, atomic, image); 
		image->generate_parallel_for(args, image->image, bg_colour, fg_colour);
		write.join();
		break;
	}
	case 311: // nested parallel for - atomic variable - thread limit
	{
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
		std::thread write(write_tga_thread, filename, atomic, image);
		image->generate_nested_parallel_for(args, image->image_atomic, bg_colour, fg_colour,threads);
		write.join(); 
		break;
	}
	case 310: // nested parallel for - atomic variable - no thread limit
	{
		
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now(); 
		std::thread write(write_tga_thread, filename, atomic, image); 
		image->generate_nested_parallel_for(args, image->image_atomic, bg_colour, fg_colour);
		write.join(); 
		break;
		
	}
	case 301: // nested parallel for - unique_lock - thread limit
	{
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
		std::thread write(write_tga_thread, filename, atomic, image); 
		image->generate_nested_parallel_for(args, image->image, bg_colour, fg_colour, threads);
		write.join(); 
		break;
	
	}
	case 300: // nested parallel for - unique_lock - no thread limit
	{
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now(); 
		std::thread write(write_tga_thread, filename, atomic, image); 
		image->generate_nested_parallel_for(args, image->image, bg_colour, fg_colour);
		write.join(); 
		break;
		
	}
	case 411: // nested parallel for calling a function - atomic variable - thread limit
	{
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now(); 
		std::thread write(write_tga_thread, filename, atomic, image); 
		image->generate_nested_parallel_for_func(args, image->image_atomic, bg_colour, fg_colour,threads);
		write.join(); 
		break;
		
	}
	case 410: // nested parallel for calling a function - atomic variable - no thread limit
	{
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now(); 
		std::thread write(write_tga_thread, filename, atomic, image); 
		image->generate_nested_parallel_for_func(args, image->image_atomic, bg_colour, fg_colour);
		write.join(); 
		break;
		
	}	
	case 401: // nested parallel for calling a function - unique_lock - thread limit
	{
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now(); 
		std::thread write(write_tga_thread, filename, atomic, image); 
		image->generate_nested_parallel_for_func(args, image->image, bg_colour, fg_colour, threads);
		write.join();
		break;
		
	}
	case 400: // nested parallel for calling a function - unique_lock - no thread limit
	{
		start = std::chrono::steady_clock::now(); 
		std::thread write(write_tga_thread, filename, atomic, image); 
		image->generate_nested_parallel_for_func(args, image->image, bg_colour, fg_colour);
		write.join(); 
		break;
		
	}
	default:
		std::cout << "Something went wrong, you should not end up here, were all your inputs correct?" << std::endl;
		break;
	}

	stop = std::chrono::steady_clock::now();																						//stop the timer
	std::cout << "It took " << std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count() << " ms" << std::endl;	// Display the time it took
	
	std::cout << "All done, you can now safely close.";
	while (1) //useless bit of code, that will prevent the window from closing on it's own after the program is finished
	static int x = std::getchar();
	
	return 0;
}