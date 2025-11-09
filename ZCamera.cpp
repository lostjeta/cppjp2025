#include "ZD3D11.h"
#include <DirectXMath.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include "ZCamera.h"

using namespace DirectX;

ZCamera::ZCamera()
{
    Reset();
    Move(_xpos, _ypos, _zpos);
    Rotate(_pitch, _yaw, _roll);
}


void ZCamera::Reset()
{
    _xpos = 0.0f;
    _ypos = 0.0f;
    _zpos = -20.0f;
    _pitch = 0.0f;
    _yaw = 0.0f;
    _roll = 0.0f;

    _r = 20.0f;
    _theta = 0.0f;
    _phi = 0.0f;
}

/**

@brief 카메라의 뷰 행렬(View Matrix)을 생성하여 반환합니다.
이 함수는 구형 좌표계(Spherical Coordinates)를 사용하여 카메라의 위치를 계산하고,
LookAt 행렬과 추가 회전을 결합하여 최종 뷰 행렬을 생성합니다.
@return 카메라의 뷰 변환 행렬 (XMMATRIX)
계산 과정:
구형 좌표계를 사용하여 카메라 위치 계산
LookAt 행렬 생성 (카메라가 원점을 바라봄)
추가 회전(pitch, yaw, roll) 적용
*/
/*
DirectX::XMMATRIX ZCamera::GetMatrix() noexcept
{
    // Step 1: 구형 좌표계를 사용하여 카메라 위치 계산
    // 초기 위치: (0, 0, -_r) - 원점에서 -Z 방향으로 _r 거리만큼 떨어진 위치
    // _phi: 위/아래 회전 (경도 회전, X축 기준)
    // _theta: 좌/우 회전 (적도 회전, Y축 기준) - 음수로 적용하여 직관적인 방향 설정
    // _r: 원점으로부터의 거리 (반지름)
    const auto pos = XMVector3Transform(
        XMVectorSet(0.0f, 0.0f, -_r, 0.0f),  // 초기 위치 벡터
        XMMatrixRotationRollPitchYaw(_phi, -_theta, 0.0f)  // 회전 행렬 적용
    );

    // Step 2: LookAt 행렬 생성
    // - pos: 카메라의 위치 (Eye Position)
    // - XMVectorZero(): 카메라가 바라보는 대상 위치 (원점, Look-At Position)
    // - (0, 1, 0): 카메라의 업 벡터 (Up Vector) - Y축이 위쪽
    //
    // Step 3: 추가 회전 적용
    // - _pitch: X축 회전 (위/아래 기울임)
    // - _yaw: Y축 회전 (좌/우 회전) - 음수로 적용
    // - _roll: Z축 회전 (좌/우 기울임)
    // LookAt 행렬과 회전 행렬을 곱하여 최종 뷰 행렬 생성
    return XMMatrixLookAtLH(
        pos,                                              // 카메라 위치
        XMVectorZero(),                                   // 카메라가 바라보는 지점 (원점)
        XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)              // 업 벡터
    ) * XMMatrixRotationRollPitchYaw(_pitch, -_yaw, _roll);  // 추가 회전
}
*/
DirectX::XMMATRIX ZCamera::GetMatrix() noexcept
{
    Update();
    return _matView;
}

void ZCamera::Update() noexcept
{
    MoveRel();
    RotateRel();

    _matView = _matTranslation * _matRotation;
}

void ZCamera::SpawnConrtolWindow() noexcept
{
        if (ImGui::Begin("Camera"))
        {
            ImGui::Text("Position");
            ImGui::SliderFloat("XPos", &_xpos, -10.0f, 10.0f, "%.1f");
            ImGui::SliderFloat("YPos", &_ypos, -10.0f, 10.0f, "%.1f");
            ImGui::SliderFloat("ZPos", &_zpos, -40.0f, 40.0f, "%.1f");
            //ImGui::SliderFloat("R", &_r, 0.1f, 10.0f, "%.1f"); // 0이 나오면 안됨(에러)
            //ImGui::SliderFloat("Theta", &_theta, -3.14f, 3.14f, "%.1f");
            //ImGui::SliderFloat("Phi", &_phi, -3.14f, 3.14f, "%.1f");
            
            
            ImGui::Text("Orientation");
            //ImGui::SliderAngle("Roll", &_roll, -3.14f, 3.14f, "%.1f");
            ImGui::SliderAngle("Roll", &_roll, -89.0f, 89.0f, "%.1f");
            //ImGui::SliderAngle("Pitch", &_pitch, -3.14f, 3.14f, "%.1f");
            ImGui::SliderAngle("Pitch", &_pitch, -89.0f, 89.0f, "%.1f");
            //ImGui::SliderAngle("Yaw", &_yaw, -3.14f, 3.14f, "%.1f");
            ImGui::SliderAngle("Yaw", &_yaw, -89.0f, 89.0f, "%.1f");
            
            if (ImGui::Button("Reset"))
            {
                Reset();
            }
        }
        ImGui::End();
}

DirectX::XMFLOAT3 ZCamera::GetLookDir()
{
    // 화면 방향 벡터 (초기값: Z축 방향)

    XMVECTOR look = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

    // X축 회전 행령 생성
    XMMATRIX matXRot = XMMatrixRotationX(GetXRotation());

    // Y축 회전 행령 생성
    XMMATRIX matYRot = XMMatrixRotationY(GetYRotation());

    // Z축 회전 행령 생성
    XMMATRIX matZRot = XMMatrixRotationZ(GetZRotation());

    // 행렬 곱셈(X축 회전 후 Y축 회전)
    XMMATRIX matMul = XMMatrixMultiply(matXRot, matYRot);

    //벡터 변환(회전 적용)
    look = XMVector3TransformCoord(look, matMul);

    // XMVECTOR를 XMFLOAT3로 변환하여 반환
    XMFLOAT3 result; // 타입변환?
    XMStoreFloat3(&result, look);

    return result;
}

BOOL ZCamera::Move(float xpos, float ypos, float zpos)
{
    _xpos = xpos;
    _ypos = ypos;
    _zpos = zpos;

    //이동하는 곳의 반대방향으로 카메라 이동
    _matTranslation = XMMatrixTranslation(-_xpos, -_ypos, -_zpos);



    return TRUE;
}

BOOL ZCamera::MoveRel(float xadd, float yadd, float zadd)
{
    return Move(_xpos + xadd, _ypos + yadd, _zpos + zadd);
}

BOOL ZCamera::Rotate(float xrot, float yrot, float zrot)
{
    _pitch = xrot;
    _yaw = yrot;
    _roll = zrot;

    // 각 축별 회전 행렬 생성
    XMMATRIX matXRotation = XMMatrixRotationX(-_pitch);
    XMMATRIX matYRotation = XMMatrixRotationY(-_yaw);
    XMMATRIX matZRotation = XMMatrixRotationZ(-_roll);

    // 회전 행렬 결합
    _matRotation = matZRotation;
    _matRotation = XMMatrixMultiply(_matRotation, matYRotation);
    _matRotation = XMMatrixMultiply(_matRotation, matXRotation);

    return TRUE;
}

BOOL ZCamera::RotateRel(float xadd, float yadd, float zadd)
{
    return Rotate(_pitch + xadd, _yaw + yadd, _roll + zadd);
}

BOOL ZCamera::Point(float xeye, float yeye, float zeye, float xat, float yat, float zat)
{
    float xrot, yrot, xdiff, ydiff, zdiff;
    xdiff = xat - xeye;
    ydiff = yat - yeye;
    zdiff = zat - zeye;
    xrot = (float)atan2(-ydiff, sqrt(xdiff * xdiff + zdiff * zdiff));
    yrot = (float)atan2(xdiff, zdiff);

    Move(xeye, yeye, zeye);
    Rotate(xrot, yrot, 0.0);

    return TRUE;
}

float ZCamera::GetXPos()
{
    return _xpos;
}

float ZCamera::GetYPos()
{
    return _ypos;
}

float ZCamera::GetZPos()
{
    return _zpos;
}

float ZCamera::GetXRotation()
{
    return _pitch;
}

float ZCamera::GetYRotation()
{
    return _yaw;
}

float ZCamera::GetZRotation()
{
    return _roll;
}




