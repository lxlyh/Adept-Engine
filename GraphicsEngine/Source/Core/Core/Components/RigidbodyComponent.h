#pragma once
#include "Component.h"
#include "Physics/Physics_fwd.h"
class RigidbodyComponent :
	public Component
{
public:
	CORE_API RigidbodyComponent();
	~RigidbodyComponent();

	// Inherited via Component
	void BeginPlay() override;
	void Update(float delta) override;
	void FixedUpdate(float delta) override;
	virtual void SceneInitComponent() override;

private:
	RigidBody* actor;

	// Inherited via Component
	void GetInspectorProps(std::vector<Inspector::InspectorProperyGroup>& props) override;
	void ProcessSerialArchive(Archive * A) override;

	float mass = 1.0f;

	// Inherited via Component
	virtual void InitComponent()override {} ;

};

