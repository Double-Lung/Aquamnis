#include "AM_RenderUtils.h"

void AM_RenderUtils::ReadFile(std::vector<char>& outShaderCode, const std::string& aFilename)
{
	std::ifstream file(aFilename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error("failed to open file!");

	size_t fileSize = static_cast<size_t>(file.tellg());
	outShaderCode.resize(fileSize);
	file.seekg(0);
	file.read(outShaderCode.data(), fileSize);
	file.close();
}
