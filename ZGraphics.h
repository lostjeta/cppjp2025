#pragma once
#include "ChiliException.h"
#include <wrl.h> // ComPtr
#include <DirectXMath.h> // DirectX Math
#include "DxgiInfoManager.h"
#include "ZConditionalNoExcept.h"

// D3D 11의 초기화 및 핵심 인터페이스 관리

class ZGraphics
{
	friend class ZGraphicsResource;

public:
    // DirectionalLight 정보 구조체
    struct DirectionalLightData
    {
        DirectX::XMFLOAT4 ambient = { 0.2f, 0.2f, 0.2f, 1.0f };
        DirectX::XMFLOAT4 diffuse = { 0.3f, 0.3f, 0.3f, 1.0f };
        DirectX::XMFLOAT4 specular = { 0.2f, 0.2f, 0.2f, 1.0f };
        DirectX::XMFLOAT3 direction = { 0.0f, -1.0f, 1.0f };
    };

    // PointLight 정보 구조체
    struct PointLightData
    {
        DirectX::XMFLOAT4 ambient = { 0.05f, 0.05f, 0.05f, 1.0f };
        DirectX::XMFLOAT4 diffuse = { 1.0f, 1.0f, 1.0f, 1.0f };
        DirectX::XMFLOAT4 specular = { 1.0f, 1.0f, 1.0f, 1.0f };
        DirectX::XMFLOAT3 position = { 0.0f, 0.0f, 0.0f };
        float range = 100.0f;
        DirectX::XMFLOAT3 att = { 1.0f, 0.045f, 0.0075f };
    };

private:
    bool imguiEnabled = true;
    DirectX::XMMATRIX projection;
    DirectX::XMMATRIX camera;
    DirectionalLightData directionalLight;  // DirectionalLight 정보 저장
    PointLightData pointLight;              // PointLight 정보 저장

	Microsoft::WRL::ComPtr<ID3D11Device> pDevice;			// D3D11 장치, 리소스 생성 및 관리
	Microsoft::WRL::ComPtr<IDXGISwapChain> pSwap;			// 스왑 체인, 후면 버퍼와 화면 출력을 교체
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> pContext;	// D3D11 장치 컨텍스트, 렌더링 명령을 GPU에 전달
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> pTarget;	// 렌더 타겟 뷰, 렌더링 결과가 저장되는 곳
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> pDSV;
    Microsoft::WRL::ComPtr<ID3D11BlendState> pBlendState;	// 알파 블렌드 상태

    double winRatio;
    HANDLE m_hWnd;
    DWORD m_ClientWidth;
    DWORD m_ClientHeight;

#ifndef NDEBUG
	DxgiInfoManager infoManager;
#endif

public:
	class Exception : public ChiliException
	{
		using ChiliException::ChiliException;
	};
	class HrException : public Exception
	{
	public:
		HrException(int line, const char* file, HRESULT hr, std::vector<std::string> infoMsgs = {}) noexcept;
		const char* what() const noexcept override;
		const char* GetType() const noexcept override;
		HRESULT GetErrorCode() const noexcept;
		std::string GetErrorString() const noexcept;
		std::string GetErrorDescription() const noexcept;
		std::string GetErrorInfo() const noexcept;
	private:
		HRESULT hr;
		std::string info;
	};
	class InfoException : public Exception
	{
	public:
		InfoException(int line, const char* file, std::vector<std::string> infoMsgs) noexcept;
		const char* what() const noexcept override;
		const char* GetType() const noexcept override;
		std::string GetErrorInfo() const noexcept;
	private:
		std::string info;
	};
	class DeviceRemovedException : public HrException
	{
		using HrException::HrException;
	public:
		const char* GetType() const noexcept override;
	private:
		std::string reason;
	};

//protected:
    public:
    // 후면 버퍼를 지정된 색상으로 초기화
    // red (0.0f ~ 1.0f)
    // green (0.0f ~ 1.0f)
    // blue (0.0f ~ 1.0f)
    void ClearBuffer(float red, float green, float blue) noexcept;

public:
	ZGraphics(HWND hWnd, double winRatio, DWORD width, DWORD height);

	// 복사 생성자와 대입 연산자를 사용하지 않는다. (객체 복사 방지)
	// -- D3D 리소스 고유하게 유지
	ZGraphics(const ZGraphics&) = delete;
	ZGraphics& operator=(const ZGraphics&) = delete;

    ~ZGraphics();

	// 현재 프레임의 렌더링을 끝내고 후면 버퍼를 화면에 표시한다.
	void EndFrame();
    void BeginFrame(float red, float green, float blue) noexcept;
    void SetViewport() noexcept;
    void RenderIndexed(UINT count) noxnd;
    void SetProjection(DirectX::FXMMATRIX proj) noexcept;
    DirectX::XMMATRIX GetProjection() const noexcept;
    void SetCamera(DirectX::FXMMATRIX cam) noexcept;
    void SetDirectionalLight(const DirectionalLightData& light) noexcept;
    const DirectionalLightData& GetDirectionalLight() const noexcept;
    void SetPointLight(const PointLightData& light) noexcept;
    const PointLightData& GetPointLight() const noexcept;
    DirectX::XMMATRIX GetCamera() const noexcept;
    void EnableImgui() noexcept;
    void DisableImgui() noexcept;
    bool IsImguiEnabled() const noexcept;
    
    // DirectXTK 사용을 위한 인터페이스 접근 메서드
    ID3D11Device* GetDeviceCOM() noexcept;
    ID3D11DeviceContext* GetDeviceContext() noexcept;
    ID3D11BlendState* GetBlendState() noexcept;
    HWND GetHWND() noexcept;
    DWORD GetClientWidth();
    DWORD GetClientHeight();
};
