#pragma once
#include "GameState.h"
#include "SpriteBatch.h"
#include "ZTexture.h"

class ZGUIManager;
class BasicRenderState : public GameState
{
private:
    double _elapsedTime;    // 경과 시간
    double _deltaTime;      // 프레임과 프레임 사이 시간
    int _curX, _curY;

    ZGraphics* _pGraphicsRef;

    std::vector<std::unique_ptr<class SampleBox>> boxes;
    std::vector<std::unique_ptr<class Sheet>> sheets;

    std::unique_ptr<DirectX::SpriteBatch> pSpriteBatch;
    std::unique_ptr<Bind::ZTexture> pTexture;

    ZGUIManager* m_pGUIManager;
    ZFont m_Font;

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