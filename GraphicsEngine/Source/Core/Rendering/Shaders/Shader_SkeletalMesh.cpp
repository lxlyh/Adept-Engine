#include "Shader_SkeletalMesh.h"
#include "RHI/Shader.h"
#include "Shader_Main.h"
#include "Core/Platform/PlatformCore.h"

IMPLEMENT_GLOBAL_SHADER(Shader_SkeletalMesh);
Shader_SkeletalMesh::Shader_SkeletalMesh(DeviceContext* dev) :Shader(dev)
{
	m_Shader->ModifyCompileEnviroment(ShaderProgramBase::Shader_Define("MAX_POINT_SHADOWS", std::to_string(std::max(RHI::GetRenderConstants()->MAX_DYNAMIC_POINT_SHADOWS, 1))));
	m_Shader->ModifyCompileEnviroment(ShaderProgramBase::Shader_Define("MAX_DIR_SHADOWS", std::to_string(std::max(RHI::GetRenderConstants()->MAX_DYNAMIC_DIRECTIONAL_SHADOWS, 1))));
	m_Shader->ModifyCompileEnviroment(ShaderProgramBase::Shader_Define("MAX_LIGHTS", std::to_string(RHI::GetRenderConstants()->MAX_LIGHTS)));
	m_Shader->ModifyCompileEnviroment(ShaderProgramBase::Shader_Define("TEST", "1"));
	m_Shader->AttachAndCompileShaderFromFile("Anim_vs", EShaderType::SHADER_VERTEX);
	//#TODO: fix me
	//m_Shader->AttachAndCompileShaderFromFile("Gen\\Default", EShaderType::SHADER_FRAGMENT);
	BonesBuffer = RHI::CreateRHIBuffer(ERHIBufferType::Constant);
	BonesBuffer->CreateConstantBuffer(sizeof(BoneData), 1);
}
//Anim_vs

Shader_SkeletalMesh::~Shader_SkeletalMesh()
{
	EnqueueSafeRHIRelease(BonesBuffer);
}

std::vector<ShaderParameter> Shader_SkeletalMesh::GetShaderParameters()
{
	std::vector<ShaderParameter> Output;
	Shader_Main::GetMainShaderSig(Output);
	Output.push_back(ShaderParameter(ShaderParamType::CBV, 8, 5));
	Output.push_back(ShaderParameter(ShaderParamType::SRV, 9, 20));
	return Output;
}

std::vector<VertexElementDESC> Shader_SkeletalMesh::GetVertexFormat()
{
	std::vector<VertexElementDESC> foamt = Shader_Main::GetMainVertexFormat();//ends on 32 
	foamt.push_back(VertexElementDESC{ "BLENDINDICES", 0, R32G32B32A32_UINT, 0, 44, EInputClassification::PER_VERTEX, 0 });
	foamt.push_back(VertexElementDESC{ "TEXCOORD", 2, R32G32B32A32_FLOAT, 0, 44 + 16, EInputClassification::PER_VERTEX, 0 });
	return foamt;
}

void Shader_SkeletalMesh::PushBones(std::vector<glm::mat4x4>& bonetrans, RHICommandList* list)
{
	ensure(bonetrans.size() < MAX_BONES);
	if (bonetrans.size() == 0)
	{
		return;
	}
	for (int i = 0; i < bonetrans.size(); i++)
	{
		boneD.Bones[i] = bonetrans[i];
	}
	BonesBuffer->UpdateConstantBuffer(&boneD, 0);
	list->SetConstantBufferView(BonesBuffer, 0, 8);
}