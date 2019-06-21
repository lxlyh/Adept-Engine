#pragma once
#include "../RenderNode.h"
class OutputToScreenNode : public RenderNode
{
public:
	OutputToScreenNode();
	~OutputToScreenNode();

	virtual void OnExecute() override;
protected:
	virtual void OnNodeSettingChange() override;
	RHICommandList* ScreenWriteList = nullptr;

	virtual void OnSetupNode() override;

};

