#pragma once
#include "Component.h"
#include "Physics/RigidBody.h"
class RigidbodyComponent :
	public Component
{
public:
	RigidbodyComponent();
	~RigidbodyComponent();

	// Inherited via Component
	void BeginPlay() override;
	void Update(float delta) override;
	void FixedUpdate(float delta) override;
	virtual void InitComponent() override;
private:
	RigidBody* actor;

	// Inherited via Component


	// Inherited via Component
	virtual void Serialise(rapidjson::Value & v) override;

	virtual void Deserialise(rapidjson::Value & v) override;

};
