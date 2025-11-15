#include "d3d11.h"
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
    // Camera Position, Rotation value	  
    _xpos = 0.0f;
    _ypos = 0.0f;
    _zpos = -20.0f;
    _pitch = 0.0f;
    _yaw = 0.0f;
    _roll = 0.0f;

    _r = 20.0f; // dist from origin
    _theta = 0.0f; // 적도 회전
    _phi = 0.0f; // 경도 회전
}

/**
 * @brief 카메라의 뷰 행렬(View Matrix)을 생성하여 반환합니다.
 * 
 * 이 함수는 구형 좌표계(Spherical Coordinates)를 사용하여 카메라의 위치를 계산하고,
 * LookAt 행렬과 추가 회전을 결합하여 최종 뷰 행렬을 생성합니다.
 * 
 * @return 카메라의 뷰 변환 행렬 (XMMATRIX)
 * 
 * 계산 과정:
 * 1. 구형 좌표계를 사용하여 카메라 위치 계산
 * 2. LookAt 행렬 생성 (카메라가 원점을 바라봄)
 * 3. 추가 회전(pitch, yaw, roll) 적용
 */
//DirectX::XMMATRIX ZCamera::GetMatrix() const noexcept
//{
//    // Step 1: 구형 좌표계를 사용하여 카메라 위치 계산
//    // 초기 위치: (0, 0, -_r) - 원점에서 -Z 방향으로 _r 거리만큼 떨어진 위치
//    // _phi: 위/아래 회전 (경도 회전, X축 기준)
//    // _theta: 좌/우 회전 (적도 회전, Y축 기준) - 음수로 적용하여 직관적인 방향 설정
//    // _r: 원점으로부터의 거리 (반지름)
//    const auto pos = XMVector3Transform(
//        XMVectorSet(0.0f, 0.0f, -_r, 0.0f),  // 초기 위치 벡터
//        XMMatrixRotationRollPitchYaw(_phi, -_theta, 0.0f)  // 회전 행렬 적용
//    );
//    
//    // Step 2: LookAt 행렬 생성
//    // - pos: 카메라의 위치 (Eye Position)
//    // - XMVectorZero(): 카메라가 바라보는 대상 위치 (원점, Look-At Position)
//    // - (0, 1, 0): 카메라의 업 벡터 (Up Vector) - Y축이 위쪽
//    //
//    // Step 3: 추가 회전 적용
//    // - _pitch: X축 회전 (위/아래 기울임)
//    // - _yaw: Y축 회전 (좌/우 회전) - 음수로 적용
//    // - _roll: Z축 회전 (좌/우 기울임)
    //// LookAt 행렬과 회전 행렬을 곱하여 최종 뷰 행렬 생성
    //return XMMatrixLookAtLH(
    //    pos,                                              // 카메라 위치
    //    XMVectorZero(),                                   // 카메라가 바라보는 지점 (원점)
    //    XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f)              // 업 벡터
    //) * XMMatrixRotationRollPitchYaw(_pitch, -_yaw, _roll);  // 추가 회전
//}

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

void ZCamera::SpawnControlWindow() noexcept
{
    if (ImGui::Begin("Camera"))
    {
        ImGui::Text("Position");
        bool posChanged = false;
        posChanged |= ImGui::SliderFloat("XPos", &_xpos, -10.0f, 10.0f, "%.1f");
        posChanged |= ImGui::SliderFloat("YPos", &_ypos, -10.0f, 10.0f, "%.1f");
        posChanged |= ImGui::SliderFloat("ZPos", &_zpos, -40.0f, 40.0f, "%.1f");
        
        if (posChanged)
        {
            Move(_xpos, _ypos, _zpos);
        }
        
        ImGui::Text("Orientation");
        bool rotChanged = false;
        rotChanged |= ImGui::SliderAngle("Roll", &_roll, -180.0f, 180.0f, "%.1f deg");
        rotChanged |= ImGui::SliderAngle("Pitch", &_pitch, -180.0f, 180.0f, "%.1f deg");
        rotChanged |= ImGui::SliderAngle("Yaw", &_yaw, -180.0f, 180.0f, "%.1f deg");
                      //                        ↑         ↑         ↑
                      //                        라디안    도 단위   도 단위
        
        if (rotChanged)
        {
            Rotate(_pitch, _yaw, _roll);
        }
        
        if (ImGui::Button("Reset"))
        {
            Reset();
            Move(_xpos, _ypos, _zpos);
            Rotate(_pitch, _yaw, _roll);
        }
    }
    ImGui::End();
}

XMFLOAT3 ZCamera::GetLookDir()
{
    // 화면 방향 벡터 (초기값: Z축 방향)
    XMVECTOR look = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);

    // X축 회전 행렬 생성
    XMMATRIX matXRot = XMMatrixRotationX(GetXRotation());

    // Y축 회전 행렬 생성
    XMMATRIX matYRot = XMMatrixRotationY(GetYRotation());

    // 행렬 곱셈 (X축 회전 후 Y축 회전)
    XMMATRIX matMul = XMMatrixMultiply(matXRot, matYRot);

    // 벡터 변환 (회전 적용)
    look = XMVector3TransformCoord(look, matMul);

    // XMVECTOR를 XMFLOAT3로 변환하여 반환
    XMFLOAT3 result;
    XMStoreFloat3(&result, look);

    return result;
}

BOOL ZCamera::Move(float xpos, float ypos, float zpos)
{
    _xpos = xpos;
    _ypos = ypos;
    _zpos = zpos;

    // 이동 행렬 생성 (카메라는 반대 방향으로 이동)
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

    // 회전 행렬 결합: Z * Y * X 순서
    // Roll 먼저, Yaw 두번째, Pitch 마지막
    // 이 순서는 Roll을 먼저 적용하여 이후 Yaw/Pitch가 월드 축 기준으로 동작
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

    // Calculate angles between points
    xdiff = xat - xeye;
    ydiff = yat - yeye;
    zdiff = zat - zeye;
    xrot = (float)atan2(-ydiff, sqrt(xdiff * xdiff + zdiff * zdiff));
    yrot = (float)atan2(xdiff, zdiff);

    Move(xeye, yeye, zeye);
    Rotate(xrot, yrot, 0.0f);

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
