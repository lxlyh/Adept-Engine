#include "BufferStorageNode.h"
#include "FrameBufferStorageNode.h"
#include "Core/Maths/Math.h"
#include "RHI/SFRController.h"
#include "../../Core/Screen.h"

BufferStorageNode::BufferStorageNode()
{
	StoreType = EStorageType::Buffer;
	Desc.StartState = EResourceState::Common;
}

BufferStorageNode::~BufferStorageNode()
{
}

void BufferStorageNode::Update()
{

}

void BufferStorageNode::Resize()
{
	EnqueueSafeRHIRelease(GPUBuffer);
	Create();
}

void BufferStorageNode::Create()
{
	if (FramebufferNode != nullptr)
	{
		int Size = FramebufferNode->GetFrameBufferDesc().Width * FramebufferNode->GetFrameBufferDesc().Height;
		if (RHI::GetRenderSettings()->GetCurrnetSFRSettings().Enabled)
		{
			RHIScissorRect r = SFRController::GetScissor(1, Screen::GetScaledRes());
			int Width = abs(r.Right - r.Left);
			Size = Width * r.Bottom;
		}
		int compCount = RHIUtils::GetComponentCount(FramebufferNode->GetFrameBufferDesc().RTFormats[0]);
		compCount = 3;
		Desc.ElementCount = Math::Max(1, (Size * compCount) * LinkedFrameBufferRatio);
		Desc.Stride = sizeof(float) / 2;
	}
	Desc.AllowUnorderedAccess = true;
	Desc.Accesstype = EBufferAccessType::GPUOnly;
	GPUBuffer = RHI::CreateRHIBuffer(ERHIBufferType::GPU, DeviceObject);
	GPUBuffer->CreateBuffer(Desc);
}