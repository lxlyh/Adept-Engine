#pragma once

#include "Core/Assets/AssetTypes.h"
class Asset_Shader
{
public:
	Asset_Shader();
	~Asset_Shader();
	bool IsGraph = false;
	class Material* GetMaterialInstance();

private:
#if WITH_EDITOR
	class ShaderGraph* Graph;
#endif

	AssetPathRef OutputFilePath;
};
