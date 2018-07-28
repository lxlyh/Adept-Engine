#include "stdafx.h"
#include "PostProcessEffectBase.h"
#include "RHI/RHI.h"
#include "RHI/DeviceContext.h"
#include "RHI/RenderAPIs/D3D12/D3D12TimeManager.h"
PostProcessEffectBase::PostProcessEffectBase()
{}
void PostProcessEffectBase::SetUpData()
{
	float g_quad_vertex_buffer_data[] = {
		-1.0f, -1.0f, 0.0f,0.0f,
		1.0f, -1.0f, 0.0f,0.0f,
		-1.0f,  1.0f, 0.0f,0.0f,
		-1.0f,  1.0f, 0.0f,0.0f,
		1.0f, -1.0f, 0.0f,0.0f,
		1.0f,  1.0f, 0.0f,0.0f,
	};
	VertexBuffer = RHI::CreateRHIBuffer(RHIBuffer::BufferType::Vertex);
	VertexBuffer->CreateVertexBuffer(sizeof(float)*4, sizeof(float)*6*4);
	VertexBuffer->UpdateVertexBuffer(&g_quad_vertex_buffer_data, sizeof(float) * 6 * 4);

	//test
	PostSetUpData();
}


PostProcessEffectBase::~PostProcessEffectBase()
{
	delete CMDlist;
	delete VertexBuffer;
}

void PostProcessEffectBase::InitEffect(FrameBuffer* Target)
{
	PostInitEffect(Target);
}
void PostProcessEffectBase::RunPass(FrameBuffer* InputTexture)
{
	CMDlist->ResetList();
	CMDlist->GetDevice()->GetTimeManager()->StartTimer(CMDlist, EGPUTIMERS::PostProcess);
	ExecPass(CMDlist, InputTexture);
	CMDlist->GetDevice()->GetTimeManager()->EndTimer(CMDlist, EGPUTIMERS::PostProcess);
	CMDlist->Execute();
	PostPass();
}

void PostProcessEffectBase::RenderScreenQuad(RHICommandList * list)
{
	//todo: less than full screen!
	list->SetVertexBuffer(VertexBuffer);
	list->DrawPrimitive(6, 1, 0, 0);
}

//void PostProcessEffectBase::ExecPass(RHICommandList * list, FrameBuffer * InputTexture)
//{
//	/*list->SetScreenBackBufferAsRT();
//	list->SetFrameBufferTexture(InputTexture, 0);
//	RenderScreenQuad(list);*/
//}
