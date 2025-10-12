#include "ZD3D11.h"
#include <sstream>
#include "GraphicsThrowMacros.h"
#include <d3dcompiler.h>
#include <DirectXMath.h> // dx math

#pragma comment(lib,"d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib") // Shader Compiler

namespace wrl = Microsoft::WRL;
namespace dx = DirectX;

ZGraphics::ZGraphics(HWND hWnd)
{
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

}

void ZGraphics::EndFrame()
{
	HRESULT hr;

#ifndef NDEBUG
	infoManager.Set();
#endif

	// 후면 버퍼의 내용을 화면에 표시(Present)합니다.
	// 첫 번째 인자(SyncInterval): 수직 동기화 옵션. 1은 수직 동기화를 켭니다.
	// 두 번째 인자(Flags): 추가적인 프레젠테이션 옵션.
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
}

void ZGraphics::DrawTriangle()
{
	HRESULT hr;

	struct Vertex
	{
		struct
		{
			float x;
			float y;
		} pos;
	};

	// NDC 화면 가운데가 (0,0), X(-1,1), Y(-1,1)
	// 왼손좌표계 앞면 : CW (Clock Wise)
	Vertex vertices[] =
	{
		{ 0.0f, 0.5f },
		{ 0.5f, -0.5f },
		{ -0.5f, -0.5f },
	};

	// Create VertexBuffer
	wrl::ComPtr<ID3D11Buffer> pVertexBuffer;
	D3D11_BUFFER_DESC bd = {};
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.CPUAccessFlags = 0u;
	bd.MiscFlags = 0u;
	bd.ByteWidth = sizeof(vertices);
	bd.StructureByteStride = sizeof(Vertex);

	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = vertices;
	GFX_THROW_INFO(pDevice->CreateBuffer(&bd, &sd, &pVertexBuffer));

	// Bind vertex buffer to pipeline
	const UINT stride = sizeof(Vertex);
	const UINT offset = 0u;
	pContext->IASetVertexBuffers(0u, 1u, pVertexBuffer.GetAddressOf(), &stride, &offset);

	// Create pixel shader
	wrl::ComPtr<ID3D11PixelShader> pPixelShader;
	wrl::ComPtr<ID3DBlob> pBlob;
	GFX_THROW_INFO(D3DReadFileToBlob(L"./x64/Debug/PixelShader.cso", &pBlob));
	GFX_THROW_INFO(pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pPixelShader));

	// bind pixel shader
	pContext->PSSetShader(pPixelShader.Get(), nullptr, 0u);

	// Create vertex shader
	wrl::ComPtr<ID3D11VertexShader> pVertexShader;
	GFX_THROW_INFO(D3DReadFileToBlob(L"./x64/Debug/VertexShader.cso", &pBlob));
	GFX_THROW_INFO(pDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pVertexShader));

	// Bind vertex shader
	pContext->VSSetShader(pVertexShader.Get(), nullptr, 0u);

	// VertexShader의 바이트코드(pBlob)가 필요하기 때문에 PixelShader를 먼저 생성한다.
	wrl::ComPtr<ID3D11InputLayout> pInputLayout;
	const D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "Position", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	GFX_THROW_INFO(pDevice->CreateInputLayout(
		ied, (UINT)std::size(ied),
		pBlob->GetBufferPointer(),
		pBlob->GetBufferSize(),
		&pInputLayout
	));
	// Bind vertex layout
	pContext->IASetInputLayout(pInputLayout.Get());

	// Bind render target
	pContext->OMSetRenderTargets(1u, pTarget.GetAddressOf(), nullptr);

	// Set primitive topology to triangle list (groups of 3 vertices)
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Configure viewport
	D3D11_VIEWPORT vp;
	vp.Width = (float)800;
	vp.Height = (float)600;
	vp.MinDepth = 0;
	vp.MaxDepth = 1;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	pContext->RSSetViewports(1u, &vp);

	GFX_THROW_INFO_ONLY(pContext->Draw((UINT)std::size(vertices), 0u));
}




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