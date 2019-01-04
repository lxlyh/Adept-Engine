#include "Transform.h"
#include "Core/Assets/Archive.h"
#include "Utils/MathUtils.h"
#include "GameObject.h"

Transform::Transform(const glm::vec3 & pos, const glm::vec3 & rot, const glm::vec3 & scale) :
	_pos(pos),
	_qrot(glm::quat(rot)),
	_scale(scale)
{
	GetModel();
	parentMatrix = glm::mat4(1);//Identity
}
glm::vec3 Transform::GetForward()
{
	return TransformDirection(glm::vec3(0, 0, 1), GetModel());
}

glm::vec3 Transform::GetUp()
{
	return TransformDirection(glm::vec3(0, 1, 0), GetModel());
}

glm::vec3 Transform::GetRight()
{
	return TransformDirection(glm::vec3(1, 0, 0), GetModel());
}

glm::vec3 Transform::TransfromToLocalSpace(glm::vec3 & direction)
{
	return TransformDirection(direction, GetModel());
}

glm::vec3 Transform::TransformDirection(const  glm::vec3 & pDirection, const  glm::mat4 & pMatrix)
{
	return /*glm::normalize*/(glm::vec3(pMatrix * glm::vec4(pDirection, 0.0f)));
}

glm::quat Transform::GetQuatRot()
{
	GetModel();
	return _qrot;
}

Transform * Transform::GetParent() const
{
	if (parent.IsValid())
	{
		return parent->GetTransform();
	}
	return nullptr;
}

bool Transform::IsChanged()
{
	if (oldpos != _pos)
		return true;

	if (oldqrot != _qrot)
		return true;

	if (oldscale != _scale)
		return true;

	return false;
}
void Transform::Update()
{
	oldpos = _pos;
	oldqrot = _qrot;
	oldscale = _scale;
}

void Transform::Serilise(Archive * A)
{
	ArchiveProp(_pos);
	//ArchiveProp(_qrot);
	ArchiveProp(_scale);
}

glm::mat4 Transform::GetModel(bool NoParent)
{
#if USE_TRANSFORM_CACHING
	if (!UpdateModel && parent == nullptr)
	{
		return CacheModel;
	}
	if (parent != nullptr && !parent->IsChanged() && !UpdateModel)
	{
		return CacheModel;
	}
#endif
	glm::mat4 posMat = glm::translate(_pos);
	glm::mat4 scaleMat = glm::scale(_scale);
	glm::mat4 rotMat = glm::toMat4(_qrot);
	if (parent.IsValid()
#if USE_TRANSFORM_CACHING		
		&& parent->IsChanged()
#endif
		&& !NoParent)
	{
		parentMatrix = parent->GetTransform()->GetModel();
	}
	else
	{
		parentMatrix = glm::mat4(1);
	}
	CacheModel = parentMatrix * (posMat * rotMat * scaleMat);
	_rot = glm::eulerAngles(_qrot);

	UpdateModel = false;
	return CacheModel;
}

glm::mat4 Transform::GetMVP(Camera * camera)
{
	glm::mat4 VP = camera->GetViewProjection();
	glm::mat4 M = GetModel();
	return VP * M;
}

glm::mat4 Transform::GetTransFormView()
{
	return glm::lookAt(GetPos(), GetPos() + GetForward(), GetUp());
}

///------------------------------------------------------
//setters	

void Transform::SetPos(const glm::vec3 & pos)
{
	UpdateModel = true;
	oldpos = _pos;
	_pos = pos;
	GetModel();
	CheckNAN(_pos);
}

void Transform::SetEulerRot(const glm::vec3 & rot)
{
	UpdateModel = true;
	oldqrot = this->_qrot;
	glm::quat QuatAroundX = glm::angleAxis(glm::radians(rot.x), glm::vec3(1.0, 0.0, 0.0));
	glm::quat QuatAroundY = glm::angleAxis(glm::radians(rot.y), glm::vec3(0.0, 1.0, 0.0));
	glm::quat QuatAroundZ = glm::angleAxis(glm::radians(rot.z), glm::vec3(0.0, 0.0, 1.0));
	glm::quat finalOrientation = QuatAroundX * QuatAroundY * QuatAroundZ;
	this->_qrot = finalOrientation;
	CheckNAN(finalOrientation);
	CheckNAN(_qrot);
	//this->_qrot = glm::quat(glm::radians(rot));
}

void Transform::SetScale(const glm::vec3 & scale)
{
	UpdateModel = true;
	this->oldscale = this->_scale;
	this->_scale = scale;
	CheckNAN(_scale);
}

void Transform::AddRotation(glm::vec3 & rot)
{
	UpdateModel = true;
	this->_rot += rot;
}

void Transform::RotateAboutAxis(glm::vec3 & axis, float amt)
{
	UpdateModel = true;
	oldqrot = this->_qrot;
	this->_qrot *= glm::angleAxis(glm::radians(amt), axis);
	GetModel();
	CheckNAN(_qrot);
}

void Transform::SetQrot(const glm::quat & val)
{
	UpdateModel = true;
	_qrot = val;
	CheckNAN(_qrot);
}

void Transform::SetParent(GameObject* Parent)
{
	UpdateModel = true;
	parent = Parent;
	if (Parent != nullptr)
	{
		Parent->GetTransform()->UpdateModel = true;
	}
}

void Transform::TranslatePos(const glm::vec3 & pos)
{
	UpdateModel = true;
	_pos += pos;
	CheckNAN(_pos);
}

void Transform::MakeRotationFromXY(const glm::vec3 & Fwd, const glm::vec3 & up)
{
	//_qrot = glm::)
}

void Transform::SetLocalPosition(glm::vec3 localpos)
{
	if (parent.IsValid())
	{
		glm::mat4 LocalMAtrix = parent->GetTransform()->GetModel();
		glm::vec4 newpos = glm::vec4(localpos, 1.0f)*glm::inverse(LocalMAtrix);
		newpos = newpos * LocalMAtrix;
		CheckNAN(newpos);
		SetPos(newpos);
	}
	else
	{
		CheckNAN(localpos);
		SetPos(localpos);
	}
}

glm::vec3 Transform::GetPos() const
{
	return glm::vec3(CacheModel[3][0], CacheModel[3][1], CacheModel[3][2]);
}

glm::vec3 Transform::GetScale() const
{
	return glm::vec3(CacheModel[0][0], CacheModel[1][1], CacheModel[2][2]);
}

void Transform::SetLocalRotation(glm::quat localrot)
{
	if (parent.IsValid())
	{
		glm::mat4 LocalMAtrix = parent->GetTransform()->GetModel();
		glm::quat rotation = glm::quat(glm::toMat4(localrot)*glm::inverse(LocalMAtrix));
		rotation = glm::toMat4(rotation) *LocalMAtrix;
		CheckNAN(rotation);
		SetQrot(GetQuatRot()* rotation);
	}
	else
	{
		SetQrot(localrot);//todo: what todo?
	}
}

glm::vec3 Transform::GetEulerRot() const
{
	return glm::degrees(_rot);
}


