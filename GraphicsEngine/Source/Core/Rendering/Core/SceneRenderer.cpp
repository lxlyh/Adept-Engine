#include "Stdafx.h"
#include "SceneRenderer.h"
#include "Rendering/Shaders/Shader_Main.h"
#include "Core/GameObject.h"
#include "Core/Assets/Scene.h"
#include "Rendering/Shaders/Shader_NodeGraph.h"

SceneRenderer::SceneRenderer(Scene* Target)
{
	TargetScene = Target;
	WorldDefaultMatShader = (Shader_NodeGraph*)Material::GetDefaultMaterialShader();
}


SceneRenderer::~SceneRenderer()
{
	EnqueueSafeRHIRelease(CLightBuffer);
	EnqueueSafeRHIRelease(CMVBuffer);
	MemoryUtils::RHIUtil::DeleteRHICArray(GameObjectTransformBuffer, 2);
}

void SceneRenderer::RenderScene(RHICommandList * CommandList, bool PositionOnly, FrameBuffer* FrameBuffer)
{
	if (!PositionOnly)
	{
		BindLightsBuffer(CommandList, MainShaderRSBinds::LightDataCBV);
	}
	BindMvBuffer(CommandList);
	//UpdateMV(MainCamera);
	for (size_t i = 0; i < (*TargetScene->GetMeshObjects()).size(); i++)
	{
		GameObject* CurrentObj = (*TargetScene->GetMeshObjects())[i];
		SetActiveIndex(CommandList, (int)i, CommandList->GetDeviceIndex());
		if (!PositionOnly)
		{
			if (CurrentObj->GetMat()->GetProperties()->ShaderInUse != nullptr)
			{
				CommandList->SetPipelineStateObject(CurrentObj->GetMat()->GetProperties()->ShaderInUse, FrameBuffer);
			}
			else
			{
				CommandList->SetPipelineStateObject(WorldDefaultMatShader, FrameBuffer);
			}
		}
		CurrentObj->Render(PositionOnly, CommandList);
	}
}

void SceneRenderer::Init()
{
	for (int i = 0; i < MaxConstant; i++)
	{
		SceneBuffer.push_back(SceneConstantBuffer());
	}
	for (int i = 0; i < RHI::GetDeviceCount(); i++)//optimize!
	{
		GameObjectTransformBuffer[i] = RHI::CreateRHIBuffer(RHIBuffer::Constant, RHI::GetDeviceContext(i));
		GameObjectTransformBuffer[i]->CreateConstantBuffer(sizeof(SceneConstantBuffer), MaxConstant);
	}
	CLightBuffer = RHI::CreateRHIBuffer(RHIBuffer::Constant);
	CLightBuffer->CreateConstantBuffer(sizeof(LightBufferW), 1, true);
	CMVBuffer = RHI::CreateRHIBuffer(RHIBuffer::Constant);
	CMVBuffer->CreateConstantBuffer(sizeof(MVBuffer), 1, true);
}

void SceneRenderer::UpdateCBV()
{
	for (int DeviceIndex = 0; DeviceIndex < RHI::GetDeviceCount(); DeviceIndex++)//optimize!
	{
		for (int i = 0; i < MaxConstant; i++)
		{
			GameObjectTransformBuffer[DeviceIndex]->UpdateConstantBuffer(&SceneBuffer[i], i);
		}
	}
}

void SceneRenderer::UpdateUnformBufferEntry(const SceneConstantBuffer &bufer, int index)
{
	if (index < MaxConstant)
	{
		SceneBuffer[index] = bufer;
	}
}

void SceneRenderer::SetActiveIndex(RHICommandList* list, int index, int DeviceIndex)
{
	list->SetConstantBufferView(GameObjectTransformBuffer[DeviceIndex], index, MainShaderRSBinds::GODataCBV);
}


void SceneRenderer::UpdateMV(Camera * c)
{
	MV_Buffer.V = c->GetView();
	MV_Buffer.P = c->GetProjection();
	MV_Buffer.CameraPos = c->GetPosition();
	CMVBuffer->UpdateConstantBuffer(&MV_Buffer, 0);
}

void SceneRenderer::UpdateMV(glm::mat4 View, glm::mat4 Projection)
{
	//	ensure(false);
	MV_Buffer.V = View;
	MV_Buffer.P = Projection;
	CMVBuffer->UpdateConstantBuffer(&MV_Buffer, 0);
}

SceneConstantBuffer SceneRenderer::CreateUnformBufferEntry(GameObject * t)
{
	SceneConstantBuffer m_constantBufferData;
	m_constantBufferData.M = t->GetTransform()->GetModel();
	m_constantBufferData.HasNormalMap = false;
	if (t->GetMat() != nullptr)
	{
		m_constantBufferData.HasNormalMap = t->GetMat()->HasNormalMap();
		m_constantBufferData.Metallic = t->GetMat()->GetProperties()->Metallic;
		m_constantBufferData.Roughness = t->GetMat()->GetProperties()->Roughness;
	}
	//used in the prepare stage for this frame!
	return m_constantBufferData;
}

void SceneRenderer::UpdateLightBuffer(std::vector<Light*> lights)
{
	for (int i = 0; i < lights.size(); i++)
	{
		if (i >= MAX_LIGHTS)
		{
			continue;
		}
		LightUniformBuffer newitem;
		newitem.position = lights[i]->GetPosition();
		newitem.color = glm::vec3(lights[i]->GetColor());
		newitem.Direction = lights[i]->GetDirection();
		newitem.type = lights[i]->GetType();
		newitem.HasShadow = lights[i]->GetDoesShadow();
		newitem.ShadowID = lights[i]->GetShadowId();
		if (lights[i]->GetType() == Light::Directional || lights[i]->GetType() == Light::Spot)
		{
			glm::mat4 LightView = glm::lookAtLH<float>(lights[i]->GetPosition(), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));//world up
			glm::vec3 position = glm::vec3(0, 20, 50);
			//position = lights[i]->GetPosition();
			LightView = glm::lookAtLH<float>(position, position + newitem.Direction, glm::vec3(0, 0, 1));//world up
			float size = 100.0f;
			glm::mat4 proj;
			if (lights[i]->GetType() == Light::Spot)
			{
				proj = glm::perspective<float>(glm::radians(45.0f), 1.0f, 2.0f, 50.0f);
				LightView = glm::lookAtLH<float>(lights[i]->GetPosition(), lights[i]->GetPosition() + newitem.Direction, glm::vec3(0, 0, 1));//world up
			}
			else
			{
				proj = glm::orthoLH<float>(-size, size, -size, size, -200, 100);
			}
			lights[i]->Projection = proj;
			lights[i]->DirView = LightView;
			newitem.LightVP = proj * LightView;
		}
		if (lights[i]->GetType() == Light::Point)
		{
			float znear = 1.0f;
			float zfar = 500;
			glm::mat4 proj = glm::perspectiveLH<float>(glm::radians(90.0f), 1.0f, znear, zfar);
			lights[i]->Projection = proj;
		}
		LightsBuffer.Light[i] = newitem;
	}
	CLightBuffer->UpdateConstantBuffer(&LightsBuffer, 0);
}

void SceneRenderer::BindLightsBuffer(RHICommandList*  list, int Override)
{
	list->SetConstantBufferView(CLightBuffer, 0, Override);
}

void SceneRenderer::BindMvBuffer(RHICommandList * list)
{
	list->SetConstantBufferView(CMVBuffer, 0, MainShaderRSBinds::MVCBV);
}

void SceneRenderer::BindMvBuffer(RHICommandList * list, int slot)
{
	list->SetConstantBufferView(CMVBuffer, 0, slot);
}

void SceneRenderer::SetScene(Scene * NewScene)
{
	TargetScene = NewScene;
}
void SceneRenderer::ClearBuffer()
{
	SceneBuffer.empty();
}