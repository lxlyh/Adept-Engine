#include "Stdafx.h"
#include "RenderGraphDrawer.h"
#include "RenderGraph.h"
#include "RenderNode.h"
#include "..\Core\DebugLineDrawer.h"
#include "..\Renderers\TextRenderer.h"
#include "NodeLink.h"
#include "Core\Input\Input.h"


RenderGraphDrawer::RenderGraphDrawer()
{}


RenderGraphDrawer::~RenderGraphDrawer()
{}

void RenderGraphDrawer::Draw(RenderGraph* G)
{
	return;
	CurrentGraph = G;
	drawer = DebugLineDrawer::Get2();
	RenderNode* itor = G->GetNodeAtIndex(0);
	glm::vec3 tPos = StartPos;
	nodesize = glm::vec3(200, -200, 0);
	int index = 0;
	while (itor->GetNextNode() != nullptr)
	{
		DrawNode(itor, tPos, index);
		DrawLinks(itor, index, tPos);
		index++;
		tPos += glm::vec3(nodesize.x + NodeSpaceing, 0, 0);
		itor = itor->GetNextNode();
	}
}
void RenderGraphDrawer::Update()
{
	const float movesize = 10;
	if (Input::GetKey('i'))
	{
		StartPos.x -= movesize;
	}
	if (Input::GetKey('o'))
	{
		StartPos.x += movesize;
	}
}
void RenderGraphDrawer::DrawNode(RenderNode * Node, glm::vec3 & LastPos, int index)
{
	glm::vec3 BoxSize = nodesize;
	glm::vec3 BoxPos = LastPos + glm::vec3(nodesize.x, 0, 0);

	BoxPos.y += 100 * (index % 3);

	drawer->AddLine(BoxPos, BoxPos + glm::vec3(BoxSize.x, 0, 0), Colour);
	drawer->AddLine(BoxPos + glm::vec3(BoxSize.x, 0, 0), BoxPos + glm::vec3(BoxSize.x, BoxSize.y, 0), Colour);
	drawer->AddLine(BoxPos + glm::vec3(BoxSize.x, BoxSize.y, 0), BoxPos + glm::vec3(0, BoxSize.y, 0), Colour);
	drawer->AddLine(BoxPos + glm::vec3(0, BoxSize.y, 0), BoxPos, Colour);
	drawer->AddLine(BoxPos + glm::vec3(0, -20, 0), BoxPos + glm::vec3(BoxSize.x, -20, 0), Colour);
	drawer->AddLine(BoxPos + glm::vec3(BoxSize.x / 2, -20, 0), BoxPos + glm::vec3(BoxSize.x / 2, BoxSize.y, 0), Colour);
	TextRenderer::instance->RenderText(Node->GetName(), BoxPos.x, BoxPos.y - 10.0f, 0.3f, Colour);
	for (uint i = 0; i < Node->GetNumInputs(); i++)
	{
		NodeLink* Link = Node->GetInput(i);
		TextRenderer::instance->RenderText(Link->GetLinkName(), BoxPos.x, BoxPos.y - 40 - (InputSpaceing*i), 0.3, Colour);
	}
	for (uint i = 0; i < Node->GetNumOutputs(); i++)
	{
		NodeLink* Link = Node->GetOutput(i);
		TextRenderer::instance->RenderText(Link->GetLinkName(), BoxPos.x + nodesize.x / 2, BoxPos.y - 40 - (InputSpaceing*i), 0.3, Colour);
	}
}

void RenderGraphDrawer::DrawLinks(RenderNode * A, int index, glm::vec3 & LastPos)
{
	glm::vec3 BoxPos = LastPos + glm::vec3(nodesize.x, 0, 0);

	for (uint i = 0; i < A->GetNumInputs(); i++)
	{
		NodeLink* Link = A->GetInput(i);
		if (Link->StoreLink != nullptr)
		{
			if (Link->StoreLink->OwnerNode != nullptr)
			{
				glm::vec3 InputPos = GetPosOfNodeindex(index);
				InputPos.y = InputPos.y - 40 - (InputSpaceing*i) + 5;
				int oindex = CurrentGraph->GetIndexOfNode(Link->StoreLink->OwnerNode);
				glm::vec3 othernode = GetPosOfNodeindex(oindex);

				for (int x = 0; x < Link->StoreLink->OwnerNode->GetNumOutputs(); x++)
				{
					NodeLink* OtherLink = Link->StoreLink->OwnerNode->GetOutput(x);
					if (OtherLink == Link->StoreLink)
					{
						othernode.y -= 40 - InputSpaceing * x - 5;
						othernode.x += nodesize.x;
						break;
					}
				}
				drawer->AddLine(InputPos, othernode, Colour);
			}
		}
	}
}

glm::vec3 RenderGraphDrawer::GetPosOfNodeindex(int index)
{
	glm::vec3 othernode = StartPos + glm::vec3(nodesize.x, 0, 0) + (glm::vec3(nodesize.x + NodeSpaceing, 0, 0)*(index));
	othernode.y += 100 * (index % 3);
	return othernode;
}