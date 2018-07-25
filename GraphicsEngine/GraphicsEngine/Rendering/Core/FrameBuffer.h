#pragma once
#include "EngineGlobals.h"
#include "RHI/RHITypes.h"
class FrameBuffer
{
public:	
	FrameBuffer(class DeviceContext* device, RHIFrameBufferDesc& Desc);
	virtual ~FrameBuffer();
	
	int GetWidth() const { return m_width; }
	int GetHeight() const { return m_height; }
	virtual class DeviceContext* GetDevice() = 0;	
	RHIFrameBufferDesc& GetDescription();
	virtual void Resize(int width, int height);

	int m_width = 0;
	int m_height = 0;
	virtual void SetupCopyToDevice(DeviceContext* device) {};
	static void CopyHelper(FrameBuffer* Target, DeviceContext* TargetDevice);
protected:	
	
	RHIFrameBufferDesc BufferDesc;
	
};

