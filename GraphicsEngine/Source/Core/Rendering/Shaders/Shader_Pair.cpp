#include "Shader_Pair.h"


Shader_Pair::Shader_Pair(DeviceContext* context, const std::vector<std::string>& names, const std::vector<EShaderType::Type>& stageList, const std::vector<ShaderProgramBase::Shader_Define>& defines) :Shader(context)
{
	Names = names;
	StageList = stageList;
	Defines = defines;
	Init();
}

Shader_Pair::~Shader_Pair()
{}

Shader_Pair * Shader_Pair::CreateShader(std::string VertexName, std::string PixelName, DeviceContext* context)
{
	return new Shader_Pair(context, std::vector<std::string>{ VertexName, PixelName }, std::vector<EShaderType::Type>{ EShaderType::SHADER_VERTEX, EShaderType::SHADER_FRAGMENT });
}

Shader_Pair * Shader_Pair::CreateComputeShader(std::string Compute, DeviceContext* context)
{
	return new Shader_Pair(context, std::vector<std::string>{ Compute }, std::vector<EShaderType::Type>{ EShaderType::SHADER_COMPUTE });
}

const std::string Shader_Pair::GetName()
{
	return "Shader_Pair_" + shadername;
}

bool Shader_Pair::IsPartOfGlobalShaderLibrary = false;

void Shader_Pair::Init()
{
	uint64 MaxIndex = Math::Min(Names.size(), StageList.size());
	for (uint64 i = 0; i < Defines.size(); i++)
	{
		m_Shader->ModifyCompileEnviroment(Defines[i]);
	}
	for (uint64 i = 0; i < MaxIndex; i++)
	{
		m_Shader->AttachAndCompileShaderFromFile(Names[i].c_str(), StageList[i]);
		shadername += Names[i];
	}
#ifdef PLATFORM_WINDOWS
	//ensure(IsPartOfGlobalShaderLibrary);	
#endif
}
