#include "AM_VkRenderCore.h"

int main() 
{
	AM_VkRenderCore app;

	try {
		app.Engage();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}