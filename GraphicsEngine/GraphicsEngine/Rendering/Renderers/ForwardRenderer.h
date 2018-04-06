#pragma once
#include "RenderEngine.h"

#include <vector>
#include "../Rendering/Core/Camera.h"
#include "Core/Transform.h"
#include "Core/GameObject.h"
#include "../Rendering/Core/Light.h"
#include "OpenGL/OGLPlane.h"
#include "OpenGL/OGLShader.h"
#include "../D3D12/D3D12RHI.h"

#include "../Rendering/Core/Material.h"
#include "RHI/Shader.h"
#include "Rendering\Shaders\Shader_Main.h"
#include "Rendering\Shaders\Shader_Depth.h"
#include "Rendering\Shaders\Shader_Grass.h"
#include "Rendering\Shaders\Shader_Querry.h"
#include "Core/Assets/Scene.h"

#include "Rendering\Shaders\ShaderOutput.h"
#include "../Rendering/Core/FrameBuffer.h"
#include "../Rendering/Core/ParticleSystem.h"
#include <memory>
#include "../Rendering/Core/ShadowRenderer.h"
#include "../Rendering/Core/GPUStateCache.h"
#include "../Rendering/Core/GrassPatch.h"
#include "Core/Assets/ImageLoader.h"
#include "Rendering\Shaders\Shader_Skybox.h"
#include "OpenGL/OGLFrameBuffer.h"
#include "../Core/Performance/PerfManager.h"
#include "../EngineGlobals.h"
#include "../RHI/RHICommandList.h"
class OGLShaderProgram;
class ForwardRenderer : public RenderEngine
{
public:
	ForwardRenderer(int width, int height);
	void RunQuery();
	void Resize(int width, int height);

	~ForwardRenderer();
	void Render() override;
	std::vector<GameObject*> InGetObj();
	void UpdateDeltaTime(float value);
	void Init()override;
	void ReflectionPass();
	void ShadowPass();
	void BindAsRenderTarget();
	void PrepareData();
	void RenderDebugPlane();
	void MainPass();

	void RenderSkybox(bool ismain = true);
	void RenderFitlerBufferOutput();
	virtual std::vector<GameObject*> GetObjects() override;
	void SetScene(Scene * sc) override;

	void TESTINIT();

	float deltatime = 1;
	clock_t tstart;
	Camera* GetMainCam() override;
	void AddGo(GameObject* g)override;

	void AddLight(Light* l) override;
	void FixedUpdatePhysx(float dtime) override;
	Camera* RefelctionCamera;
	FrameBuffer* RelfectionBuffer;
	ShaderOutput* outshader;
	bool RenderParticles = true;
	bool RenderGrass = true;
	bool UseQuerry = false;
	void SetRenderSettings(RenderSettings settings) override;
private:
#if BUILD_D3D12
	CommandListDef* MainList = nullptr;
	CommandListDef* ShadowList = nullptr;
#endif
	FrameBuffer* FilterBuffer = nullptr;
	std::unique_ptr<ParticleSystem> particlesys;
	std::unique_ptr<GrassPatch>  grasstest;
	std::vector<GameObject*>* Objects;
	class D3D12Plane* debugplane = nullptr;
	//std::vector<GameObject*> ob;
	std::vector<GameObject*> PhysicsObjects;
	//TextRenderer
	Camera*     MainCamera = nullptr;
	std::vector<Light*>* Lights;
	float FrameBufferRatio = 1;

	GameObject* skybox = nullptr;
	GameObject* playerGO = nullptr;
	ShadowRenderer* shadowrender;
	D3D12RHI * DRHI = nullptr;
	Shader_Main* mainshader = nullptr;
	Shader_Grass* grassshader = nullptr;
	Shader* QuerryShader = nullptr;
	GameObject* testGrassobject = nullptr;
	std::unique_ptr<Shader_Skybox> skyboxShader;
	bool RenderedReflection = false;
	bool LoadGrass = true;

	bool LoadParticles = true;
	class PostProcessing* Post = nullptr;

	int ReflectionBufferWidth = 512;
	int ReflectionBufferHeight = 512;
	// Inherited via RenderEngine
	virtual Shader_Main * GetMainShader() override;

	// Inherited via RenderEngine
	virtual void SetReflectionCamera(Camera * c) override;


	// Inherited via RenderEngine
	virtual FrameBuffer * GetReflectionBuffer() override;


	// Inherited via RenderEngine
	virtual ShaderOutput * GetFilterShader() override;
	
	virtual void DestoryRenderWindow() override;



	// Inherited via RenderEngine
	virtual void FinaliseRender() override;
	RHICommandList* MainCommandList;
	RHICommandList* ShadowCMDList = nullptr;
	bool once = true;
};

