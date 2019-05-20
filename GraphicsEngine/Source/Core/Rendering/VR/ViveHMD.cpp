#include "Stdafx.h"
#include "ViveHMD.h"
#include "OpenVR/headers/openvr.h"
#include "VRCamera.h"
#include "Core/Utils/DebugDrawers.h"

ViveHMD::ViveHMD()
{}


ViveHMD::~ViveHMD()
{
	vr::VR_Shutdown();
}

bool ViveHMD::CanCreate()
{
	return vr::VR_IsHmdPresent();
}

void ViveHMD::Init()
{
	HMD::Init();
	vr::HmdError error;
	system = vr::VR_Init(&error, vr::VRApplication_Scene);
	ensure(error == vr::HmdError::VRInitError_None);
	ensure(vr::VRCompositor());
}
glm::mat4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose)
{
	glm::mat4 matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], matPose.m[3][0],
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
	);
	matrixObj = glm::rowMajor4(matrixObj);
	return matrixObj;
}
glm::mat4 ConvertSteamVRMatrixToMatrix4_T(const vr::HmdMatrix34_t &matPose)
{
	glm::mat4 matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
	);
	matrixObj[0][2] = -matrixObj[0][2];
	matrixObj[1][2] = -matrixObj[1][2];

	matrixObj[2][0] = -matrixObj[2][0];
	matrixObj[2][1] = -matrixObj[2][1];
	matrixObj[2][3] = -matrixObj[2][3];

	//matrixObj = glm::rowMajor4(matrixObj);
	return matrixObj;
}
glm::mat4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix44_t &mat)
{
	glm::mat4 matrixObj(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
	);
	matrixObj = glm::rowMajor4(matrixObj);
	return matrixObj;
}


static float _copysign(float sizeval, float signval)
{
	return glm::sign(signval) == 1 ? glm::abs(sizeval) : -glm::abs(sizeval);
}

static glm::quat GetRotation(glm::mat4 matrix)
{
	glm::quat q;// = new Quaternion();
	q.w = glm::sqrt(glm::max(0.0f, 1.0f + matrix[0][0] + matrix[1][1] + matrix[2][2])) / 2.0f;
	q.x = glm::sqrt(glm::max(0.0f, 1.0f + matrix[0][0] - matrix[1][1] - matrix[2][2])) / 2.0f;
	q.y = glm::sqrt(glm::max(0.0f, 1.0f - matrix[0][0] + matrix[1][1] - matrix[2][2])) / 2.0f;
	q.z = glm::sqrt(glm::max(0.0f, 1.0f - matrix[0][0] - matrix[1][1] + matrix[2][2])) / 2.0f;
	q.x = _copysign(q.x, matrix[2][1] - matrix[1][2]);
	q.y = _copysign(q.y, matrix[0][2] - matrix[2][0]);
	q.z = _copysign(q.z, matrix[1][0] - matrix[0][1]);
	return q;
}
glm::vec3 Getpos(glm::mat4 matMVP)
{
	return glm::vec3(matMVP[0][3], matMVP[1][3], -matMVP[2][3]);
}
void RenderTransfrom(Transform t)
{
	glm::vec3 pos = t.GetPos()*10;
	DebugDrawers::DrawDebugLine(pos, pos + t.GetUp(), glm::vec3(1, 0, 0));
	DebugDrawers::DrawDebugLine(pos, pos + t.GetRight(), glm::vec3(0, 1, 0));
	DebugDrawers::DrawDebugLine(pos, pos + t.GetForward(), glm::vec3(0, 0, 1));
}
void ViveHMD::Update()
{

	//	system->PollNextEventWithPose()
	vr::VREvent_t event;
	while (system->PollNextEvent(&event, sizeof(event)))
	{
		//ProcessVREvent(event);
	}
	for (vr::TrackedDeviceIndex_t unDevice = 0; unDevice < vr::k_unMaxTrackedDeviceCount; unDevice++)
	{
		vr::VRControllerState_t state;
		if (system->GetControllerState(unDevice, &state, sizeof(state)))
		{
			float t = 0;
			//m_rbShowTrackedDevice[unDevice] = state.ulButtonPressed == 0;
		}
	}
	vr::VRCompositor()->SetTrackingSpace(vr::ETrackingUniverseOrigin::TrackingUniverseSeated);
	vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);
	for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
	{
		if (m_rTrackedDevicePose[nDevice].bPoseIsValid)
		{
			poses[nDevice] = ConvertSteamVRMatrixToMatrix4_T(m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking);
			if (system->GetTrackedDeviceClass(nDevice) == vr::TrackedDeviceClass_HMD)
			{
				//	poses[nDevice] = glm::inverse(poses[nDevice]);
				glm::vec3 pos = glm::vec3(poses[nDevice][0][3], poses[nDevice][1][3], poses[nDevice][2][3]);
				//glm::vec3(m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[0][3], m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[1][3], m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking.m[2][3]);
				//DebugDrawers::DrawDebugSphere(pos * 10, 2, glm::vec3(1));
				posm = ConvertSteamVRMatrixToMatrix4(m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking);
			}
			else
			{
				TransArray[nDevice].SetPos(Getpos(ConvertSteamVRMatrixToMatrix4(m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking)));
				TransArray[nDevice].SetQrot(GetRotation(glm::inverse(poses[nDevice])));
				
				RenderTransfrom(TransArray[nDevice]);
			}
		}
	}
	if (m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
	{
		HMDPose = glm::inverse(poses[vr::k_unTrackedDeviceIndex_Hmd]);
		
		Rot = GetRotation(HMDPose);
		//HMDPose = 
	}
	glm::mat4 matMVP;
	for (uint32_t unTrackedDevice = 0; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; unTrackedDevice++)
	{
		const vr::TrackedDevicePose_t & pose = m_rTrackedDevicePose[unTrackedDevice];
		if (!pose.bPoseIsValid)
			continue;

		if (system->GetTrackedDeviceClass(unTrackedDevice) == vr::TrackedDeviceClass_Controller)
			continue;

		const glm::mat4 & matDeviceToTracking = poses[unTrackedDevice];
		matMVP = HMDPose /** matDeviceToTracking*/;

	}
	glm::mat4 mToggle_YZ = glm::mat4(1, 0, 0, 0,
		0, 0, 1, 0,
		0, 1, 0, 0,
		0, 0, 0, 1
	);
	//CameraInstance->GetTransfrom()->Set(glm::inverse(matMVP));
	CameraInstance->GetTransfrom()->SetQrot(Rot);
	matMVP = posm;
	glm::vec3 offset = glm::vec3(0, 5, 0);
	CameraInstance->GetTransfrom()->SetPos(glm::vec3(matMVP[0][3], matMVP[1][3], -matMVP[2][3])+offset);

	//CameraInstance->GetTransfrom()->Set(matMVP);
#if 1
	glm::vec3 pos = CameraInstance->GetTransfrom()->GetPos() * 10;
	DebugDrawers::DrawDebugLine(pos, pos + CameraInstance->GetTransfrom()->GetUp(), glm::vec3(1, 0, 0));
	DebugDrawers::DrawDebugLine(pos, pos + CameraInstance->GetTransfrom()->GetRight(), glm::vec3(0, 1, 0));
	DebugDrawers::DrawDebugLine(pos, pos + CameraInstance->GetTransfrom()->GetForward(), glm::vec3(0, 0, 1));
#endif

	HMD::Update();
#if 0
	glm::mat4 TRANS = matMVP * glm::inverse(ConvertSteamVRMatrixToMatrix4(system->GetEyeToHeadTransform(vr::EVREye::Eye_Left)));
	CameraInstance->GetEyeCam(EEye::Left)->SetProjection(ConvertSteamVRMatrixToMatrix4(system->GetProjectionMatrix(vr::Eye_Left, 0.1, 1000)));
	CameraInstance->GetEyeCam(EEye::Left)->SetViewTransFrom(TRANS);
	TRANS = matMVP * glm::inverse(ConvertSteamVRMatrixToMatrix4(system->GetEyeToHeadTransform(vr::Eye_Right)));
	CameraInstance->GetEyeCam(EEye::Right)->SetProjection(ConvertSteamVRMatrixToMatrix4(system->GetProjectionMatrix(vr::Eye_Right, 0.1, 1000)));
	CameraInstance->GetEyeCam(EEye::Right)->SetViewTransFrom(TRANS);
#else
	CameraInstance->UpdateDebugTracking();
#endif

}

void ViveHMD::OutputToEye(FrameBuffer* buffer, EEye::Type eye)
{
	RHI::SubmitToVRComposter(buffer, eye);
}

glm::ivec2 ViveHMD::GetDimentions()
{
	uint32_t Width, height;
	system->GetRecommendedRenderTargetSize(&Width, &height);
	return glm::ivec2(Width, height);
}

bool ViveHMD::IsActive()
{
	return true;
}
