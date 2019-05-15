#include "Stdafx.h"
#include "Defaults.h"
#include "Core\Assets\AssetManager.h"
#include "Core\Platform\PlatformCore.h"
#include "Core\IRefCount.h"
#include "Core\Assets\Asset_Shader.h"

Defaults* Defaults::Instance = nullptr;

Defaults::Defaults()
{
	DefaultTexture = AssetManager::DirectLoadTextureAsset("\\texture\\T_GridSmall_01_D.png");
	ensureFatalMsgf(DefaultTexture != nullptr, "Failed to Load Fallback Texture");

	DefaultMateral = new Material(new Asset_Shader(true));
	DefaultMateral->Init();
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
}

Defaults::~Defaults()
{
	//	SafeDelete(DefaultShaderMat);
}

void Defaults::Start()
{
	Instance = new Defaults();
}

void Defaults::Shutdown()
{
	SafeDelete(Instance);
}

BaseTextureRef Defaults::GetDefaultTexture()
{
	return Instance->DefaultTexture;
}

Material * Defaults::GetDefaultMaterial()
{
	return Instance->DefaultMateral;
}


RHIBuffer * Defaults::GetQuadBuffer()
{
	return Instance->VertexBuffer;
}


