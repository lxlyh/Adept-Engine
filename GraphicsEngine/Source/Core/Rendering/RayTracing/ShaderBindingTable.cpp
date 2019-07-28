#include "ShaderBindingTable.h"
#include "Core/Assets/Scene.h"
#include "Rendering/Shaders/Raytracing/Shader_RTMateralHit.h"

ShaderBindingTable::ShaderBindingTable()
{}

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
		HitGroups.push_back(new ShaderHitGroup("HitGroup0"));
		HitGroups[HitGroups.size() - 1]->HitShader = new Shader_RTMateralHit(RHI::GetDefaultDevice());
		Shader_RTBase* Shader = HitGroups[HitGroups.size() - 1]->HitShader;

		Shader->LocalRootSig.SetBufferReadOnly(DefaultLocalRootSignatureParams::IndexBuffer, Object->GetMesh()->SubMeshes[i]->IndexBuffers[0].Get());
		Shader->LocalRootSig.SetBufferReadOnly(DefaultLocalRootSignatureParams::VertexBuffer, Object->GetMesh()->SubMeshes[i]->VertexBuffers[0].Get());
		OnMeshProcessed(Object->GetMesh(), Object->GetMesh()->SubMeshes[i], Shader);
	}
}

void ShaderBindingTable::InitTable()
{}

void ShaderBindingTable::OnMeshProcessed(Mesh* Mesh, MeshEntity* E, Shader_RTBase* Shader)
{}
