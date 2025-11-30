#pragma once
#include "GameState.h"
#include "SpriteBatch.h"
#include "ZTexture.h"
#include <set>
#include <optional>

class ZGUIManager;

class BasicRenderState : public GameState
{
private:
    double _elapsedTime;    // 경과 시간
    double _deltaTime;      // 프레임과 프레임 사이 시간
    int _curX, _curY;

    ZGraphics* _pGraphicsRef;  // For HWND access

    std::vector<std::unique_ptr<class SampleBox>> boxes;
    std::vector<std::unique_ptr<class Sheet>> sheets;
    std::vector<std::unique_ptr<class LightBox>> lightBoxes;
    std::vector<std::unique_ptr<class Cylinder>> lightCylinder;
    std::vector<std::unique_ptr<class Pyramid>> lightPyramid;
    std::vector<std::unique_ptr<class TexturedBox>> textureBox;

    std::vector<std::unique_ptr<class MeshTest>> meshModel;

    std::unique_ptr<class FbxStaticModel> fbxStaticModel;
    std::unique_ptr<class FbxTBNModel> fbxTBNModel;
    std::unique_ptr<class FbxSkinnedModel> fbxSkinnedModel;

    std::unique_ptr<DirectX::SpriteBatch> pSpriteBatch;
    std::unique_ptr<Bind::ZTexture> pTexture;

    ZGUIManager* m_pGUIManager;
    ZFont m_Font;  // 테스트용 폰트

    std::optional<int> comboBoxIndex;
    std::set<int> boxControlIds;

private:
    void SpawnLightBoxWindowManager(ZGraphics& gfx);
    void SpawnBoxWindows(ZGraphics& gfx) noexcept;

public:
    BasicRenderState(ZGraphics& gfx);
    ~BasicRenderState();

    void Enter(ZGraphics& gfx) override;
    void Exit() override;
    void Update(float deltaTime) override;
    void Render(ZGraphics& gfx) override;
    void OnKeyDown(WPARAM wParam) override;
    void OnKeyUp(WPARAM wParam) override;
    void OnMouseDown(int x, int y, int button) override;
    void OnMouseUp(int x, int y, int button) override;
    void OnMouseMove(int x, int y) override;

    void DrawTriangle(ZGraphics& gfx);
    void DrawIndexedTriangle(ZGraphics& gfx);
    void DrawConstTriangle(ZGraphics& gfx, double angle);
    void DrawDepthCube(ZGraphics& gfx, float angle, float x, float y); // using face color

    void DrawTexture(ZGraphics& gfx);
};