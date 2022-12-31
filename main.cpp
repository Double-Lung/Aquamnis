#include "VkDraw.h"
#include <stdexcept>
#include <iostream>

int main() 
{
	VkDraw app;

	try {
		app.Engage();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}