#pragma once
#if !BUILD_SHIPPING
#include "AI/Core/Behaviour/BehaviourTree.h"
class TestBTTree : public BehaviourTree
{
public:
	TestBTTree();
	virtual ~TestBTTree();

	virtual void SetupTree() override;

};
#endif
