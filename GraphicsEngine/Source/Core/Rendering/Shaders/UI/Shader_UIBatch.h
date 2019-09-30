#pragma once
#include "RHI/Shader.h"
class Shader_UIBatch :
	public Shader
{
	struct UnifromData
	{
		glm::mat4x4 Proj;
	};
public:
	DECLARE_GLOBAL_SHADER(Shader_UIBatch);
	Shader_UIBatch(class DeviceContext* dev);
	virtual ~Shader_UIBatch();
	std::vector<Shader::VertexElementDESC> GetVertexFormat() override;
	void PushTOGPU(RHICommandList * list);
	std::vector<ShaderParameter> GetShaderParameters() override;
	void UpdateUniforms(glm::mat4x4 Proj);
private:
	UnifromData data;
	RHIBuffer* UniformBuffer = nullptr;
};
