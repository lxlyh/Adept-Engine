
#include "Shader_Bloom.h"
DECLARE_GLOBAL_SHADER(Shader_Bloom);
//Handles Threshold Pass for Bloom!
Shader_Bloom::Shader_Bloom(DeviceContext* device) :Shader(device)
{
	m_Shader = RHI::CreateShaderProgam(Device);

	m_Shader->AttachAndCompileShaderFromFile("BloomThreshold_CS", EShaderType::SHADER_COMPUTE);
}

Shader_Bloom::~Shader_Bloom()
{}

bool Shader_Bloom::IsComputeShader()
{
	return true;
}

std::vector<Shader::ShaderParameter> Shader_Bloom::GetShaderParameters()
{
	std::vector<Shader::ShaderParameter> Output;
	Output.push_back(ShaderParameter(ShaderParamType::SRV, 0, 0));
	Output.push_back(ShaderParameter(ShaderParamType::UAV, 1, 0));
	return Output;
}
