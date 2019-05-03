#pragma once
#include "RHI/RHI.h"
#include "RHI/RHI_inc_fwd.h"
#define MAX_POSSIBLE_LIGHTS 128
class MeshPipelineController;
typedef struct _LightUniformBuffer
{
	glm::vec3 position;
	float t;
	glm::vec3 color;
	float t2;
	glm::vec3 Direction;
	float t3;
	glm::mat4x4 LightVP;
	int type;//type 1 == point, type 0 == directional, type 2 == spot
	int ShadowID;
	int DirShadowID;
	int HasShadow = 0;
	int PreSampled[4];//padding sucks!

}LightUniformBuffer;

struct MVBuffer
{
	glm::mat4 V;
	glm::mat4 P;
	glm::vec3 CameraPos;
};

struct LightBufferW
{
	LightUniformBuffer Light[MAX_POSSIBLE_LIGHTS];
}; 

/*__declspec(align(32))*/ struct SceneConstantBuffer//CBV need to be 256 aligned
{
	glm::mat4 M;
	int HasNormalMap = 0;
	float Roughness = 0.0f;
	float Metallic = 0.0f;
};
struct MeshTransfromBuffer
{
	glm::mat4 M;
};
class RelfectionProbe;
class SceneRenderer
{
public:
	SceneRenderer(class Scene* Target);
	~SceneRenderer();
	void RenderScene(RHICommandList* CommandList, bool PositionOnly, FrameBuffer* FrameBuffer = nullptr, bool IsCubemap = false);
	void Init();
	void UpdateReflectionParams(glm::vec3 lightPos);
	void UpdateMV(Camera * c);
	void UpdateMV(glm::mat4 View, glm::mat4 Projection);
	SceneConstantBuffer CreateUnformBufferEntry(GameObject * t);
	void UpdateLightBuffer(std::vector<Light*> lights);
	void BindLightsBuffer(RHICommandList * list, int Override = -1);
	void BindMvBuffer(RHICommandList * list);
	void BindMvBuffer(RHICommandList * list, int slot);
	void SetScene(Scene* NewScene);

	void UpdateRelflectionProbes(std::vector<RelfectionProbe*>& probes, RHICommandList * commandlist);

	void RenderCubemap(RelfectionProbe * Map, RHICommandList * commandlist);
	MeshPipelineController* Controller = nullptr;
private:

	RHIBuffer * CLightBuffer[MAX_GPU_DEVICE_COUNT] = { nullptr };
	RHIBuffer* CMVBuffer = nullptr;


	//the View and projection Matix in one place as each gameobject will not have diffrent ones.
	struct MVBuffer MV_Buffer;
	LightBufferW LightsBuffer;


	class Scene* TargetScene = nullptr;
	class Shader_NodeGraph* WorldDefaultMatShader = nullptr;
	//Cube map captures
	MVBuffer CubeMapViews[6];
	float zNear = 0.1f;
	float ZFar = 1000.0f;
	RHIBuffer* RelfectionProbeProjections = nullptr;
};

