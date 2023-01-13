#include "VkDraw.h"

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