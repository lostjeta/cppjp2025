#pragma once

class GameState : public ZGraphicsResource
{
public:
	virtual ~GameState() {}

	virtual void Enter(ZGraphics& gfx) = 0;
	virtual void Exit() = 0;
	virtual void Update(float deltaTime) = 0;
	virtual void Render(ZGraphics& gfx) = 0;
	virtual void OnKeyDown(WPARAM wParam) = 0;
	virtual void OnKeyUp(WPARAM wParam) = 0;
	virtual void OnMouseDown(int x, int y, int button) = 0;
	virtual void OnMouseUp(int x, int y, int button) = 0;
	virtual void OnMouseMove(int x, int y) = 0;
};