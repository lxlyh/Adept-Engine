#include "RenderEngine.h"
#include "Core/Assets/Scene.h"
#include "Core/Performance/PerfManager.h"
#include "Core/Utils/StringUtil.h"
#include "Editor/Editor_Camera.h"
#include "Editor/EditorCore.h"
#include "Editor/EditorWindow.h"
#include "Rendering/Core/Culling/CullingManager.h"
#include "Rendering/Core/DebugLineDrawer.h"
#include "Rendering/Core/DynamicResolutionScaler.h"
#include "Rendering/Core/LightCulling/LightCullingEngine.h"
#include "Rendering/Core/Material.h"
#include "Rendering/Core/ParticleSystemManager.h"
#include "Rendering/Core/RelfectionProbe.h"
#include "Rendering/Core/RenderingUtils.h"
#include "Rendering/Core/SceneRenderer.h"
#include "Rendering/Core/ShadowRenderer.h"
#include "Rendering/PostProcessing/PostProcessing.h"
#include "Rendering/Shaders/Generation/Shader_EnvMap.h"
#include "Rendering/Shaders/PostProcess/Shader_Compost.h"
#include "Rendering/Shaders/Shader_Skybox.h"
#include "Rendering/VR/HMD.h"
#include "Rendering/VR/HMDManager.h"
#include "RHI/DeviceContext.h"
#include "RHI/RHI.h"
#include "../Shaders/Raytracing/Shader_Skybox_Miss.h"
#include "../RayTracing/RayTracingEngine.h"
#include "../Shaders/Shader_Deferred.h"
#include "Core/Assets/ShaderComplier.h"

RenderEngine::RenderEngine(int width, int height)
{
	m_width = width;
	m_height = height;
	SceneRender = new SceneRenderer(nullptr);
	Scaler = new DynamicResolutionScaler();
	Culling = new CullingManager();
	ScreenWriteList = RHI::CreateCommandList(ECommandListType::Graphics);
	LightCulling = new LightCullingEngine();
}

RenderEngine::~RenderEngine()
{
	DestoryRenderWindow();
	for (int i = 0; i < MAX_GPU_DEVICE_COUNT; i++)
	{
		DDOs[i].Release();
	}
	SafeDelete(SceneRender);
	SafeDelete(mShadowRenderer);
	SafeDelete(Post);
	SafeDelete(Culling);
}

void RenderEngine::Render()
{
	

	if (once)
	{
		RHI::RHIRunFirstFrame();
		once = false;
	}
	if (MainCamera == nullptr)
	{
		return;
	}
	if (MainScene->GetMeshObjects().size() == 0)
	{
		//return;
	}
	PreRender();
	ParticleSystemManager::Get()->Simulate();
#if BASIC_RENDER_ONLY
	UpdateMVForMainPass();
	return;
#endif
	OnRender();
}

void RenderEngine::PreRender()
{

	if (MainScene->StaticSceneNeedsUpdate)
	{
		StaticUpdate();
	}
	PrepareData();
	Scaler->Tick();
	SceneRender->LightsBuffer.LightCount = LightCulling->GetNumLights();
	SceneRender->UpdateLightBuffer(MainScene->GetLights());
#if WITH_EDITOR
	if (EditorCam != nullptr && EditorCam->GetEnabled())
	{
		if (MainCamera != EditorCam->GetCamera())
		{
			MainCamera = EditorCam->GetCamera();
		}
	}
	else
#endif
	{
		MainCamera = MainScene->GetCurrentRenderCamera();
	}
	ParticleSystemManager::Get()->PreRenderUpdate(MainCamera);

	//if using VR FS culling in run just before each eye
	if (!RHI::IsRenderingVR())
	{
		Culling->UpdateMainPassFrustumCulling(MainCamera, MainScene);
	}
	UpdateMVForMainPass();
#if TESTGRAPH
	//RunLightCulling();
#endif

}

//init common to both renderers
void RenderEngine::Init()
{
	if (RHI::GetMGPUSettings()->MainPassSFR)
	{
		DevicesInUse = 2;
	}
	else
	{
		DevicesInUse = 1;
	}

	SceneRender->Init();
#if BASIC_RENDER_ONLY
	return;
#endif
#if !NOSHADOW
	mShadowRenderer = new ShadowRenderer(SceneRender, Culling);
	if (MainScene != nullptr)
	{
		mShadowRenderer->InitShadows(MainScene->GetLights());
	}
#endif
	InitProcessingShaders(RHI::GetDeviceContext(0));
	InitProcessingShaders(RHI::GetDeviceContext(1));
#if !NOSHADOW
	CubemapCaptureList = RHI::CreateCommandList(ECommandListType::Graphics);
	RHIPipeLineStateDesc desc = RHIPipeLineStateDesc();
	desc.ShaderInUse = Material::GetDefaultMaterialShader();
	desc.FrameBufferTarget = DDOs[0].MainFrameBuffer;
	CubemapCaptureList->SetPipelineStateDesc(desc);
#endif
	DDOs[0].DebugCommandList = RHI::CreateCommandList(ECommandListType::Graphics, RHI::GetDeviceContext(0));
	PostInit();
	Post = new PostProcessing();
	Post->Init(DDOs[0].MainFrameBuffer);

	SceneRender->SB = DDOs[0].SkyboxShader;
	LightCulling->Init(Culling);

	//debug 
#if !NOSHADOW
	SceneRender->probes.push_back(new RelfectionProbe());
#endif
	//SceneRender->probes[0]->ProbeMode = EReflectionProbeMode::ERealTime;
	PostSizeUpdate();
}

void RenderEngine::InitProcessingShaders(DeviceContext* dev)
{
	if (dev == nullptr)
	{
		return;
	}
	if (dev->GetDeviceIndex() == 0)
	{
		DDOs[dev->GetDeviceIndex()].ConvShader = ShaderComplier::GetShader<Shader_Convolution>(dev);
		DDOs[dev->GetDeviceIndex()].EnvMap = ShaderComplier::GetShader<Shader_EnvMap>(dev);
	}
	else
	{
		DDOs[dev->GetDeviceIndex()].ConvShader = new Shader_Convolution(dev);
		DDOs[dev->GetDeviceIndex()].EnvMap = new Shader_EnvMap(dev);
	}
	DDOs[dev->GetDeviceIndex()].ConvShader->init();
	DDOs[dev->GetDeviceIndex()].EnvMap->Init();
}

void RenderEngine::ProcessSceneGPU(DeviceContext* dev)
{
	if (dev == nullptr)
	{
		return;
	}
	Scene::LightingEnviromentData* Data = MainScene->GetLightingData();
	DDOs[dev->GetDeviceIndex()].ConvShader->TargetCubemap = Data->SkyBox;
	DDOs[dev->GetDeviceIndex()].EnvMap->TargetCubemap = Data->SkyBox;
	dev->UpdateCopyEngine();
	dev->CPUWaitForAll();
	dev->ResetCopyEngine();
	if (Data->DiffuseMap == nullptr)
	{
		DDOs[dev->GetDeviceIndex()].ConvShader->ComputeConvolution(DDOs[dev->GetDeviceIndex()].ConvShader->TargetCubemap);
	}
	DDOs[dev->GetDeviceIndex()].EnvMap->ComputeEnvBRDF();

}

void RenderEngine::ProcessScene()
{
	if (MainScene == nullptr)
	{
		return;
	}
	//#Scene TEMP FIX!
	if (RHI::GetFrameCount() > 10)
	{
		return;
	}
	ProcessSceneGPU(RHI::GetDeviceContext(0));
	ProcessSceneGPU(RHI::GetDeviceContext(1));
	if (RHI::IsRenderingVR())
	{
		RHI::GetHMD()->UpdateProjection((float)GetScaledWidth() / (float)GetScaledHeight());
	}
	if (RHI::GetRenderSettings()->RaytracingEnabled())
	{
		ShaderComplier::GetShader<Shader_Skybox_Miss>()->SetSkybox(MainScene->GetLightingData()->SkyBox);
		RayTracingEngine::Get()->UpdateFromScene(MainScene);
	}
}

void RenderEngine::PrepareData()
{
	if (MainScene == nullptr)
	{
		return;
	}
	SceneRender->MeshController->GatherBatches();
	for (size_t i = 0; i < MainScene->GetMeshObjects().size(); i++)
	{
		MainScene->GetMeshObjects()[i]->PrepareDataForRender();
	}
}

void RenderEngine::Resize(int width, int height)
{
	Post->Resize(DDOs[0].MainFrameBuffer);

	if (RHI::IsRenderingVR())
	{
		RHI::GetHMD()->UpdateProjection((float)GetScaledWidth() / (float)GetScaledHeight());
	}
	else
	{
		if (MainCamera != nullptr)
		{
			MainCamera->UpdateProjection((float)GetScaledWidth() / (float)GetScaledHeight());
		}
	}
	int ApoxPValue = glm::iround((float)GetScaledWidth() / (16.0f / 9.0f));
	Log::OutS << "Resizing to " << GetScaledWidth() << "x" << GetScaledHeight() << " approx: " << ApoxPValue << "P " << Log::OutS;
	LightCulling->Resize();
	PostSizeUpdate();
}

void RenderEngine::StaticUpdate()
{
	if (mShadowRenderer != nullptr)
	{
		mShadowRenderer->InitShadows(MainScene->GetLights());
		mShadowRenderer->Renderered = false;
	}
	SceneRender->UpdateLightBuffer(MainScene->GetLights());
	OnStaticUpdate();
}

void RenderEngine::SetScene(Scene * sc)
{
	MainScene = sc;
	if (sc == nullptr)
	{
		MainCamera = nullptr;
		mShadowRenderer->ClearShadowLights();
		return;
	}
	SceneRender->SetScene(sc);
	ShaderComplier::GetShader<Shader_Skybox>()->SetSkyBox(sc->GetLightingData()->SkyBox);
	DDOs[0].SkyboxShader->SetSkyBox(sc->GetLightingData()->SkyBox);
	if (DDOs[1].SkyboxShader != nullptr)
	{
		DDOs[1].SkyboxShader->SetSkyBox(sc->GetLightingData()->SkyBox);
	}
#if !NOSHADOW
	if (mShadowRenderer != nullptr)
	{
		mShadowRenderer->InitShadows(MainScene->GetLights());
		mShadowRenderer->Renderered = false;

	}
#endif
	ProcessScene();
	if (sc == nullptr)
	{
		MainCamera = nullptr;
	}
	else
	{
		MainCamera = MainScene->GetCurrentRenderCamera();
	}
	HandleCameraResize();
}

void RenderEngine::SetEditorCamera(Editor_Camera * cam)
{
	EditorCam = cam;
}

void RenderEngine::ShadowPass()
{
#if !NOSHADOW
	if (mShadowRenderer != nullptr)
	{
		mShadowRenderer->RenderShadowMaps(MainCamera, MainScene->GetLights(), MainScene->GetMeshObjects(), MainShader);
	}
#endif
}

void RenderEngine::PostProcessPass()
{
	Post->ExecPPStack(&DDOs[0]);
}

void RenderEngine::PresentToScreen()
{
	//	return;
	ScreenWriteList->ResetList();
	ScreenWriteList->BeginRenderPass(RHI::GetRenderPassDescForSwapChain(true));
	if (RHI::IsRenderingVR())
	{
		if (RHI::GetVrSettings()->MirrorMode == EVRMirrorMode::Both)
		{
			RHIPipeLineStateDesc D = RHIPipeLineStateDesc::CreateDefault(ShaderComplier::GetShader<Shader_VROutput>());
			D.Cull = false;
			ScreenWriteList->SetPipelineStateDesc(D);

			ScreenWriteList->SetFrameBufferTexture(DDOs[0].MainFrameBuffer, EEye::Right);
			ScreenWriteList->SetFrameBufferTexture(DDOs[0].RightEyeFramebuffer, EEye::Left);
		}
		else
		{
			RHIPipeLineStateDesc D = RHIPipeLineStateDesc::CreateDefault(ShaderComplier::GetShader<Shader_Compost>());
			D.Cull = false;
			ScreenWriteList->SetPipelineStateDesc(D);
			if (RHI::GetVrSettings()->MirrorMode == EVRMirrorMode::Left)
			{
				ScreenWriteList->SetFrameBufferTexture(DDOs[0].MainFrameBuffer, 0);
			}
			else if (RHI::GetVrSettings()->MirrorMode == EVRMirrorMode::Right)
			{
				ScreenWriteList->SetFrameBufferTexture(DDOs[0].RightEyeFramebuffer, 0);
			}
		}
	}
	else
	{
		RHIPipeLineStateDesc D = RHIPipeLineStateDesc::CreateDefault(ShaderComplier::GetShader<Shader_Compost>());
		D.Cull = false;
		ScreenWriteList->SetPipelineStateDesc(D);
		ScreenWriteList->SetFrameBufferTexture(DDOs[0].MainFrameBuffer, 0);
	}
	RenderingUtils::RenderScreenQuad(ScreenWriteList);
	ScreenWriteList->EndRenderPass();
	ScreenWriteList->Execute();
	if (RHI::IsRenderingVR())
	{
		RHI::GetHMD()->OutputToEye(DDOs[0].MainFrameBuffer, EEye::Right);
		RHI::GetHMD()->OutputToEye(DDOs[0].RightEyeFramebuffer, EEye::Left);
	}
}

void RenderEngine::UpdateMVForMainPass()
{
	if (RHI::IsRenderingVR())
	{
		VRCamera* VRCam = RHI::GetHMD()->GetVRCamera();
		SceneRender->UpdateMV(VRCam);
	}
	else
	{
		SceneRender->UpdateMV(MainCamera);
	}
}

Camera * RenderEngine::GetMainCam()
{
	return MainCamera;
}

int RenderEngine::GetScaledWidth()
{
	if (RHI::GetRenderSettings()->LockBackBuffer)
	{
		return glm::iround(RHI::GetRenderSettings()->LockedWidth*RHI::GetRenderSettings()->GetCurrentRenderScale());
	}
	else
	{
		return (int)(m_width * RHI::GetRenderSettings()->GetCurrentRenderScale());
	}
}

int RenderEngine::GetScaledHeight()
{
	if (RHI::GetRenderSettings()->LockBackBuffer)
	{
		return glm::iround(RHI::GetRenderSettings()->LockedHeight*RHI::GetRenderSettings()->GetCurrentRenderScale());
	}
	else
	{
		return (int)(m_height * RHI::GetRenderSettings()->GetCurrentRenderScale());
	}
}

glm::ivec2 RenderEngine::GetScaledRes()
{
	return glm::ivec2(GetScaledWidth(), GetScaledHeight());
}

void RenderEngine::HandleCameraResize()
{
#if WITH_EDITOR
	if (MainCamera != nullptr)
	{
		if (EditorWindow::GetEditorCore()->LockAspect)
		{
			MainCamera->UpdateProjection(EditorWindow::GetEditorCore()->LockedAspect);
		}
		else
		{
			MainCamera->UpdateProjection((float)GetScaledWidth() / (float)GetScaledHeight());
		}

	}
#else
	if (MainCamera != nullptr)
	{
		MainCamera->UpdateProjection((float)GetScaledWidth() / (float)GetScaledHeight());
	}
#endif

}

Shader * RenderEngine::GetMainShader()
{
	return MainShader;
}

DeviceDependentObjects::~DeviceDependentObjects()
{
	SafeDelete(SkyboxShader);
}

void DeviceDependentObjects::Release()
{
	SafeRelease(Gbuffer);
	SafeRelease(MainFrameBuffer);
	SafeRelease(RightEyeFramebuffer);
	SafeRelease(RightEyeGBuffer);
	SafeRelease(RTBuffer);
	MemoryUtils::DeleteReleaseableCArray(MainCommandList, EEye::Limit);
	MemoryUtils::DeleteReleaseableCArray(GbufferWriteList, EEye::Limit);
	SafeDelete(DeferredShader);
	SafeRelease(DebugCommandList);	
}

FrameBuffer * DeviceDependentObjects::GetGBuffer(EEye::Type e)
{
	if (e == EEye::Left)
	{
		return Gbuffer;
	}
	else
	{
		return RightEyeGBuffer;
	}
}

FrameBuffer * DeviceDependentObjects::GetMain(int e)
{
	if (e == EEye::Left)
	{
		return MainFrameBuffer;
	}
	else
	{
		return RightEyeFramebuffer;
	}
}

void RenderEngine::CubeMapPass()
{
	if (!SceneRender->AnyProbesNeedUpdate())
	{
		return;
	}
	CubemapCaptureList->ResetList();
	RHIPipeLineStateDesc Desc = RHIPipeLineStateDesc::CreateDefault(Material::GetDefaultMaterialShader());
	CubemapCaptureList->SetPipelineStateDesc(Desc);
	if (mShadowRenderer != nullptr)
	{
		mShadowRenderer->BindShadowMapsToTextures(CubemapCaptureList, true);
	}
	CubemapCaptureList->SetFrameBufferTexture(DDOs[CubemapCaptureList->GetDeviceIndex()].ConvShader->CubeBuffer, MainShaderRSBinds::DiffuseIr);
	if (MainScene->GetLightingData()->SkyBox != nullptr)
	{
		CubemapCaptureList->SetTexture(MainScene->GetLightingData()->SkyBox, MainShaderRSBinds::SpecBlurMap);
	}
	CubemapCaptureList->SetFrameBufferTexture(DDOs[CubemapCaptureList->GetDeviceIndex()].EnvMap->EnvBRDFBuffer, MainShaderRSBinds::EnvBRDF);
	SceneRender->UpdateRelflectionProbes(CubemapCaptureList);

	CubemapCaptureList->Execute();
}


void RenderEngine::RenderDebug(FrameBuffer* FB, RHICommandList* list, EEye::Type eye)
{
	PerfManager::StartTimer("LineDrawer");
	DebugLineDrawer::Get()->RenderLines(FB, list, eye);
	DebugLineDrawer::Get2()->RenderLines(FB, list, eye);
	PerfManager::EndTimer("LineDrawer");
}

void RenderEngine::UpdateMainPassCulling(EEye::Type eye)
{

}

void RenderEngine::PostSizeUpdate()
{
	std::string Data = "";
	Data += "MainFrameBuffer Is " + StringUtils::ToStringFloat(DDOs[0].MainFrameBuffer->GetSizeOnGPU() / 1e6f) + " MB";
	if (DDOs[0].Gbuffer != nullptr)
	{
		Data += "\nGbuffer Is " + StringUtils::ToStringFloat(DDOs[0].Gbuffer->GetSizeOnGPU() / 1e6f) + " MB";
	}
	Log::LogMessage(Data);
}

void RenderEngine::RunLightCulling()
{
	
	LightCulling->LaunchCullingForScene(EEye::Left);
}

DynamicResolutionScaler * RenderEngine::GetDS()
{
	return Scaler;
}

void RenderEngine::RenderDebugPass()
{
	DDOs[0].DebugCommandList->ResetList();
	DDOs[0].DebugCommandList->StartTimer(EGPUTIMERS::DebugRender);
	if (RHI::IsRenderingVR())
	{
		RenderDebug(DDOs[0].MainFrameBuffer, DDOs[0].DebugCommandList, EEye::Left);
		RenderDebug(DDOs[0].RightEyeFramebuffer, DDOs[0].DebugCommandList, EEye::Right);
	}
	else
	{
		RenderDebug(DDOs[0].MainFrameBuffer, DDOs[0].DebugCommandList, EEye::Left);
	}
	DDOs[0].DebugCommandList->EndTimer(EGPUTIMERS::DebugRender);
	DDOs[0].DebugCommandList->Execute();
}


void RenderEngine::RunReflections(DeviceDependentObjects* d, EEye::Type eye)
{
	if (RHI::GetRenderSettings()->RaytracingEnabled())
	{
		if (RHI::GetRenderSettings()->GetRTSettings().UseForReflections)
		{
			RayTracingEngine::Get()->TraceRaysForReflections(d->RTBuffer, d->GetGBuffer(eye));
		}
		if (RHI::GetRenderSettings()->GetRTSettings().UseForMainPass)
		{
			//RayTracingEngine::Get()->DispatchRaysForMainScenePass(d->GetMain(eye));
		}
	}
}
