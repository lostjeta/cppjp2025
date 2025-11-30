#pragma once

class ZTrackingCamera
{
public:
    ZTrackingCamera();
    ~ZTrackingCamera() = default;

    void Initialize(ZGraphics& gfx);
    void Update(float deltatTime, float dtSave = 0.0f);

    void SetTarget(DirectX::XMFLOAT3 targerPos);
    void SetTargetPosition(float x, float y, float z);
    DirectX::XMMATRIX GetViewMatrix() const;
    DirectX::XMFLOAT3 GetCameraPosition() const;
    //DirectX::XMMATRIX GetProjectionMatrix() const; // 그래픽스에 있어 제외

    void SetTrackingSpeed(float speed) { trackingSpeed_ = speed; }
    float GetTrackingSpeed() const { return trackingSpeed_; }

    // 줌 거리
    void SetDistance(float distance) { distance_ = distance; }
    float GetDistance() const { return distance_; }

    void SetYaw(float yaw) {yaw_ = yaw;} // y축기준
    void SetPitch(float pitch) { pitch = pitch;} // x축기준
    float GetYaw() const { return yaw_; }
    float GetPitch() const { return pitch_; }

    void ShowControlWindow();

private:
    void UpdateViewMatrix();

    // 카메라 계층 구조
    DirectX::XMFLOAT3 targetPos_; // 추적 대상 위치
    DirectX::XMFLOAT3 cam3rdPos_; // 3인칭 카메라 기준 위치
    DirectX::XMFLOAT3 cameraPos_; // 최종 카메라 위치

    //카메라 속성
    float yaw_;
    float pitch_;
    float distance_; // 카메라와 대상의 거리
    float trackingSpeed_;

    float dtSave_; // 프레임레이트 독립적 업데이트용 시간
    DirectX::XMFLOAT4X4 viewMatrix_;
    bool isInitialized_;
    ZGraphics* gfx_;
};