
#include "BehaviourTreeNode.h"
#include "AI/Core/AISystem.h"
#include "BehaviourTree.h"
#include "Core/GameObject.h"
#include "BTBlackboard.h"
#include "../Services/ServiceBase.h"
#include "BaseDecorator.h"
#include "Core/Performance/PerfManager.h"
#include "../AIController.h"


EBTNodeReturn::Type BehaviourTreeNode::ExecuteNode()
{
	return EBTNodeReturn::Success;
}

EBTNodeReturn::Type BehaviourTreeNode::HandleExecuteNode()
{
	EBTNodeReturn::Type returnvalue = EBTNodeReturn::LIMIT;
	if (RunningChild == nullptr)
	{
		returnvalue = ExecuteNode();
		if (returnvalue == EBTNodeReturn::Failure)
		{
			return returnvalue;
		}
	}
	else
	{
		returnvalue = RunningChild->HandleExecuteNode();
	}
	if (ExecuteChilds)
	{
		//Children should be sorted for priority
		for (int i = 0; i < Children.size(); i++)
		{
			if (RunningChild != Children[i] && RunningChild != nullptr)
			{
				continue;
			}
			returnvalue = Children[i]->HandleExecuteNode();
			if (returnvalue == EBTNodeReturn::Failure)
			{
				return returnvalue;
			}
			if (returnvalue == EBTNodeReturn::Running)
			{
				RunningChild = Children[i];
				return returnvalue;
			}
			RunningChild = nullptr;
		}
	}
	return returnvalue;
}

BTMoveToNode::BTMoveToNode(BTValue * GoalPos)
{
	BBValues.push_back(GoalPos);
}

EBTNodeReturn::Type BTMoveToNode::ExecuteNode()
{
	NavigationPath* path = nullptr;
	if (ParentTree->Target == nullptr)
	{
		return EBTNodeReturn::Failure;
	}
	AIController* controller = 	ParentTree->Target->GetComponent<AIController>();
	controller->MoveTo(BBValues[0]->GetValue<GameObject>());
	return  (/*hr == ENavRequestStatus::Complete*/1) ? EBTNodeReturn::Success : EBTNodeReturn::Failure;
}

EBTNodeReturn::Type BTSelectorNode::HandleExecuteNode()
{
	if (RunningChild == nullptr)
	{
		for (int i = 0; i < Services.size(); i++)
		{
			Services[i]->HandleTick();//If a child is running a service can't tick 
		}
		bool Checks = false;
		for (int i = 0; i < Decorators.size(); i++)
		{
			Checks = Decorators[i]->RunCheck();
			if (!Checks)
			{
				return EBTNodeReturn::Failure;
			}
		}
	}
	//the checks have passed execute lower
	EBTNodeReturn::Type returnvalue = EBTNodeReturn::Success;
	for (int i = 0; i < Children.size(); i++)
	{
		if (RunningChild != Children[i] && RunningChild != nullptr)
		{
			continue;
		}
		returnvalue = Children[i]->HandleExecuteNode();
		if (returnvalue == EBTNodeReturn::Running)
		{
			RunningChild = Children[i];
			return returnvalue;
		}
		RunningChild = nullptr;
		if (returnvalue == EBTNodeReturn::Failure && ContinueUntilFail)
		{
			return returnvalue;
		}
		else if (returnvalue == EBTNodeReturn::Success && !ContinueUntilFail)
		{
			return returnvalue;
		}
	}
	return returnvalue;

}

EBTNodeReturn::Type BTSelectorNode::ExecuteNode()
{
	return EBTNodeReturn::Success;
}

EBTNodeReturn::Type BTWaitNode::ExecuteNode()
{
	if (Remaining > 0.0f)
	{
		Remaining -= PerfManager::GetDeltaTime();
		return EBTNodeReturn::Running;
	}
	return EBTNodeReturn::Success;
}