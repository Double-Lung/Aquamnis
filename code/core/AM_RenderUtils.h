#pragma once
#include <vector>
#include <fstream>

namespace AM_RenderUtils
{
	void ReadFile(std::vector<char>& outShaderCode, const std::string& aFilename);
}