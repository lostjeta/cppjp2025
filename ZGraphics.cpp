#include "ZD3D11.h"
#include <sstream>
#include "GraphicsThrowMacros.h"
#include <d3dcompiler.h>
#include <DirectXMath.h> // dx math
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib") // Shader Compiler

namespace wrl = Microsoft::WRL;
namespace dx = DirectX;

ZGraphics::ZGraphics(HWND hWnd, double winRatio, DWORD width, DWORD height)
    :
    winRatio(winRatio),
    m_hWnd(hWnd),
    m_ClientWidth(width),
    m_ClientHeight(height)
{
    HRESULT hr;

	// 스왑 체인을 설정하기 위한 구조체입니다.
	DXGI_SWAP_CHAIN_DESC sd = {};
	// 후면 버퍼의 너비와 높이를 설정합니다. 0으로 설정하면 윈도우 크기에 맞춰 자동으로 조절됩니다.
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	// 후면 버퍼의 픽셀 포맷을 설정합니다. (B8G8R8A8_UNORM: 32비트, 채널당 8비트, BGRA 순서)
	sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	// 화면 주사율(Refresh Rate)을 설정합니다. 0으로 설정하면 시스템 기본값을 사용합니다.
	sd.BufferDesc.RefreshRate.Numerator = 0;
	sd.BufferDesc.RefreshRate.Denominator = 0;
	// 디스플레이 스케일링 모드를 설정합니다.
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	// 스캔 라인 렌더링 순서를 설정합니다.
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	// 멀티샘플링(MSAA) 설정을 합니다. Count=1, Quality=0은 멀티샘플링을 사용하지 않음을 의미합니다.
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	// 후면 버퍼의 사용 용도를 설정합니다. 렌더 타겟으로 사용됩니다.
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// 후면 버퍼의 개수를 설정합니다. 일반적으로 1개(더블 버퍼링) 또는 2개(트리플 버퍼링)를 사용합니다.
	sd.BufferCount = 1;
	// 렌더링 결과를 출력할 윈도우의 핸들입니다.
	sd.OutputWindow = hWnd;
	// 창 모드(TRUE) 또는 전체 화면 모드(FALSE)를 설정합니다.
	sd.Windowed = TRUE;
	// 후면 버퍼를 화면에 표시한 후의 처리 방식을 설정합니다. (DISCARD: 내용을 버림)
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	// 추가적인 플래그를 설정합니다.
	sd.Flags = 0;

	// D3D11 장치(Device), 장치 컨텍스트(Context), 스왑 체인(Swap Chain)을 생성합니다.
	D3D11CreateDeviceAndSwapChain(
		nullptr,						// pAdapter: 기본 어댑터(그래픽 카드)를 사용합니다.
		D3D_DRIVER_TYPE_HARDWARE,		// DriverType: 하드웨어 가속을 사용합니다.
		nullptr,						// Software: 하드웨어 드라이버를 사용하므로 NULL입니다.
		0,							// Flags: 추가적인 생성 플래그 (예: 디버그 모드)
		nullptr,						// pFeatureLevels: 지원할 기능 수준 배열, NULL이면 기본값을 사용합니다.
		0,							// FeatureLevels: 기능 수준 배열의 크기입니다.
		D3D11_SDK_VERSION,			// SDKVersion: 항상 D3D11_SDK_VERSION으로 설정합니다.
		&sd,						// pSwapChainDesc: 위에서 설정한 스왑 체인 설정 구조체입니다.
		&pSwap,						// ppSwapChain: 생성된 스왑 체인 인터페이스를 받을 포인터입니다.
		&pDevice,					// ppDevice: 생성된 장치 인터페이스를 받을 포인터입니다.
		nullptr,						// pFeatureLevel: 실제로 선택된 기능 수준을 받을 포인터입니다.
		&pContext					// ppImmediateContext: 생성된 장치 컨텍스트를 받을 포인터입니다.
	);

	// 스왑 체인으로부터 후면 버퍼에 접근하기 위한 리소스를 가져옵니다.
	wrl::ComPtr<ID3D11Resource> pBackBuffer;
	pSwap->GetBuffer(0, __uuidof(ID3D11Resource), &pBackBuffer);

	// 후면 버퍼에 대한 렌더 타겟 뷰를 생성합니다.
	// 렌더링 파이프라인의 최종 출력은 이 렌더 타겟 뷰에 쓰여집니다.
	pDevice->CreateRenderTargetView(
		pBackBuffer.Get(),		// pResource: 렌더 타겟으로 사용할 리소스 (후면 버퍼)
		nullptr,			// pDesc: 렌더 타겟 뷰 설정, NULL이면 리소스의 기본 설정을 따릅니다.
		&pTarget			// ppRTView: 생성된 렌더 타겟 뷰를 받을 포인터입니다.
	);

    // depth 버퍼 초기화 할 때 사용한다.
    // create depth stensil state
    D3D11_DEPTH_STENCIL_DESC dsDesc = {};
    dsDesc.DepthEnable = TRUE;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS; // z축으로 가까운 것이 다른 것을 오버라이드한다.
    wrl::ComPtr<ID3D11DepthStencilState> pDSState;
    GFX_THROW_INFO(pDevice->CreateDepthStencilState(&dsDesc, &pDSState));

    // bind depth state
    pContext->OMSetDepthStencilState(pDSState.Get(), 1u);

    // create depth stensil texture
    wrl::ComPtr<ID3D11Texture2D> pDepthStencil;
    D3D11_TEXTURE2D_DESC descDepth = {};
    descDepth.Width = (UINT)m_ClientWidth;
    descDepth.Height = (UINT)m_ClientHeight;
    descDepth.MipLevels = 1u;
    descDepth.ArraySize = 1u;
    descDepth.Format = DXGI_FORMAT_D32_FLOAT;
    descDepth.SampleDesc.Count = 1u;
    descDepth.SampleDesc.Quality = 0u;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    GFX_THROW_INFO(pDevice->CreateTexture2D(&descDepth, nullptr, &pDepthStencil));

    // create view of depth stensil texture
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
    descDSV.Format = DXGI_FORMAT_D32_FLOAT;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0u;
    GFX_THROW_INFO(pDevice->CreateDepthStencilView(pDepthStencil.Get(), &descDSV, &pDSV));

    // bind depth stensil view to OM
    pContext->OMSetRenderTargets(1u, pTarget.GetAddressOf(), pDSV.Get());

    // create alpha blend state
    D3D11_BLEND_DESC blendDesc = {};
    blendDesc.RenderTarget[0].BlendEnable = TRUE;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    GFX_THROW_INFO(pDevice->CreateBlendState(&blendDesc, &pBlendState));

    // init imgui d3d impl
    ImGui_ImplDX11_Init(pDevice.Get(), pContext.Get());
}

ZGraphics::~ZGraphics()
{
    ImGui_ImplDX11_Shutdown();
}

void ZGraphics::EndFrame()
{
    if (imguiEnabled)
    {
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

	HRESULT hr;

#ifndef NDEBUG
	infoManager.Set();
#endif

	// 후면 버퍼의 내용을 화면에 표시(Present)합니다.
	// 첫 번째 인자(SyncInterval): 수직 동기화 옵션. 1은 수직 동기화를 켭니다.
	// 두 번째 인자(Flags): 추가적인 프레젠테이션 옵션.
    //DXGI_SWAP_EFFECT_SEQUENTIAL
	if (FAILED(hr = pSwap->Present(1u, 0u)))
	{
		if (hr == DXGI_ERROR_DEVICE_REMOVED)
		{
			throw GFX_DEVICE_REMOVED_EXCEPT(pDevice->GetDeviceRemovedReason());
		}
		else
		{
			throw GFX_EXCEPT(hr);
		}
	}
}

void ZGraphics::ClearBuffer(float red, float green, float blue) noexcept
{
	// 렌더 타겟 뷰를 지정된 색상으로 초기화합니다.
	const float color[] = { red,green,blue,1.0f }; // RGBA 순서
	pContext->ClearRenderTargetView(pTarget.Get(), color);
    pContext->ClearDepthStencilView(pDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0u);
}

void ZGraphics::BeginFrame(float red, float green, float blue) noexcept
{
    if (imguiEnabled)
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
    }

    ClearBuffer(red, green, blue);
}

// 뷰포트(Viewport)를 설정하는 함수
// 뷰포트는 3D 공간의 렌더링 결과를 2D 화면의 어느 영역에 표시할지를 정의합니다.
// 예를 들어, 분할 화면 게임에서 각 플레이어의 화면 영역을 지정할 때 사용됩니다.
void ZGraphics::SetViewport() noexcept
{
    // D3D11_VIEWPORT 구조체를 생성하여 뷰포트 설정을 정의합니다.
    D3D11_VIEWPORT vp;
    
    // Width: 뷰포트의 너비를 픽셀 단위로 설정합니다.
    // 클라이언트 영역의 전체 너비를 사용합니다.
    vp.Width = (float)m_ClientWidth;
    
    // Height: 뷰포트의 높이를 픽셀 단위로 설정합니다.
    // 클라이언트 영역의 전체 높이를 사용합니다.
    vp.Height = (float)m_ClientHeight;
    
    // MinDepth: 깊이 버퍼의 최소값입니다. (0.0 = 카메라에 가장 가까운 지점)
    // 일반적으로 0.0으로 설정하여 Near Plane을 나타냅니다.
    vp.MinDepth = 0;
    
    // MaxDepth: 깊이 버퍼의 최대값입니다. (1.0 = 카메라에서 가장 먼 지점)
    // 일반적으로 1.0으로 설정하여 Far Plane을 나타냅니다.
    vp.MaxDepth = 1;
    
    // TopLeftX: 뷰포트의 왼쪽 상단 X 좌표입니다.
    // 0으로 설정하면 화면의 왼쪽 끝부터 시작합니다.
    vp.TopLeftX = 0;
    
    // TopLeftY: 뷰포트의 왼쪽 상단 Y 좌표입니다.
    // 0으로 설정하면 화면의 위쪽 끝부터 시작합니다.
    vp.TopLeftY = 0;
    
    // RSSetViewports: Rasterizer Stage에 뷰포트를 설정합니다.
    // 첫 번째 인자(1u): 설정할 뷰포트의 개수 (여러 개 설정 가능)
    // 두 번째 인자(&vp): 뷰포트 구조체 배열의 포인터
    pContext->RSSetViewports(1u, &vp);
}

void ZGraphics::RenderIndexed(UINT count) noxnd
{
    GFX_THROW_INFO_ONLY(pContext->DrawIndexed(count, 0u, 0u));
}

void ZGraphics::SetProjection(DirectX::FXMMATRIX proj) noexcept
{
    projection = proj;
}

DirectX::XMMATRIX ZGraphics::GetProjection() const noexcept
{
    return projection;
}

void ZGraphics::SetCamera(DirectX::FXMMATRIX cam) noexcept
{
    camera = cam;
}

DirectX::XMMATRIX ZGraphics::GetCamera() const noexcept
{
    return camera;
}

void ZGraphics::EnableImgui() noexcept
{
    imguiEnabled = true;
}

void ZGraphics::DisableImgui() noexcept
{
    imguiEnabled = false;
}

bool ZGraphics::IsImguiEnabled() const noexcept
{
    return imguiEnabled;
}

ID3D11Device* ZGraphics::GetDeviceCOM() noexcept
{
    return pDevice.Get();
}

ID3D11DeviceContext* ZGraphics::GetDeviceContext() noexcept
{
    return pContext.Get();
}

ID3D11BlendState* ZGraphics::GetBlendState() noexcept
{
	return pBlendState.Get();
}

HWND ZGraphics::GetHWND() noexcept
{
    return static_cast<HWND>(m_hWnd);
}

DWORD ZGraphics::GetClientWidth()
{
    return m_ClientWidth;
}

DWORD ZGraphics::GetClientHeight()
{
    return m_ClientHeight;
}

//------------------------------------------------------------------------------
// Graphics exception stuff

ZGraphics::HrException::HrException(int line, const char* file, HRESULT hr, std::vector<std::string> infoMsgs) noexcept
	:
	Exception(line, file),
	hr(hr)
{
	// join all info messages with newlines into single string
	for (const auto& m : infoMsgs)
	{
		info += m;
		info.push_back('\n');
	}
	// remove final newline if exists
	if (!info.empty())
	{
		info.pop_back();
	}
}

const char* ZGraphics::HrException::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << std::endl
		<< "[Error Code] 0x" << std::hex << std::uppercase << GetErrorCode()
		<< std::dec << " (" << (unsigned long)GetErrorCode() << ")" << std::endl
		<< "[Error String] " << GetErrorString() << std::endl
		<< "[Description] " << GetErrorDescription() << std::endl;
	if (!info.empty())
	{
		oss << "\n[Error Info]\n" << GetErrorInfo() << std::endl << std::endl;
	}
	oss << GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}

const char* ZGraphics::HrException::GetType() const noexcept
{
	return "Chili Graphics Exception";
}

HRESULT ZGraphics::HrException::GetErrorCode() const noexcept
{
	return hr;
}

std::string ZGraphics::HrException::GetErrorString() const noexcept
{
	// DXGetErrorString은 TCHAR*를 반환하므로, std::string으로 변환해야 합니다.
	const TCHAR* errorString = DXGetErrorString(hr);
#ifdef _UNICODE
	// 유니코드 환경에서는 wchar_t*를 char*로 변환합니다.
	std::wstring w_str(errorString);
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &w_str[0], (int)w_str.size(), NULL, 0, NULL, NULL);
	std::string str_to(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &w_str[0], (int)w_str.size(), &str_to[0], size_needed, NULL, NULL);
	return str_to;
#else
	// 멀티바이트 환경에서는 바로 변환 가능합니다.
	return std::string(errorString);
#endif
}

std::string ZGraphics::HrException::GetErrorDescription() const noexcept
{
	TCHAR buf[512];
	DXGetErrorDescription(hr, buf, sizeof(buf) / sizeof(TCHAR));
#ifdef _UNICODE
	// 유니코드 환경에서는 wchar_t*를 char*로 변환합니다.
	std::wstring w_str(buf);
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &w_str[0], (int)w_str.size(), NULL, 0, NULL, NULL);
	std::string str_to(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &w_str[0], (int)w_str.size(), &str_to[0], size_needed, NULL, NULL);
	return str_to;
#else
	// 멀티바이트 환경에서는 바로 변환 가능합니다.
	return std::string(buf);
#endif
}

std::string ZGraphics::HrException::GetErrorInfo() const noexcept
{
	return info;
}


const char* ZGraphics::DeviceRemovedException::GetType() const noexcept
{
	return "Chili Graphics Exception [Device Removed] (DXGI_ERROR_DEVICE_REMOVED)";
}
ZGraphics::InfoException::InfoException(int line, const char* file, std::vector<std::string> infoMsgs) noexcept
	:
	Exception(line, file)
{
	// join all info messages with newlines into single string
	for (const auto& m : infoMsgs)
	{
		info += m;
		info.push_back('\n');
	}
	// remove final newline if exists
	if (!info.empty())
	{
		info.pop_back();
	}
}


const char* ZGraphics::InfoException::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << std::endl
		<< "\n[Error Info]\n" << GetErrorInfo() << std::endl << std::endl;
	oss << GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}

const char* ZGraphics::InfoException::GetType() const noexcept
{
	return "Chili Graphics Info Exception";
}

std::string ZGraphics::InfoException::GetErrorInfo() const noexcept
{
	return info;
}