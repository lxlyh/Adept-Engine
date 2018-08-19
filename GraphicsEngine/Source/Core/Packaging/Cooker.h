#pragma once
class Cooker
{
public:
	Cooker();
	Cooker(class AssetManager * ASM);
	~Cooker();
	std::string GetTargetPath(bool AppendSlash = false);

	void CopyToOutput();
	void CopyFolderToOutput(std::string Target, std::string PathFromBuild);
		bool CopyAssetToOutput(std::string RelTarget);
	void CreatePackage();
private:
	std::string OutputPath = "\\Build";
	AssetManager* AssetM;
};

