#include "VoxelTracingEngine.h"
#include "../Shaders/Shader_Pair.h"
#include "RHI/RHITexture.h"
#include "../Core/Mesh/MeshBatch.h"
#include "../Core/SceneRenderer.h"
#include "RHI/RHITimeManager.h"
#include "../Core/Mesh/MeshPipelineController.h"
VoxelTracingEngine* VoxelTracingEngine::Instance = nullptr;

void VoxelTracingEngine::VoxliseTest(RHICommandList * list)
{
	if (!RHI::GetRenderSettings()->GetVoxelSet().Enabled)
	{
		return;
	}
	RHIPipeLineStateDesc desc = RHIPipeLineStateDesc::CreateDefault(voxeliseShader);
	desc.DepthStencilState.DepthEnable = false;
	desc.RasterizerState.ConservativeRaster = true;
	desc.RasterizerState.Cull = false;
	desc.Cull = false;
	list->SetPipelineStateDesc(desc);
	list->SetViewport(0, 0, size, size, 0, 1.0f);
	list->SetUAV(VoxelMap, "voxelTex");
	MeshPassRenderArgs Args;
	Args.PassType = ERenderPass::VoxelPass;
	Args.PassData = this;
	SceneRenderer::Get()->MeshController->RenderPass(Args, list);
	//list->UAVBarrier(VoxelMap);
	//uav barrier?
}

void VoxelTracingEngine::RenderVoxelDebug(RHICommandList* list, FrameBuffer* buffer)
{
	if (!RHI::GetRenderSettings()->GetVoxelSet().Enabled)
	{
		return;
	}
	DECALRE_SCOPEDGPUCOUNTER(list,"RenderVoxelDebug");
	RHIPipeLineStateDesc desc = RHIPipeLineStateDesc::CreateDefault(DebugvoxeliseShader, buffer);
	desc.RasterizerState.Cull = false;
	desc.RasterMode = PRIMITIVE_TOPOLOGY_TYPE_POINT;
	desc.DepthStencilState.DepthEnable = true;
	desc.Cull = false;
	list->SetPipelineStateDesc(desc);
	list->BeginRenderPass(RHIRenderPassDesc(buffer,ERenderPassLoadOp::Clear));
	RHIViewDesc d = RHIViewDesc::DefaultSRV();
	d.Dimension = DIMENSION_TEXTURE3D;
	list->SetTexture2(VoxelMap, 0, d);
	SceneRenderer::Get()->BindMvBuffer(list);
	list->DrawPrimitive(size*size*size, 1, 0, 0);
	list->EndRenderPass();
}

VoxelTracingEngine::VoxelTracingEngine()
{
	if (!RHI::GetRenderSettings()->GetVoxelSet().Enabled)
	{
		return;
	}
	voxeliseShader = new Shader_Pair(RHI::GetDefaultDevice(), { "Voxel\\Voxelise_VS","Voxel\\Voxelise_GS","Voxel\\Voxelise_PS" },
		{ EShaderType::SHADER_VERTEX,EShaderType::SHADER_GEOMETRY,EShaderType::SHADER_FRAGMENT }, { ShaderProgramBase::Shader_Define("MAX_INSTANCES", std::to_string(RHI::GetRenderConstants()->MAX_MESH_INSTANCES)) });
	DebugvoxeliseShader = new Shader_Pair(RHI::GetDefaultDevice(), { "Voxel\\VoxelDebugOut_VS","Voxel\\VoxelDebugOut_GS","Voxel\\VoxelDebugOut_PS" },
		{ EShaderType::SHADER_VERTEX,EShaderType::SHADER_GEOMETRY,EShaderType::SHADER_FRAGMENT });
	VoxelMap = RHI::GetRHIClass()->CreateTexture2();
	RHITextureDesc2 Desc = {};
	size = 250;
	Desc.Width = size;
	Desc.Height = size;
	Desc.Depth = size;
	Desc.clearcolour = glm::vec4(0, 0, 0, 0);
	Desc.AllowUnorderedAccess = true;
	Desc.Dimension = DIMENSION_TEXTURE3D;
	Desc.Format = FORMAT_R32G32B32A32_UINT;
	Desc.Name = "Voxel Struct";
	VoxelMap->Create(Desc);
}

VoxelTracingEngine* VoxelTracingEngine::Get()
{
	if (Instance == nullptr)
	{
		Instance = new VoxelTracingEngine();
	}
	return Instance;
}
