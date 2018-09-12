#pragma once
#include "Core/GameObject.h"
#include "Rendering/Core/Light.h"
#include <vector>
class RenderEngine;
class Scene
{
public:
	Scene();
	~Scene();
	void UpdateScene(float deltatime);
	void EditorUpdateScene();
	void OnFrameEnd(); 
	void FixedUpdateScene(float deltatime);
	CORE_API void AddGameobjectToScene(GameObject* gameobject);
	void CopyScene(Scene * newscene);
	std::vector<GameObject*>* GetObjects() { return &SceneObjects; }
	std::vector<GameObject*>* GetMeshObjects() { return &RenderSceneObjects; }
	void StartScene();
	void LoadDefault();
	void LoadExampleScene(RenderEngine * Renderer, bool IsDeferredMode);
	std::vector<Light*>* GetLights() { return &Lights; }
	Camera* GetCurrentRenderCamera() { return CurrentCamera; }
	void AddCamera(Camera* cam) { Cameras.emplace_back(cam); CurrentCamera = cam; }//todo: Camera priority
	void RemoveCamera(Camera* Cam);
	void AddLight(Light* Light) { Lights.emplace_back(Light); }//todo: Camera priority
	void RemoveLight(Light* Light);
	void RemoveGameObject(GameObject * object);
	bool StaticSceneNeedsUpdate = false;
	
	struct LightingEnviromentData
	{
		BaseTexture* SkyBox = nullptr;
		BaseTexture* DiffuseMap = nullptr;
	};
	LightingEnviromentData* GetLightingData() { return &LightingData; }
private:
	std::vector<GameObject*> SceneObjects;	
	std::vector<GameObject*> RenderSceneObjects;
	std::vector<Light*> Lights;
	std::vector<Camera*> Cameras;
	Camera* CurrentCamera = nullptr;
	
	
	LightingEnviromentData LightingData;
};

