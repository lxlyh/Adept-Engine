
#include "ParticleSystemManager.h"
#include "Core/Assets/ShaderComplier.h"
#include "Rendering/Shaders/Particle/Shader_ParticleCompute.h"
#include "Rendering/Shaders/Particle/Shader_ParticleDraw.h"
#include "RHI/DeviceContext.h"
#include "Core/Assets/AssetManager.h"
ParticleSystemManager* ParticleSystemManager::Instance = nullptr;

ParticleSystemManager::ParticleSystemManager()
{
	Init();
}

ParticleSystemManager::~ParticleSystemManager()
{}

void ParticleSystemManager::Init()
{
	//return;
	GPU_ParticleData = RHI::CreateRHIBuffer(ERHIBufferType::GPU);
	RHIBufferDesc desc;
	desc.Accesstype = EBufferAccessType::GPUOnly;
	desc.Stride = sizeof(ParticleData);
	desc.ElementCount = CurrentParticleCount;
	desc.AllowUnorderedAccess = true;
	desc.CreateUAV = true;
	GPU_ParticleData->SetDebugName("Particle Data");
	GPU_ParticleData->CreateBuffer(desc);


	CounterBuffer = RHI::CreateRHIBuffer(ERHIBufferType::GPU);
	desc.Accesstype = EBufferAccessType::Static;
	desc.Stride = sizeof(Counters);
	desc.ElementCount = 1;
	desc.CreateUAV = true;
	CounterBuffer->SetDebugName("Counter");
	CounterBuffer->CreateBuffer(desc);
	Counters count = Counters();
	count.deadCount = CurrentParticleCount;
	CounterBuffer->UpdateIndexBuffer(&count, sizeof(Counters));

	CmdList = RHI::CreateCommandList(ECommandListType::Compute);
	CmdList->SetPipelineStateDesc(RHIPipeLineStateDesc::CreateDefault(ShaderComplier::GetShader<Shader_ParticleCompute>()));

	SetupCommandBuffer();


	AliveParticleIndexs = RHI::CreateRHIBuffer(ERHIBufferType::GPU);
	desc = RHIBufferDesc();
	desc.Stride = sizeof(unsigned int);
	desc.AllowUnorderedAccess = true;
	desc.Accesstype = EBufferAccessType::GPUOnly;
	desc.ElementCount = CurrentParticleCount;
	desc.CreateUAV = true;
	AliveParticleIndexs->SetDebugName("Alive Pre sim");
	AliveParticleIndexs->CreateBuffer(desc);



	AliveParticleIndexs_PostSim = RHI::CreateRHIBuffer(ERHIBufferType::GPU);
	desc = RHIBufferDesc();
	desc.Stride = sizeof(unsigned int);
	desc.AllowUnorderedAccess = true;
	desc.Accesstype = EBufferAccessType::GPUOnly;
	desc.ElementCount = CurrentParticleCount;
	desc.CreateUAV = true;
	AliveParticleIndexs_PostSim->SetDebugName("Alive Post Sim");
	AliveParticleIndexs_PostSim->CreateBuffer(desc);



	DeadParticleIndexs = RHI::CreateRHIBuffer(ERHIBufferType::GPU);
	desc = RHIBufferDesc();
	desc.Stride = sizeof(unsigned int);
	desc.AllowUnorderedAccess = true;
	desc.Accesstype = EBufferAccessType::Static;
	desc.ElementCount = CurrentParticleCount;
	desc.CreateUAV = true;
	DeadParticleIndexs->SetDebugName("Dead particle indexes");
	DeadParticleIndexs->CreateBuffer(desc);
	uint32_t* indices = new uint32_t[MAX_PARTICLES];
	for (uint32_t i = 0; i < MAX_PARTICLES; ++i)
	{
		indices[i] = i;
	}
	DeadParticleIndexs->UpdateBufferData(indices, MAX_PARTICLES, EBufferResourceState::UnorderedAccess);

	delete[] indices;

	//test
	TEstTex = AssetManager::DirectLoadTextureAsset("texture\\smoke.png", true);
}

void ParticleSystemManager::SetupCommandBuffer()
{
	DispatchCommandBuffer = RHI::CreateRHIBuffer(ERHIBufferType::GPU);
	RHIBufferDesc desc;
	desc.Accesstype = EBufferAccessType::Static;
	desc.Stride = sizeof(IndirectDispatchArgs) * 2;
	desc.ElementCount = 1;
	desc.AllowUnorderedAccess = true;
	desc.CreateUAV = true;
	DispatchCommandBuffer->SetDebugName("Dispatch Command Buffer");
	DispatchCommandBuffer->CreateBuffer(desc);
	DispatchArgs Data[2];
	Data[0].dispatchArgs = IndirectDispatchArgs();
	Data[0].dispatchArgs.ThreadGroupCountX = 1;
	Data[0].dispatchArgs.ThreadGroupCountY = 1;
	Data[0].dispatchArgs.ThreadGroupCountZ = 1;
	Data[1].dispatchArgs = IndirectDispatchArgs();
	Data[1].dispatchArgs.ThreadGroupCountX = 1;
	Data[1].dispatchArgs.ThreadGroupCountY = 1;
	Data[1].dispatchArgs.ThreadGroupCountZ = 1;
	DispatchCommandBuffer->UpdateBufferData(Data, sizeof(IndirectDispatchArgs) * 2, EBufferResourceState::UnorderedAccess);


#if USE_INDIRECTCOMPUTE
	CmdList->SetUpCommandSigniture(sizeof(DispatchArgs), true);
#endif
	RenderCommandBuffer = RHI::CreateRHIBuffer(ERHIBufferType::GPU);
	desc.Accesstype = EBufferAccessType::Static;
	desc.Stride = sizeof(IndirectArgs);
	desc.ElementCount = CurrentParticleCount;
	desc.AllowUnorderedAccess = true;
	desc.CreateUAV = true;
	RenderCommandBuffer->SetDebugName("RenderCommandBuffer");
	RenderCommandBuffer->CreateBuffer(desc);
	std::vector<IndirectArgs> cmdbuffer;
	cmdbuffer.resize(CurrentParticleCount);
	for (int i = 0; i < cmdbuffer.size(); i++)
	{
		cmdbuffer[i].drawArguments.VertexCountPerInstance = 6;
		cmdbuffer[i].drawArguments.InstanceCount = 1;
		cmdbuffer[i].drawArguments.StartVertexLocation = 0;
		cmdbuffer[i].drawArguments.StartInstanceLocation = 0;
		cmdbuffer[i].data = 0;
	}
	RenderCommandBuffer->UpdateBufferData(cmdbuffer.data(), cmdbuffer.size(), EBufferResourceState::UnorderedAccess);



	RenderList = RHI::CreateCommandList();
	//PipeLineState pls = PipeLineState();
	//pls.Cull = false;
	//pls.RenderTargetDesc = RHIPipeRenderTargetDesc();
	//pls.RenderTargetDesc.RTVFormats[0] = eTEXTURE_FORMAT::FORMAT_R32G32B32A32_FLOAT;
	//pls.RenderTargetDesc.NumRenderTargets = 1;
	//pls.RenderTargetDesc.DSVFormat = FORMAT_D32_FLOAT;
	//pls.Blending = true;
	//RenderList->SetPipelineState_OLD(pls);
	//RenderList->SetPipelineStateObject_OLD(ShaderComplier::GetShader<Shader_ParticleDraw>());
	RHIPipeLineStateDesc pdesc;
	pdesc.Cull = false;
	pdesc.RenderTargetDesc = RHIPipeRenderTargetDesc();
	pdesc.RenderTargetDesc.RTVFormats[0] = eTEXTURE_FORMAT::FORMAT_R32G32B32A32_FLOAT;
	pdesc.RenderTargetDesc.NumRenderTargets = 1;
	pdesc.RenderTargetDesc.DSVFormat = FORMAT_D32_FLOAT;
	pdesc.Blending = false;
	pdesc.ShaderInUse = ShaderComplier::GetShader<Shader_ParticleDraw>();
	RenderList->SetPipelineStateDesc(pdesc);
#if 1//USE_INDIRECTCOMPUTE
	RenderList->SetUpCommandSigniture(sizeof(IndirectArgs), false);
#endif


	float g_quad_vertex_buffer_data[] = {
	-1.0f, -1.0f, 0.0f,0.0f,
	1.0f, -1.0f, 0.0f,0.0f,
	-1.0f,  1.0f, 0.0f,0.0f,
	-1.0f,  1.0f, 0.0f,0.0f,
	1.0f, -1.0f, 0.0f,0.0f,
	1.0f,  1.0f, 0.0f,0.0f,
	};
	VertexBuffer = RHI::CreateRHIBuffer(ERHIBufferType::Vertex);
	VertexBuffer->CreateVertexBuffer(sizeof(float) * 4, sizeof(float) * 6 * 4);
	VertexBuffer->UpdateVertexBuffer(&g_quad_vertex_buffer_data, sizeof(float) * 6 * 4);
	ParticleRenderConstants = RHI::CreateRHIBuffer(ERHIBufferType::Constant);
	ParticleRenderConstants->CreateConstantBuffer(sizeof(RenderData), 1);
}

void ParticleSystemManager::PreRenderUpdate(Camera* c)
{
	RenderData.CameraUp_worldspace = glm::vec4(c->GetUp(), 1.0f);
	RenderData.CameraRight_worldspace = glm::vec4(c->GetRight(), 1.0f);
	RenderData.VPMat = c->GetViewProjection();
	if (ParticleRenderConstants)
	{
		ParticleRenderConstants->UpdateConstantBuffer(&RenderData, 0);
	}
}

ParticleSystemManager * ParticleSystemManager::Get()
{
	if (Instance == nullptr)
	{
		Instance = new ParticleSystemManager();
	}
	return Instance;
}

void ParticleSystemManager::ShutDown()
{
	EnqueueSafeRHIRelease(GPU_ParticleData);
	EnqueueSafeRHIRelease(CmdList);
	EnqueueSafeRHIRelease(RenderCommandBuffer);
	EnqueueSafeRHIRelease(RenderList);
	EnqueueSafeRHIRelease(VertexBuffer);
	EnqueueSafeRHIRelease(ParticleRenderConstants);
	EnqueueSafeRHIRelease(CounterBuffer);
	EnqueueSafeRHIRelease(DispatchCommandBuffer);
	EnqueueSafeRHIRelease(AliveParticleIndexs);
	EnqueueSafeRHIRelease(DeadParticleIndexs);
	EnqueueSafeRHIRelease(TEstTex);
	EnqueueSafeRHIRelease(AliveParticleIndexs_PostSim);
}

void ParticleSystemManager::Sync()
{

	CmdList->UAVBarrier(AliveParticleIndexs->GetUAV());
	CmdList->UAVBarrier(DeadParticleIndexs->GetUAV());
	CmdList->UAVBarrier(CounterBuffer->GetUAV());
	CmdList->UAVBarrier(DispatchCommandBuffer->GetUAV());
	CmdList->UAVBarrier(AliveParticleIndexs_PostSim->GetUAV());
	CmdList->UAVBarrier(GPU_ParticleData->GetUAV());
}

RHIBuffer* ParticleSystemManager::GetPreSimList()
{
	if (Flip)
	{
		return AliveParticleIndexs_PostSim;
	}
	return AliveParticleIndexs;
}

RHIBuffer* ParticleSystemManager::GetPostSimList()
{
	if (Flip)
	{
		return AliveParticleIndexs;
	}
	return AliveParticleIndexs_PostSim;
}

void ParticleSystemManager::Simulate()
{
	//return;
	CmdList->ResetList();
	CmdList->StartTimer(EGPUTIMERS::ParticleSimulation);
	DispatchCommandBuffer->SetBufferState(CmdList, EBufferResourceState::UnorderedAccess);
	CmdList->SetPipelineStateDesc(RHIPipeLineStateDesc::CreateDefault(ShaderComplier::GetShader<Shader_StartSimulation>()));
	CounterBuffer->GetUAV()->Bind(CmdList, 0);
	DispatchCommandBuffer->GetUAV()->Bind(CmdList, 1);
	emitcount++;
	int on = (emitcount % 10 == 0);
	CmdList->SetRootConstant(2, 1, &on, 0);
	CmdList->Dispatch(1, 1, 1);
	Sync();
	DispatchCommandBuffer->SetBufferState(CmdList, EBufferResourceState::IndirectArgs);

	CmdList->SetPipelineStateDesc(RHIPipeLineStateDesc::CreateDefault(ShaderComplier::GetShader<Shader_ParticleEmit>()));
	CounterBuffer->GetUAV()->Bind(CmdList, 1);
	GPU_ParticleData->GetUAV()->Bind(CmdList, 0);
	GetPreSimList()->GetUAV()->Bind(CmdList, 2);

	DeadParticleIndexs->GetUAV()->Bind(CmdList, 3);
#if USE_INDIRECTCOMPUTE
	CmdList->ExecuteIndiect(1, DispatchCommandBuffer, 0, nullptr, 0);
#else
	CmdList->Dispatch(MAX_PARTICLES, 1, 1);
#endif
	Sync();
	CmdList->SetPipelineStateDesc(RHIPipeLineStateDesc::CreateDefault(ShaderComplier::GetShader<Shader_ParticleCompute>()));
	GPU_ParticleData->GetUAV()->Bind(CmdList, 0);
	CounterBuffer->GetUAV()->Bind(CmdList, 1);
	GetPreSimList()->BindBufferReadOnly(CmdList, 2);
	DeadParticleIndexs->GetUAV()->Bind(CmdList, 3);
	GetPostSimList()->GetUAV()->Bind(CmdList, 4);
#if USE_INDIRECTCOMPUTE
	CmdList->ExecuteIndiect(1, DispatchCommandBuffer, sizeof(DispatchArgs), nullptr, 0);
#else
	CmdList->Dispatch(MAX_PARTICLES, 1, 1);
#endif
	Sync();
	CmdList->SetPipelineStateDesc(RHIPipeLineStateDesc::CreateDefault(ShaderComplier::GetShader<Shader_EndSimulation>()));

	GetPostSimList()->BindBufferReadOnly(CmdList, 0);
	RenderCommandBuffer->GetUAV()->Bind(CmdList, 1);
	CounterBuffer->GetUAV()->Bind(CmdList, 2);
#if USE_INDIRECTCOMPUTE
	CmdList->ExecuteIndiect(1, DispatchCommandBuffer, sizeof(DispatchArgs), nullptr, 0);
#else
	CmdList->Dispatch(MAX_PARTICLES, 1, 1);
#endif
	Sync();
	RenderCommandBuffer->SetBufferState(CmdList, EBufferResourceState::IndirectArgs);
	CmdList->EndTimer(EGPUTIMERS::ParticleSimulation);
	CmdList->Execute();
	CmdList->GetDevice()->InsertGPUWait(DeviceContextQueue::Graphics, DeviceContextQueue::Compute);
}

void ParticleSystemManager::Render(FrameBuffer* BufferTarget)
{
	//return;
	RenderList->ResetList();
	RenderList->StartTimer(EGPUTIMERS::ParticleDraw);
	RHIPipeLineStateDesc desc;
	desc.ShaderInUse = ShaderComplier::GetShader<Shader_ParticleDraw>();
	desc.FrameBufferTarget = BufferTarget;
	desc.Cull = false;
	desc.Blending = true;
	RenderList->SetPipelineStateDesc(desc);
	RenderList->SetRenderTarget(BufferTarget);
	RenderList->SetVertexBuffer(VertexBuffer);
	RenderList->SetConstantBufferView(ParticleRenderConstants, 0, 2);
	GPU_ParticleData->SetBufferState(RenderList, EBufferResourceState::Read);
	GPU_ParticleData->BindBufferReadOnly(RenderList, 1);
	RenderCommandBuffer->SetBufferState(RenderList, EBufferResourceState::IndirectArgs);
	RenderList->SetTexture(TEstTex, 3);
#if USE_INDIRECTCOMPUTE
	RenderList->ExecuteIndiect(MAX_PARTICLES, RenderCommandBuffer, 0, nullptr, 0);
#else
	for (int i = 0; i < MAX_PARTICLES; i++)
	{
		RenderList->SetRootConstant(0, 1, &i, 0);
		RenderList->DrawPrimitive(6, 1, 0, 0);
	}
#endif
	GPU_ParticleData->SetBufferState(RenderList, EBufferResourceState::UnorderedAccess);
	RenderCommandBuffer->SetBufferState(RenderList, EBufferResourceState::UnorderedAccess);
	RenderList->EndTimer(EGPUTIMERS::ParticleDraw);
	RenderList->Execute();
	CmdList->GetDevice()->InsertGPUWait(DeviceContextQueue::Compute, DeviceContextQueue::Graphics);
	Flip = !Flip;
}
