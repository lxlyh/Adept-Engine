#include "Shader_SSAO.h"
#include "RHI/RHI.h"
#include "Core/BaseWindow.h"

float lerp(float a, float b, float f)
{
	return a + f * (b - a);
}
IMPLEMENT_GLOBAL_SHADER(Shader_SSAO);
Shader_SSAO::Shader_SSAO(DeviceContext* d) :Shader(d)
{
	m_Shader->AttachAndCompileShaderFromFile("PostProcess\\SSAOCS", EShaderType::SHADER_COMPUTE);
	std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between 0.0 - 1.0
	std::default_random_engine generator;

	for (int i = 0; i < 64; ++i)
	{
		glm::vec3 sample(
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator) * 2.0 - 1.0,
			randomFloats(generator)
		);
		sample = glm::normalize(sample);
		sample *= randomFloats(generator);
		float scale = float(i) / static_cast<float>(64.0);
		scale = lerp(0.1f, 1.0f, scale * scale);
		sample *= scale;
		ssaoKernel.push_back(sample);
	}

	for (int i = 0; i < 64; i++)
	{
		CurrentData.samples[i] = ssaoKernel[i];
	}
	DataBuffer = RHI::CreateRHIBuffer(ERHIBufferType::Constant);
	DataBuffer->CreateConstantBuffer(sizeof(CurrentData), 1);
}

void Shader_SSAO::Bind(RHICommandList* list)
{
	DataBuffer->UpdateConstantBuffer(&CurrentData);
	CurrentData.projection = BaseWindow::GetCurrentCamera()->GetProjection();
	CurrentData.view = BaseWindow::GetCurrentCamera()->GetView();
	list->SetConstantBufferView(DataBuffer, 0, "Data");
}

Shader_SSAO::~Shader_SSAO()
{}


void Shader_SSAO::Resize(int width, int height)
{
	mwidth = width;
	mheight = height;
}

IMPLEMENT_GLOBAL_SHADER(Shader_SSAO_Merge);

Shader_SSAO_Merge::Shader_SSAO_Merge(DeviceContext * d):Shader(d)
{
	m_Shader->AttachAndCompileShaderFromFile("PostProcess\\SSAOMergeCS", EShaderType::SHADER_COMPUTE);
}
