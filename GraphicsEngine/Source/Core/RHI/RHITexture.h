#pragma once
#include "Core/Platform/PlatformCore.h"
#include "RHITypes.h"

class DeviceContext;
//Represents a GPU resource 
struct RHITextureDesc2
{
	uint32 Width = 0;
	uint32 Height = 0;
	uint32 Depth = 1;
	int MipCount = 1;
	bool IsRenderTarget = false;
	bool IsDepthStencil = false;
	bool AllowUnorderedAccess = false;
	bool AllowCrossGPU = false;
	eTextureDimension Dimension = eTextureDimension::DIMENSION_TEXTURE2D;
	eTEXTURE_FORMAT Format = eTEXTURE_FORMAT::FORMAT_UNKNOWN;

	glm::vec4 clearcolour = glm::vec4(0.0f, 0.2f, 0.4f, 1.0f);
	float DepthClearValue = 1.0f;
	std::string Name = "";

	EResourceState::Type InitalState = EResourceState::Common;
};
class RHITexture : public IRHIResourse
{
public:
	RHI_API RHITexture();
	RHI_API virtual ~RHITexture();
	RHI_API const RHITextureDesc2& GetDescription()const;
	RHI_API virtual void Create(const RHITextureDesc2& Desc, DeviceContext* Context = nullptr);

	RHI_API virtual void CreateWithUpload(const TextureDescription & idesc, DeviceContext * iContext);

	RHI_API virtual void CopyToStagingResource(RHIInterGPUStagingResource* Res, RHICommandList* List);
	RHI_API virtual void CopyFromStagingResource(RHIInterGPUStagingResource* Res, RHICommandList* list);
protected:
	RHITextureDesc2 Desc;
	TextureDescription	UploadDesc;
	DeviceContext* Context = nullptr;
};

