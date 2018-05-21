#include "ForwardRenderer.h"
#include "RHI/RHI.h"
#include "Core/Components/MeshRendererComponent.h"

#include "../EngineGlobals.h"
#include "../PostProcessing/PostProcessing.h"
#include "../Core/Engine.h"
#include "../RHI/RenderAPIs/D3D12/D3D12TimeManager.h"
#if USED3D12DebugP
#include "../D3D12/D3D12Plane.h"
#endif
ForwardRenderer::ForwardRenderer(int width, int height) :RenderEngine(width, height)
{
	
#if USED3D12DebugP
		debugplane = new D3D12Plane(1);
#endif
}

void ForwardRenderer::Resize(int width, int height)
{
	m_width = width;
	m_height = height;
	if (RHI::IsD3D12())
	{
		if (D3D12RHI::Instance)
		{
			D3D12RHI::Instance->ResizeSwapChain(width, height);
		}
	}
	if (MainCamera != nullptr)
	{
		MainCamera->UpdateProjection((float)width / (float)height);
	}
}

ForwardRenderer::~ForwardRenderer()
{}

void ForwardRenderer::OnRender()
{
	ShadowPass();
	MainPass();
	RenderSkybox();
	PostProcessPass();

	
}


void ForwardRenderer::PostInit()
{
	MainShader = new Shader_Main();
	FilterBuffer = RHI::CreateFrameBuffer(m_width, m_height, RHI::GetDeviceContext(0), 1.0f, FrameBuffer::ColourDepth);
	if (MainScene == nullptr)
	{
		MainScene = new Scene();
	}
	MainCommandList = RHI::CreateCommandList();
	//finally init the pipeline!
	MainCommandList->CreatePipelineState(MainShader,FilterBuffer);
	SkyBox = new Shader_Skybox();
	SkyBox->Init(FilterBuffer,nullptr);
#if DEBUG_CUBEMAPS
	SkyBox->test = mShadowRenderer->PointLightBuffer;
#endif
}


void ForwardRenderer::RenderDebugPlane()
{
#if (_DEBUG) && USED3D12DebugP
	MainList->SetGraphicsRootSignature(((D3D12Shader*)outshader->GetShaderProgram())->m_Shader.m_rootSignature);
	MainList->SetPipelineState(((D3D12Shader*)outshader->GetShaderProgram())->m_Shader.m_pipelineState);
	debugplane->Render(MainList);
#endif

}
void ForwardRenderer::MainPass()
{
	MainCommandList->ResetList();
	
	D3D12TimeManager::Instance->StartTimer(MainCommandList);
	MainCommandList->SetScreenBackBufferAsRT();
	MainCommandList->ClearScreen();	
	MainShader->UpdateMV(MainCamera);	
	MainShader->BindLightsBuffer(MainCommandList);

	mShadowRenderer->BindShadowMapsToTextures(MainCommandList);
	MainShader->UpdateMV(MainCamera);
	MainCommandList->SetRenderTarget(FilterBuffer);
	MainCommandList->ClearFrameBuffer(FilterBuffer);
	for (size_t i = 0; i < (*MainScene->GetObjects()).size(); i++)
	{
		MainShader->SetActiveIndex(MainCommandList, (int)i);
		MainCommandList->SetTexture(SkyBox->SkyBoxTexture, 0);
		(*MainScene->GetObjects())[i]->Render(false, MainCommandList);
	}
	
	MainCommandList->SetRenderTarget(nullptr);
//	D3D12TimeManager::Instance->EndTimer(MainCommandList);
	MainCommandList->Execute();
}

void ForwardRenderer::RenderSkybox()
{
	SkyBox->Render(MainShader, FilterBuffer,nullptr);
}

void ForwardRenderer::DestoryRenderWindow()
{
	delete MainCommandList;
}

void ForwardRenderer::FinaliseRender()
{

}

void ForwardRenderer::OnStaticUpdate()
{}

