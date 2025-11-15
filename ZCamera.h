#pragma once

class ZCamera
{
protected:
    // Camera Position, Rotation value	  
    float _xpos, _ypos, _zpos;
    float _pitch, _yaw, _roll;

    float _r = 20.0f; // dist from origin
    float _theta = 0.0f; // 적도 회전
    float _phi = 0.0f; // 경도 회전

    DirectX::XMMATRIX _matView;
    DirectX::XMMATRIX _matTranslation;
    DirectX::XMMATRIX _matRotation;

public:
    ZCamera();

    void Reset();
    DirectX::XMMATRIX GetMatrix() noexcept;
    void Update() noexcept;
    void SpawnControlWindow() noexcept;

    DirectX::XMFLOAT3 GetLookDir();

    // Move and rotate camera (view)
    BOOL Move(float xpos, float ypos, float zpos);
    BOOL MoveRel(float xadd = 0.0f, float yadd = 0.0f, float zadd = 0.0f);
    BOOL Rotate(float xrot, float yrot, float zrot);
    BOOL RotateRel(float xadd = 0.0f, float yadd = 0.0f, float zadd = 0.0f);

    // Point a camera from Eye position to At position
    BOOL Point(float xeye, float yeye, float zeye, float xat, float yat, float zat);

    // Retrieve translation and rotation values
    float GetXPos();
    float GetYPos();
    float GetZPos();
    float GetXRotation();
    float GetYRotation();
    float GetZRotation();
};
