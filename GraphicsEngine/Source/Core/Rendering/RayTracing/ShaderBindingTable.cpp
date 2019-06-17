#include "ShaderBindingTable.h"
#include "Rendering/Shaders/Raytracing/Shader_RTMateralHit.h"
#include "Rendering/Shaders/Raytracing/Shader_Skybox_Miss.h"
#include "Shader_RTBase.h"
#include "Core/Assets/Scene.h"
#include "../Core/Mesh.h"
#include "Core/GameObject.h"
#include "../Core/Material.h"
#include "../Core/Mesh/MeshBatch.h"
#include "../Shaders/Raytracing/Reflections/Shader_ReflectionRaygen.h"


ShaderBindingTable::ShaderBindingTable()
{
	//InitDefault();
}

void ShaderBindingTable::InitDefault()
{
	//default debug
	MissShaders.push_back(ShaderComplier::GetShader<Shader_Skybox_Miss>());
	MissShaders[0]->AddExport("Miss");

	RayGenShaders.push_back(new Shader_RTBase(RHI::GetDefaultDevice(), "Raytracing\\DefaultRayGenShader", ERTShaderType::RayGen));
	RayGenShaders[0]->AddExport("rayGen");

	HitGroups.push_back(new ShaderHitGroup("HitGroup0"));
	HitGroups[0]->HitShader = new Shader_RTMateralHit(RHI::GetDefaultDevice());
	HitGroups[0]->HitShader->AddExport("chs");

	//rootParameters[GlobalRootSignatureParams::OutputViewSlot].InitAsDescriptorTable(1, &ranges[0]);
	//rootParameters[GlobalRootSignatureParams::AccelerationStructureSlot].InitAsShaderResourceView(0);
	//rootParameters[GlobalRootSignatureParams::CameraBuffer].InitAsConstantBufferView(0);
	GlobalRootSig.Params.push_back(ShaderParameter(ShaderParamType::UAV, 0, 0));
	GlobalRootSig.Params.push_back(ShaderParameter(ShaderParamType::RootSRV, 1, 0));
	GlobalRootSig.Params.push_back(ShaderParameter(ShaderParamType::CBV, 2, 0));
	
}

void ShaderBindingTable::InitReflections()
{
	//default debug
	MissShaders.push_back(ShaderComplier::GetShader<Shader_Skybox_Miss>());
	MissShaders[0]->AddExport("Miss");

	RayGenShaders.push_back(ShaderComplier::GetShader<Shader_ReflectionRaygen>());

	HitGroups.push_back(new ShaderHitGroup("HitGroup0"));
	HitGroups[0]->HitShader = new Shader_RTMateralHit(RHI::GetDefaultDevice());
	HitGroups[0]->HitShader->AddExport("chs");

	GlobalRootSig.Params.push_back(ShaderParameter(ShaderParamType::UAV, 0, 0));
	GlobalRootSig.Params.push_back(ShaderParameter(ShaderParamType::RootSRV, 1, 0));
	GlobalRootSig.Params.push_back(ShaderParameter(ShaderParamType::CBV, 2, 0));
	GlobalRootSig.Params.push_back(ShaderParameter(ShaderParamType::SRV, 3, 5));
	GlobalRootSig.Params.push_back(ShaderParameter(ShaderParamType::SRV, 4, 6));
	GlobalRootSig.Params.push_back(ShaderParameter(ShaderParamType::CBV, 5, 1));
}

ShaderBindingTable::~ShaderBindingTable()
{}
void ShaderBindingTable::RebuildHittableFromScene(Scene* Sc)
{
	HitGroups.clear();
	for (int i = 0; i < Sc->GetMeshObjects().size(); i++)
	{
		AddObject(Sc->GetMeshObjects()[i]);
	}
}
void ShaderBindingTable::AddObject(GameObject* Object)
{
	for (int i = 0; i < Object->GetMesh()->SubMeshes.size(); i++)
	{
		HitGroups.push_back(new ShaderHitGroup("HitGroup0"/* + std::to_string(HitGroups.size())*/));
		HitGroups[HitGroups.size() - 1]->HitShader = new Shader_RTMateralHit(RHI::GetDefaultDevice());
		HitGroups[HitGroups.size() - 1]->HitShader->AddExport("chs");
		Shader_RTBase* Shader = HitGroups[HitGroups.size() - 1]->HitShader;
		Shader->Buffers.push_back(Object->GetMesh()->SubMeshes[i]->IndexBuffers[0].Get());
		Shader->Buffers.push_back(Object->GetMesh()->SubMeshes[i]->VertexBuffers[0].Get());
		Shader->Textures.push_back(Object->GetMesh()->GetMaterial(0)->GetTexturebind("DiffuseMap"));
	}
}