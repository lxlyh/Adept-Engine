#include "stdafx.h"
#include "DebugLineDrawer.h"
#include "RHI/RHI.h"
#include "UI\UIManager.h"
#include "Editor/EditorWindow.h"
#include "Rendering/Shaders/Shader_Line.h"
#include "RHI/RHICommandList.h"
#include "Core/Platform/PlatformCore.h"
#include "Core/Performance/PerfManager.h"
#include "Core/Assets/ShaderComplier.h"

DebugLineDrawer* DebugLineDrawer::instance = nullptr;
DebugLineDrawer::DebugLineDrawer(bool DOnly)
{
	Is2DOnly = DOnly;
	LineShader = ShaderComplier::GetShader_Default<Shader_Line, bool>(Is2DOnly);
	ensure(LineShader);
	DataBuffer = RHI::CreateRHIBuffer(ERHIBufferType::Constant);
	DataBuffer->CreateConstantBuffer(sizeof(glm::mat4x4), 1);
	//VertexBuffer = RHI::CreateRHIBuffer(ERHIBufferType::Vertex);

	//VertexBuffer->CreateVertexBuffer(sizeof(VERTEX), sizeof(VERTEX)*maxSize, EBufferAccessType::Dynamic);
	ReallocBuffer(CurrentMaxVerts);
	CmdList = RHI::CreateCommandList();
	PipeLineState state = {};
	state.DepthTest = false;
	state.RasterMode = PRIMITIVE_TOPOLOGY_TYPE::PRIMITIVE_TOPOLOGY_TYPE_LINE;
	CmdList->SetPipelineState(state);
	CmdList->CreatePipelineState(LineShader);
	instance = this;
}


DebugLineDrawer::~DebugLineDrawer()
{
	EnqueueSafeRHIRelease(CmdList);
	EnqueueSafeRHIRelease(VertexBuffer);
	EnqueueSafeRHIRelease(DataBuffer);
}

void DebugLineDrawer::GenerateLines()
{
	if (Lines.size() == 0)
	{
		return;
	}
	std::vector<VERTEX> Verts;
	for (int i = 0; i < Lines.size(); i++)
	{
		VERTEX AVert = {};
		AVert.pos = Lines[i].startpos;
		AVert.colour = Lines[i].colour;
		Verts.push_back(AVert);

		VERTEX BVert = {};
		BVert.pos = Lines[i].endpos;
		BVert.colour = Lines[i].colour;
		Verts.push_back(BVert);
	}
	VertsOnGPU = Verts.size();

	VertexBuffer->UpdateVertexBuffer(Verts.data(), sizeof(VERTEX) * Verts.size());
}

void DebugLineDrawer::RenderLines()
{
	RenderLines(Projection);
}
void DebugLineDrawer::ReallocBuffer(int NewSize)
{
	ensure(NewSize < maxSize);
	EnqueueSafeRHIRelease(VertexBuffer);
	CurrentMaxVerts = NewSize;
	VertexBuffer = RHI::CreateRHIBuffer(ERHIBufferType::Vertex);
	const int vertexBufferSize = sizeof(VERTEX) * CurrentMaxVerts;
	VertexBuffer->CreateVertexBuffer(sizeof(VERTEX), vertexBufferSize, EBufferAccessType::Dynamic);
}
void DebugLineDrawer::RenderLines(glm::mat4& matrix)
{
	if (VertsOnGPU == 0)
	{
		return;
	}
	CmdList->ResetList();
	CmdList->SetScreenBackBufferAsRT();
	CmdList->SetVertexBuffer(VertexBuffer);
	if (!Is2DOnly)
	{
		DataBuffer->UpdateConstantBuffer(glm::value_ptr(matrix), 0);
#if WITH_EDITOR
		if (EditorWindow::GetInstance()->UseSmallerViewPort())
		{
			IntRect rect = EditorWindow::GetInstance()->GetViewPortRect();
			CmdList->SetViewport(rect.Min.x, rect.Min.y, rect.Max.x, rect.Max.y, 0, 0);
		}
#endif
	}
	CmdList->SetConstantBufferView(DataBuffer, 0, 0);
	CmdList->DrawPrimitive((int)VertsOnGPU, 1, 0, 0);
	CmdList->Execute();
	ClearLines();//bin all lines rendered this frame.
}

void DebugLineDrawer::ClearLines()
{
	for (int i = 0; i < Lines.size(); i++)
	{
		if (Lines[i].Time > 0)
		{
			Lines[i].Time -= PerfManager::GetDeltaTime();
		}
		else
		{
			Lines.erase(Lines.begin() + i);
		}
	}
}

void DebugLineDrawer::AddLine(glm::vec3 Start, glm::vec3 end, glm::vec3 colour, float time)
{
	if (Lines.size() * 2 >= CurrentMaxVerts)
	{
		ReallocBuffer((int)Lines.size() * 2 + 10);
	}
	WLine l = {};
	l.startpos = Start;
	l.endpos = end;
	l.colour = colour;
	l.Time = time;
	Lines.push_back(l);
}

void DebugLineDrawer::OnResize(int newwidth, int newheight)
{
	if (Is2DOnly)
	{
		Projection = glm::ortho(0.0f, static_cast<float>(newwidth), 0.0f, static_cast<float>(newheight));
		DataBuffer->UpdateConstantBuffer(glm::value_ptr(Projection), 0);
	}
}
