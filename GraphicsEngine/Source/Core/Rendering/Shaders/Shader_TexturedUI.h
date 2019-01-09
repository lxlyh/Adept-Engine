#pragma once
#include "RHI\Shader.h"
class DeviceContext;
class Shader_TexturedUI : public Shader
{
public:
	DECLARE_GLOBAL_SHADER(Shader_TexturedUI);
	Shader_TexturedUI(DeviceContext* d);

	~Shader_TexturedUI();
	void Render();

	virtual std::vector<ShaderParameter> GetShaderParameters() override;

	std::vector<Shader::VertexElementDESC> GetVertexFormat() override;
	BaseTexture* Texture = nullptr;
	bool blend = true;
private:
	RHIBuffer * VertexBuffer = nullptr;
	void Init();
	RHICommandList* list = nullptr;
	RHIPipeLineStateObject* BlendPSO = nullptr;
	RHIPipeLineStateObject* NoBlendPSO = nullptr;
};

