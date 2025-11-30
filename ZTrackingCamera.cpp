#include <cmath>
#include <DirectXMath.h>
#include "ZD3D11.h"
#include "ZTrackingCamera.h"
#include "imgui/imgui.h"

using namespace DirectX;

ZTrackingCamera::ZTrackingCamera()
    : targetPos_(0.0f, 0.0f, 0.0f)
    , cam3rdPos_(0.0f, 0.0f, 0.0f)
    , yaw_(0.0f)
    , pitch_(0.0f)
    , distance_(20.0f)
    , trackingSpeed_(5.0) // float 아닌가?
    , dtSave_(0.0f)
    , cameraPos_(0.0f, 0.0f, 0.0f)
    , isInitialized_(false)
    , gfx_(nullptr)
{
    XMStoreFloat4x4(&viewMatrix_, XMMatrixIdentity()); // 대각선이 1인 행렬, 단위행렬
}

void ZTrackingCamera::Initialize(ZGraphics& gfx)
{
    gfx_ = &gfx;
    cam3rdPos_ = targetPos_;
    UpdateViewMatrix();
    isInitialized_ = true;
}

void ZTrackingCamera::Update(float deltatTime, float dtSave)
{
    if (!isInitialized_) return;

    dtSave_ = dtSave;

    //targetObj_ -> car3rd_ 부드러운 추척(Lerp)
    XMVECTOR currentCam3rd = XMLoadFloat3(&cam3rdPos_);
    XMVECTOR target = XMLoadFloat3(&targetPos_);

    // 선형 보간
    XMVECTOR newCam3rd = XMVectorLerp(
        currentCam3rd,
        target,
        dtSave * trackingSpeed_
    );
    XMStoreFloat3(&cam3rdPos_, newCam3rd);

    UpdateViewMatrix();

}

void ZTrackingCamera::UpdateViewMatrix() // 물체와 카메라 사이의 거리만큼 빠진다음에 계산
{
    if (!isInitialized_) return;

    float cosPitch = cosf(pitch_);
    float sinPitch = sinf(pitch_);
    float cosYaw = cosf(yaw_);
    float sinYaw = sinf(yaw_);

    XMVECTOR offset = XMVectorSet(
        distance_ * cosPitch * sinYaw, // x
        distance_ * sinPitch, // y
        distance_ * cosPitch * cosYaw, // z
        0.0f
    );

    // 최종 카메라 위치 = 타겟위치 + 오프셋
    XMVECTOR target = XMLoadFloat3(&targetPos_);
    XMVECTOR camPos = XMVectorAdd(target, offset);
    XMStoreFloat3(&cameraPos_, camPos);

    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    XMMATRIX view = XMMatrixLookAtLH(camPos, target, up);
    XMStoreFloat4x4(&viewMatrix_, view);
}


void ZTrackingCamera::SetTarget(DirectX::XMFLOAT3 targetPos)
{
    targetPos_ = targetPos;
}

void ZTrackingCamera::SetTargetPosition(float x, float y, float z)
{
    targetPos_ = XMFLOAT3(x, y, z);
}

DirectX::XMMATRIX ZTrackingCamera::GetViewMatrix() const
{
    return XMLoadFloat4x4(&viewMatrix_);
}

DirectX::XMFLOAT3 ZTrackingCamera::GetCameraPosition() const
{
    return cameraPos_;
}

void ZTrackingCamera::ShowControlWindow()
{
    if (ImGui::Begin("Tracking Camera"))
    {
        // 추적 대상 위치 (읽기 전용)
        ImGui::Text("Target Position");
        ImGui::SameLine(); ImGui::Text("X: %.2f Y: %.2f Z: %.2f", targetPos_.x, targetPos_.y, targetPos_.z);

        // 카메라 위치 (읽기 전용)
        ImGui::Text("Camera Position");
        ImGui::SameLine(); ImGui::Text("X: %.2f Y: %.2f Z: %.2f", cameraPos_.x, cameraPos_.y, cameraPos_.z);

        // 3인칭 카메라 기준 위치 (읽기 전용)
        ImGui::Text("3rd Person Offset");
        ImGui::SameLine(); ImGui::Text("X: %.2f Y: %.2f Z: %.2f", cam3rdPos_.x, cam3rdPos_.y, cam3rdPos_.z);

        ImGui::Separator();

        // 추적 속도 조절
        bool speedChanged = false;
        speedChanged |= ImGui::SliderFloat("Tracking Speed", &trackingSpeed_, 0.1f, 20.0f, "%.1f");

        // 거리 조절
        bool distanceChanged = false;
        distanceChanged |= ImGui::SliderFloat("Distance", &distance_, 5.0f, 100.0f, "%.1f");

        // 회전 조절 (각도 단위로 표시 및 조절)
        bool rotationChanged = false;
        float yawDegrees = XMConvertToDegrees(yaw_);
        float pitchDegrees = XMConvertToDegrees(pitch_);

        rotationChanged |= ImGui::SliderFloat("Yaw (degrees)", &yawDegrees, -180.0f, 180.0f, "%.1f");
        rotationChanged |= ImGui::SliderFloat("Pitch (degrees)", &pitchDegrees, -89.0f, 89.0f, "%.1f");

        if (rotationChanged)
        {
            yaw_ = XMConvertToRadians(yawDegrees);
            pitch_ = XMConvertToRadians(pitchDegrees);
        }

        // 값이 변경되면 뷰 행렬 업데이트
        if (speedChanged || distanceChanged || rotationChanged)
        {
            UpdateViewMatrix();
        }

        ImGui::Separator();

        // 초기화 버튼
        if (ImGui::Button("Reset Camera"))
        {
            targetPos_ = { 0.0f, 0.0f, 0.0f };
            cam3rdPos_ = { 0.0f, 0.0f, 0.0f };
            yaw_ = 0.0f;
            pitch_ = 0.0f;
            distance_ = 20.0f;
            trackingSpeed_ = 5.0f;
            UpdateViewMatrix();
        }
    }
    ImGui::End();
}
