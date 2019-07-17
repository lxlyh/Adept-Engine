#pragma once
#include "RHI\Shader.h"

class Shader_PreZ : public Shader
{
public:
	DECLARE_GLOBAL_SHADER(Shader_PreZ);
	Shader_PreZ(DeviceContext* d);
	~Shader_PreZ();	
	virtual std::vector<ShaderParameter> GetShaderParameters() override;

	std::vector<Shader::VertexElementDESC> GetVertexFormat();

};
