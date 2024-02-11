#include "Cafe.h"
#include <exception>
#include <iostream>

int main() 
{
	Cafe app;

	try {
		app.Engage();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}