#pragma once
#include "..\ShaderBindingTable.h"
class PathTraceBindingTable : public ShaderBindingTable
{
public:
	PathTraceBindingTable();
	virtual ~PathTraceBindingTable();
	virtual void InitTable() override;

	virtual Shader_RTMateralHit* GetMaterialShader() override;

protected:
	virtual void OnMeshProcessed(Mesh* Mesh, MeshEntity* E, Shader_RTBase* Shader) override;

};

