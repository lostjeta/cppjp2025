#pragma once
#include "GameState.h"

class PlayerControlState : public GameState
{
public:
    void LoadErika(ZGraphics& gfx);
    void LoadAtienze(ZGraphics& gfx);

public:
    PlayerControlState(ZGraphics& gfx);

    // GameState을(를) 통해 상속됨
    void Enter(ZGraphics& gfx) override;

    void Exit() override;

    void Update(float deltaTime) override;

    void Render(ZGraphics& gfx) override;

    void OnKeyDown(WPARAM wParam) override;

    void OnKeyUp(WPARAM wParam) override;

    void OnMouseDown(int x, int y, int button) override;

    void OnMouseUp(int x, int y, int button) override;

    void OnMouseMove(int x, int y) override; // 파스칼 방식

private:
    ZGraphics& gfx_;
    double elapsedTime_; // 경과시간
    double deltaTime_; // 게임 플레이 사이사이 시간(프레임과 프레임 사이 시간)

    std::unique_ptr<class FbxModel> fbxModel_; // 클래스 멤버변수 뒤에 _ 

};