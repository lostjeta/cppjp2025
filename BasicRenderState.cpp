#include "imgui/imgui.h"
#include "ZGUI.h"
#include "ZBindableBase.h"
//renderables
#include "Sheet.h"
#include "SampleBox.h"
#include "LightBox.h"
#include "Cylinder.h"
#include "Pyramid.h"
#include "TexturedBox.h"
#include "MeshTest.h"
#include "FbxStaticModel.h"
#include "FbxTBNModel.h"
#include "FbxSkinnedModel.h"
#include "SolidSphere.h"
//states
#include "BasicRenderState.h"
#include "PlayerControlState.h"
#include "GraphicsThrowMacros.h"
#include <d3dcompiler.h>
#include <DirectXMath.h> // dx math

namespace wrl = Microsoft::WRL;
namespace dx = DirectX;

extern void ChangeState(GameState* newState, ZGraphics& gfx);

BasicRenderState::BasicRenderState(ZGraphics& gfx)
    : 
    _elapsedTime(0.0), 
    _deltaTime(0.0),
    _curX(0),
    _curY(0),
    _pGraphicsRef(&gfx)
{
    pSpriteBatch = std::make_unique<DirectX::SpriteBatch>(gfx.GetDeviceContext());
    pTexture = std::make_unique<Bind::ZTexture>(gfx, L"./Data/Images/dxlogo_256.bmp");
}

BasicRenderState::~BasicRenderState()
{
}

void BasicRenderState::Enter(ZGraphics& gfx)
{
    std::cout << "Enter BasicRenderState" << std::endl;

    m_pGUIManager = new ZGUIManager(gfx.GetClientWidth(), gfx.GetClientHeight());

    // GUI 로딩
    if (FALSE == m_pGUIManager->Init(&gfx))
        return;

    // 폰트 초기화 (한 번만 생성)
    std::cout << "[Enter] Initializing m_Font..." << std::endl;
    if (m_Font.Create(gfx, "./Data/Font/NanumGothic_Full_16.spritefont", 24))
    {
        std::cout << "[Enter] m_Font initialized successfully!" << std::endl;
    }
    else
    {
        std::cout << "[Enter] ERROR: Failed to initialize m_Font!" << std::endl;
    }

    std::mt19937 rng(std::random_device{}()); // 메르센 트위스터 난수 생성기 초기화
    std::uniform_real_distribution<float> adist(0.0f, 3.1415f * 2.0f);  // 각도(Angle) 분포: 0에서 2π까지의 랜덤 각도
    std::uniform_real_distribution<float> ddist(0.0f, 3.1415f * 2.0f);  // 방향(Direction) 분포: 0에서 2π까지의 랜덤 방향
    std::uniform_real_distribution<float> odist(0.0f, 3.1415f * 0.3f);  // 오프셋(Offset) 분포: 0에서 0.3π까지의 작은 오프셋
    std::uniform_real_distribution<float> rdist(20.0f, 30.0f);          // 반경(Radius) 분포: 10에서 20까지의 랜덤 거리
    std::uniform_real_distribution<float> bdist(0.4f, 3.0f);            // 바이어스(Bias) 분포: Z축 스케일링을 위한 0.4에서 3.0까지의 랜덤 값
    std::uniform_real_distribution<float> cdist(0.0f, 1.0f);            // 색상(Color) 분포: 0에서 1까지의 랜덤 색상 값
    std::uniform_int_distribution<int> tdist{ 3,30 };                   // 테셀레이션(Tessellation) 분포: 3에서 30까지의 랜덤 분할 수

    
    for (auto i = 0; i < 30; i++)
    {
        boxes.push_back(std::make_unique<SampleBox>(
            gfx, rng, adist,
            ddist, odist, rdist
        ));

        sheets.push_back(std::make_unique<Sheet>(
            gfx, rng, adist,
            ddist, odist, rdist
        ));

        const DirectX::XMFLOAT3 mat = { cdist(rng), cdist(rng), cdist(rng) };

        lightBoxes.push_back(std::make_unique<LightBox>(
            gfx, rng, adist, ddist,
            odist, rdist, bdist, mat
        ));

        lightCylinder.push_back(std::make_unique<Cylinder>(
            gfx, rng, adist, ddist,
            odist, rdist, tdist
        ));

        lightPyramid.push_back(std::make_unique<Pyramid>(
            gfx, rng, adist, ddist, odist, rdist, tdist
        ));

        textureBox.push_back(std::make_unique<TexturedBox>(
            gfx, rng, adist, ddist, odist, rdist
        ));

        // MeshTest는 SetPos가 없어서 위치 제어 불가
        meshModel.push_back(std::make_unique<MeshTest>(
            gfx, rng, adist, ddist, odist, rdist, mat, 1.5f
        ));
    }


    const DirectX::XMFLOAT3 baseMaterialColor = { 1.0f, 1.0f, 1.0f };
    // 금속 재질 : XMFLOAT4{ 0.8f, 0.7f, 0.6f, 0.9f } (높은 반사율)
    // 유리 재질 : XMFLOAT4{ 0.9f, 0.9f, 0.95f, 0.8f } (높은 투명도와 반사)
    // 매트 재질 : XMFLOAT4{ 0.0f, 0.0f, 0.0f, 0.1f } (낮은 반사율)
    const DirectX::XMFLOAT4 baseReflectionColor = { 0.0f, 0.0f, 0.0f, 0.5f };

    // Textured MODEL
    //fbxStaticModel = std::make_unique<FbxStaticModel>(
    //    gfx, "./Data/Models/Alice/Alice.fbx", baseMaterialColor
    //    //gfx, "./Data/Models/Skeletons/models/DungeonSkeleton.FBX", baseMaterialColor, "./Data/Models/Skeletons/models/DS_skeleton_standard.png"
    //    //gfx, "./Data/Models/Toon_RTS_Knight/models/ToonRTS_demo_Knight.FBX", baseMaterialColor, "./Data/Models/Toon_RTS_Knight/models/DemoTexture.png"
    //);
    //// FbxStaticModel Transform 설정
    //if (fbxStaticModel)
    //{
    //    // 카메라 위치: (0, 0, -20) → +Z 방향을 봄 (왼손 좌표계)
    //    // Alice를 더 멀리: z = -10 (카메라로부터 10유닛 앞)
    //    // 스케일 0.02: 높이 약 3.2유닛 (화면에 잘 맞음)
    //    fbxStaticModel->SetPosition({ 0.0f, 0.0f, -10.0f });  // 중앙, 발이 y=0
    //    fbxStaticModel->SetScale(0.02f);
    //}


    // NORMAL MODEL
    //fbxTBNModel = std::make_unique<FbxTBNModel>(
    //    gfx, "./Data/Models/Alice/Alice.fbx", baseMaterialColor
    //    //gfx, "./Data/Models/Toon_RTS_Knight/models/ToonRTS_demo_Knight.FBX", baseMaterialColor, "./Data/Models/Toon_RTS_Knight/models/DemoTexture.png"
    //);
    //// FbxStaticModel Transform 설정
    //if (fbxTBNModel)
    //{
    //    // 카메라 위치: (0, 0, -20) → +Z 방향을 봄 (왼손 좌표계)
    //    // Alice를 더 멀리: z = -10 (카메라로부터 10유닛 앞)
    //    // 스케일 0.02: 높이 약 3.2유닛 (화면에 잘 맞음)
    //    fbxTBNModel->SetPosition({ 0.0f, 0.0f, -10.0f });  // 중앙, 발이 y=0
    //    fbxTBNModel->SetScale(0.02f);
    //}


    // BONE ANIMATION    
    //fbxSkinnedModel = std::make_unique<FbxSkinnedModel>(
    //    gfx,
    //    "./Data/Models/Alice/Alice_.fbx",
    //    baseMaterialColor,
    //    std::vector<std::string>{
    //    },
    //    std::vector<std::string>{
    //    }
    //);


    // fbxSkinnedModel = std::make_unique<FbxSkinnedModel>(
    //     gfx, 
    //     "./Data/Models/Erika/Erika Archer.fbx",
    //     baseMaterialColor,
    //     baseReflectionColor,
    //     std::vector<std::string>{
    //         "./Data/Models/Erika/Textures/FemaleFitA_Body_diffuse.png",
    //         "./Data/Models/Erika/Textures/Erika_Archer_Clothes_diffuse.png",
    //         "./Data/Models/Erika/Textures/FemaleFitA_eyelash_diffuse.png",
    //         "./Data/Models/Erika/Textures/FemaleFitA_Body_diffuse.png",
    //     },
    //     std::vector<std::string>{
    //         "./Data/Models/Erika/Textures/FemaleFitA_StdNM.png",
    //         "./Data/Models/Erika/Textures/Erika_Archer_Clothes_normal.png",
    //         "./Data/Models/Erika/Textures/FemaleFitA_StdNM.png",
    //         "./Data/Models/Erika/Textures/FemaleFitA_StdNM.png",
    //     },
    //     std::vector<std::string>{}, // Specular
    //     std::vector<std::string>{
    //          "./Data/Models/Erika/Animations/Unarmed Idle 01.fbx",
    //          "./Data/Models/Erika/Animations/Catwalk Walk Forward.fbx",
    //          "./Data/Models/Erika/Animations/Drunk Walk.fbx",                 
    //     }
    // );
    //// FbxSkinnedModel Transform 설정
    //if (fbxSkinnedModel)
    //{
    //    // 카메라 위치: (0, 0, -20) → +Z 방향을 봄 (왼손 좌표계)
    //    // Alice를 더 멀리: z = -10 (카메라로부터 10유닛 앞)
    //    // 스케일 0.02: 높이 약 3.2유닛 (화면에 잘 맞음)
    //    fbxSkinnedModel->SetPosition({ 0.0f, 0.0f, -10.0f });  // 중앙, 발이 y=0
    //    fbxSkinnedModel->SetScale(0.02f);
    //}


    fbxSkinnedModel = std::make_unique<FbxSkinnedModel>(
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
           "./Data/Models/Ely By K.Atienza/Textures/ely-vanguardsoldier-kerwinatienza_specular.png",
       },
       std::vector<std::string>{
           "./Data/Models/Ely By K.Atienza/Animations/Unarmed Idle 01.fbx",
           "./Data/Models/Ely By K.Atienza/Animations/Catwalk Walk Forward.fbx",
           "./Data/Models/Ely By K.Atienza/Animations/Drunk Walk.fbx",
       }
    );
    // FbxSkinnedModel Transform 설정
    if (fbxSkinnedModel)
    {
        // 카메라 위치: (0, 0, -20) → +Z 방향을 봄 (왼손 좌표계)
        // Ely 모델: 바운딩 박스 Y=0.125~179.105, 중심 Y=89.6
        // 스케일 0.02 적용 후 Y=0에 맞추려면 -89.6*0.02 = -1.79
        fbxSkinnedModel->SetPosition({ 0.0f, -1.79f, -10.0f });  // 중앙, 발이 y=0
        fbxSkinnedModel->SetScale(0.02f);
    }
}

void BasicRenderState::Exit()
{
    std::cout << "Exit BasicRenderState" << std::endl;

    // 모든 렌더링 오브젝트 제거
    boxes.clear();
    sheets.clear();
    lightBoxes.clear();
    lightCylinder.clear();
    lightPyramid.clear();
    textureBox.clear();
    meshModel.clear();

    fbxStaticModel.reset();
    fbxTBNModel.reset();
    fbxSkinnedModel.reset();

    pTexture.reset();
    pSpriteBatch.reset();

    // D3D 렌더 파이프라인 정리
    if (_pGraphicsRef)
    {
        auto context = _pGraphicsRef->GetDeviceContext();

        // D3D11 표준 제한:
        // 픽셀 셰이더: 최소 16개 슬롯 지원 (대부분 128개)
        // 버텍스 셰이더: 최소 16개 슬롯 지원 (대부분 128개)
        // 8개는 안전한 최소값

        // 셰이더 리소스 언바인드 (모든 슬롯)
        ID3D11ShaderResourceView* nullSRV[8] = { nullptr };
        context->PSSetShaderResources(0, 8, nullSRV);
        context->VSSetShaderResources(0, 8, nullSRV);

        // 상수 버퍼 언바인드
        ID3D11Buffer* nullBuffers[8] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };
        context->PSSetConstantBuffers(0, 8, nullBuffers);
        context->VSSetConstantBuffers(0, 8, nullBuffers);

        // 정점/인덱스 버퍼 언바인드
        UINT stride = 0;
        UINT offset = 0;
        ID3D11Buffer* nullVB[1] = { nullptr };
        context->IASetVertexBuffers(0, 1, nullVB, &stride, &offset);
        context->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);

        // 렌더 타겟 클리어 (ZGraphics 메서드 사용)
        _pGraphicsRef->ClearBuffer(0.0f, 0.0f, 0.0f);

        // 플러시하여 명령 실행
        context->Flush();
    }

    // 폰트 해제
    m_Font.Free();

    m_pGUIManager->Clear();
    SAFE_DELETE(m_pGUIManager);
}

void BasicRenderState::Update(float deltaTime)
{
    _elapsedTime += deltaTime;
    _deltaTime = deltaTime;

    //for (auto& b : boxes) b->Update(deltaTime);
    //for (auto& b : sheets) b->Update(deltaTime);
    //m_pGUIManager->Update(deltaTime);

    // LIGHT
    for (auto& b : lightBoxes) b->Update(deltaTime);
    for (auto& b : lightCylinder) b->Update(deltaTime);
    for (auto& b : lightPyramid) b->Update(deltaTime);
    for (auto& b : textureBox) b->Update(deltaTime);
    for (auto& m : meshModel) m->Update(deltaTime);

    //fbxStaticModel->Update(deltaTime);
    //fbxTBNModel->Update(deltaTime);
    fbxSkinnedModel->Update(deltaTime);

    //for (auto& s : markerSpheres) s->Update(deltaTime);
}

void BasicRenderState::Render(ZGraphics& gfx)
{
    //DrawTriangle(gfx);
    //DrawIndexedTriangle(gfx);
    //DrawConstTriangle(gfx, _elapsedTime);

    //DrawDepthCube(gfx,
    //    float(-_elapsedTime),
    //    0.0f,
    //    0.0f
    //);
    //DrawDepthCube(gfx, 
    //    float(_elapsedTime), 
    //    (((float)_curX / ((float)GetClientWidth(gfx) / 2.0f)) - 1.0f) * 2,
    //    ((-(float)_curY / ((float)GetClientHeight(gfx) / 2.0f)) + 1.0f) * 2
    //); // using face color

    //DrawTexture(gfx);
    //for (auto& b : boxes) b->Render(gfx);     
    //for (auto& b : sheets) b->Render(gfx);

    // LIGHT
    for (auto& b : lightBoxes) b->Render(gfx);
    for (auto& b : lightCylinder) b->Render(gfx);
    for (auto& b : lightPyramid) b->Render(gfx);
    for (auto& b : textureBox) b->Render(gfx);
    for (auto& m : meshModel) m->Render(gfx);

    //fbxStaticModel->Render(gfx);
    //fbxTBNModel->Render(gfx);
    fbxSkinnedModel->Render(gfx);

    //for (auto& s : markerSpheres) s->Render(gfx);  // 마커 먼저

    if (gfx.IsImguiEnabled())
    {
        SpawnLightBoxWindowManager(gfx);
        SpawnBoxWindows(gfx);

        //fbxStaticModel->ShowControlWindow();
        //fbxTBNModel->ShowControlWindow();
        fbxSkinnedModel->ShowControlWindow();
    }

    // 알파 블렌드 상태 활성화 (투명 텍스처 렌더링을 위해)
    const float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    GetContext(gfx)->OMSetBlendState(GetBlendState(gfx), blendFactor, 0xffffffff);

    //m_pGUIManager->Render(_deltaTime);


    //-----

    //pSpriteBatch->Begin();

    //if (m_Font.GetSpriteFont() != nullptr)
    //{
    //    // Font rendering test
    //    m_Font.FastPrint(100, 100, "Hello World", pSpriteBatch.get());
    //    m_Font.FastPrint(100, 120, "Welcome!", pSpriteBatch.get());
    //    m_Font.FastPrint(100, 140, "Font Test", pSpriteBatch.get());

    //    // Color test (Red)
    //    DirectX::XMFLOAT4 red(1.0f, 0.0f, 0.0f, 1.0f);
    //    m_Font.PrintLine(100, 160, 300, red, "Red Text", pSpriteBatch.get());

    //    // Green text
    //    DirectX::XMFLOAT4 green(0.0f, 1.0f, 0.0f, 1.0f);
    //    m_Font.PrintLine(100, 180, 300, green, "Green Text", pSpriteBatch.get());

    //    // Blue text
    //    DirectX::XMFLOAT4 blue(0.0f, 0.0f, 1.0f, 1.0f);
    //    m_Font.PrintLine(100, 200, 300, blue, "Blue Text", pSpriteBatch.get());

    //    // Centered text (Top)
    //    DirectX::XMFLOAT4 white(1.0f, 1.0f, 1.0f, 1.0f);
    //    m_Font.PrintEx(0, 0, 800, 100, white, DT_CENTER | DT_VCENTER, "Game Title", pSpriteBatch.get());
    //    
    //    // Centered text (Bottom)
    //    DirectX::XMFLOAT4 yellow(1.0f, 1.0f, 0.0f, 1.0f);
    //    m_Font.PrintEx(0, 500, 800, 100, yellow, DT_CENTER | DT_BOTTOM, "Press SPACE to start", pSpriteBatch.get());
    //}

    //pSpriteBatch->End();


    // 알파 블렌드 비활성화 (3D 객체 렌더링을 위해)
    GetContext(gfx)->OMSetBlendState(nullptr, blendFactor, 0xffffffff);
}

void BasicRenderState::SpawnLightBoxWindowManager(ZGraphics& gfx)
{
    // imgui window to open box windows
    if (ImGui::Begin("Light Boxes"))
    {
        using namespace std::string_literals;
        const auto preview = comboBoxIndex ? std::to_string(*comboBoxIndex) : "Choose a box..."s;
        if (ImGui::BeginCombo("Box Number", preview.c_str()))
        {
            for (int i = 0; i < lightBoxes.size(); i++)
            {
                const bool selected = comboBoxIndex && *comboBoxIndex == i;
                if (ImGui::Selectable(std::to_string(i).c_str(), selected))
                {
                    comboBoxIndex = i;
                }
                if (selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::Button("Spawn Control Window") && comboBoxIndex)
        {
            boxControlIds.insert(*comboBoxIndex);
            comboBoxIndex.reset();
        }
    }
    ImGui::End();
}

void BasicRenderState::SpawnBoxWindows(ZGraphics& gfx) noexcept
{
    for (auto i = boxControlIds.begin(); i != boxControlIds.end(); )
    {
        if (!lightBoxes[*i]->SpawnControlWindow(*i, gfx))
        {
            i = boxControlIds.erase(i);
        }
        else
        {
            i++;
        }
    }
}

void BasicRenderState::OnKeyDown(WPARAM wParam)
{
    if (wParam == VK_RETURN)
    {
        std::cout << "Goto PlayState" << std::endl;
        //ChangeState(new PlayState());
    }
}

void BasicRenderState::OnKeyUp(WPARAM wParam)
{
    if (wParam == VK_RETURN)
    {
        std::cout << "Goto PlayerControlState_pGraphicsRef" << std::endl;
        ChangeState(new PlayerControlState(*_pGraphicsRef),*_pGraphicsRef);
    }
}

void BasicRenderState::OnMouseDown(int x, int y, int button)
{
    if (m_pGUIManager && m_pGUIManager->IsInit())
    {
        // Windows 메시지로 변환
        UINT uMsg = (button == 0) ? WM_LBUTTONDOWN : (button == 1) ? WM_RBUTTONDOWN : WM_MBUTTONDOWN;
        LPARAM lParam = MAKELPARAM(x, y);
        WPARAM wParam = (button == 0) ? MK_LBUTTON : (button == 1) ? MK_RBUTTON : MK_MBUTTON;
        
        // ZGUIManager에 메시지 전달
        HWND hWnd = _pGraphicsRef->GetHWND();
        m_pGUIManager->MsgProc(hWnd, uMsg, wParam, lParam);
    }
}

void BasicRenderState::OnMouseUp(int x, int y, int button)
{   
    if (m_pGUIManager && m_pGUIManager->IsInit())
    {
        // Windows 메시지로 변환
        UINT uMsg = (button == 0) ? WM_LBUTTONUP : (button == 1) ? WM_RBUTTONUP : WM_MBUTTONUP;
        LPARAM lParam = MAKELPARAM(x, y);
        WPARAM wParam = 0;  // UP 메시지에서는 버튼 플래그 제거
        
        HWND hWnd = _pGraphicsRef->GetHWND();
        m_pGUIManager->MsgProc(hWnd, uMsg, wParam, lParam);
    }
}

void BasicRenderState::OnMouseMove(int x, int y)
{
    _curX = x;
    _curY = y;
    // std::cout << _curX << " " << _curY << std::endl;
    
    if (m_pGUIManager && m_pGUIManager->IsInit())
    {
        // Windows 메시지로 변환
        LPARAM lParam = MAKELPARAM(x, y);
        WPARAM wParam = 0;  // 버튼 상태는 현재 모르므로 0
        
        HWND hWnd = _pGraphicsRef->GetHWND();
        m_pGUIManager->MsgProc(hWnd, WM_MOUSEMOVE, wParam, lParam);
    }
}
