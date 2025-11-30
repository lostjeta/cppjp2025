#include "imgui/imgui.h"
#include "ZD3D11.h"
#include "ZBindableBase.h"
// Renderables3
#include "FbxModel.h"
// state
#include "BasicRenderState.h"
#include "PlayerControlState.h"
//Etc.
#include "GraphicsThrowMacros.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>

extern void ChangeState(GameState* newState, ZGraphics& gfx); // 프로젝트 어딘가에 있다.

const DirectX::XMFLOAT3 baseMaterialColor = {1.0f,1.0f, 1.0f}; // 빛 반사 색이 흰색으로
// 금속 재질 : XMFLOAT4{ 0.8f, 0.7f, 0.6f, 0.9f } (높은 반사율)
// 유리 재질 : XMFLOAT4{ 0.9f, 0.9f, 0.95f, 0.8f } (높은 투명도와 반사)
// 매트 재질 : XMFLOAT4{ 0.0f, 0.0f, 0.0f, 0.1f } (낮은 반사율)
const DirectX::XMFLOAT4 baseReflectionColor = { 0.0f,0.0f, 0.0f,0.5f }; // 반사 희미하게

void PlayerControlState::LoadErika(ZGraphics& gfx) {

    fbxModel_ = std::make_unique<FbxModel>(
        gfx,
        "./Data/Models/Erika/Erika Archer.fbx",
        baseMaterialColor,
        baseReflectionColor,
        std::vector<std::string>{
        "./Data/Models/Erika/Textures/FemaleFitA_Body_diffuse.png",
            "./Data/Models/Erika/Textures/Erika_Archer_Clothes_diffuse.png",
            "./Data/Models/Erika/Textures/FemaleFitA_eyelash_diffuse.png",
            "./Data/Models/Erika/Textures/FemaleFitA_Body_diffuse.png",
    },
    std::vector<std::string>{
        "./Data/Models/Erika/Textures/FemaleFitA_StdNM.png",
            "./Data/Models/Erika/Textures/Erika_Archer_Clothes_normal.png",
            "./Data/Models/Erika/Textures/FemaleFitA_StdNM.png",
            "./Data/Models/Erika/Textures/FemaleFitAStdNM.png",
    },
    std::vector<std::string>{}, // Specular
    std::vector<std::string>{
        "./Data/Models/Erika/Animations/Unarmed Idle 01.fbx",
            "./Data/Models/Erika/Animations/Catwalk Walk Forward.fbx",
            "./Data/Models/Erika/Animations/Drunk Walk.fbx",
    }
    );
    // FbxSkinnedModel Transform 설정
    if (fbxModel_)
    {
        // 카메라 위치: (0, 0, -20) → +Z 방향을 봄 (왼손 좌표계)
        // Alice를 더 멀리: z = -10 (카메라로부터 10유닛 앞)
        // 스케일 0.02: 높이 약 3.2유닛 (화면에 잘 맞음)
        fbxModel_->SetPosition({ 0.0f, 0.0f, -10.0f });  // 중앙, 발이 y=0
        fbxModel_->SetScale(0.02f);
    }
}

void PlayerControlState::LoadAtienze(ZGraphics & gfx) {
    fbxModel_ = std::make_unique<FbxModel>(
        gfx,
        "./Data/Models/Ely By K.Atienza/Ely By K.Atienza.fbx",
        baseMaterialColor,
        baseReflectionColor,
        std::vector<std::string>{
        "./Data/Models/Ely By K.Atienza/Textures/ely-vanguardsoldier-kerwinatienza_diffuse.png",
    },
        std::vector<std::string>{
        "./Data/Models/Ely By K.Atienza/Textures/ely-vanguardsoldier-kerwinatienza_normal.png",
    },
        std::vector<std::string>{
        "./Data/Models/Ely By K.Atienza/Textures/ely-vanguardsoldier-kerwinatienzaspecular.png",
    },
        std::vector<std::string>{
        "./Data/Models/Ely By K.Atienza/Animations/Unarmed Idle 01.fbx",
            "./Data/Models/Ely By K.Atienza/Animations/Catwalk Walk Forward.fbx",
            "./Data/Models/Ely By K.Atienza/Animations/Drunk Walk.fbx",
    }
    );
    // FbxSkinnedModel Transform 설정
    if (fbxModel_)
    {
        // 카메라 위치: (0, 0, -20) → +Z 방향을 봄 (왼손 좌표계)
        // Ely 모델: 바운딩 박스 Y=0.125~179.105, 중심 Y=89.6
        // 스케일 0.02 적용 후 Y=0에 맞추려면 -89.6*0.02 = -1.79
        fbxModel_->SetPosition({ 0.0f, -1.79f, -10.0f });  // 중앙, 발이 y=0
        fbxModel_->SetScale(0.02f);
    }
}

PlayerControlState::PlayerControlState(ZGraphics& gfx):gfx_(gfx)
{
}

void PlayerControlState::Enter(ZGraphics& gfx)
{
    LoadAtienze(gfx);
}

void PlayerControlState::Exit()
{
    fbxModel_.reset();

    if (gfx_.GetDeviceContext())
    {
        auto context = gfx_.GetDeviceContext();

        //D3D11 표준 제한:
        // 필셀 셰이더 :  최소 16개 슬롯 지원
        // 버텍스 셰이더  : 최소 16개 슬롯 지원
        // 8개는 최소지원

        // 셰이더 리소스 언바인드
        ID3D11ShaderResourceView* nullSRV[8] = { nullptr };
        context->PSSetShaderResources(0, 8, nullSRV);
        context->PSSetShaderResources(0, 8, nullSRV);

        // 상수 버퍼 언바인드
        ID3D11Buffer* nullBuffers[8] = { nullptr };
        context->PSSetConstantBuffers(0, 8, nullBuffers);
        context->VSSetConstantBuffers(0, 8, nullBuffers);

        UINT stride = 0;
        UINT offset = 0;

        ID3D11Buffer* nullVB[1] = { nullptr };
        context->IAGetVertexBuffers(0, 1, nullVB, &stride, &offset);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

        // 렌더 타겟 클리어
        gfx_.ClearBuffer(0.0f, 0.0f, 0.0f);

        context->Flush();
    };
}

void PlayerControlState::Update(float deltaTime)
{
    elapsedTime_ += deltaTime;
    deltaTime_ = deltaTime;

    fbxModel_->Update(deltaTime);
}

void PlayerControlState::Render(ZGraphics& gfx)
{
    fbxModel_->Render(gfx);

    if (gfx.IsImguiEnabled())
    {
        fbxModel_->ShowControlWindow();
    }
}

void PlayerControlState::OnKeyDown(WPARAM wParam)
{
}

void PlayerControlState::OnKeyUp(WPARAM wParam)
{
    if (wParam == VK_RETURN)
    {
        std::cout << "Goto BasicRenderState" << std::endl;
        ChangeState(new BasicRenderState(gfx_), gfx_);
    }
}

void PlayerControlState::OnMouseDown(int x, int y, int button)
{
}

void PlayerControlState::OnMouseUp(int x, int y, int button)
{
}

void PlayerControlState::OnMouseMove(int x, int y)
{
}
