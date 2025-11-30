#pragma once
#include "ZD3D11.h"
#include "ZRenderable.h"
#include "ZIndexBuffer.h"
#include "ZConditionalNoexcept.h"

/**
 * @brief 렌더링 가능한 객체의 기본 클래스 (템플릿)
 * 
 * 이 클래스는 CRTP(Curiously Recurring Template Pattern)를 사용하여
 * 같은 타입의 객체들이 정적 바인딩 리소스를 공유할 수 있게 합니다.
 * 
 * @tparam T 파생 클래스 타입 (CRTP 패턴)
 * 
 * 주요 기능:
 * - 정적 바인딩 리소스 관리 (버텍스 버퍼, 인덱스 버퍼, 셰이더 등)
 * - 같은 타입의 객체들이 GPU 리소스를 공유하여 메모리 절약
 * - ZRenderable의 순수 가상 함수 구현
 * 
 * 사용 예:
 * @code
 * class MyObject : public ZRenderableBase<MyObject>
 * {
 *     MyObject(ZGraphics& gfx)
 *     {
 *         if (!IsStaticInitialized())
 *         {
 *             // 첫 번째 인스턴스만 정적 리소스 초기화
 *             AddStaticBind(std::make_unique<ZVertexBuffer>(gfx, vertices));
 *             AddStaticIndexBuffer(std::make_unique<ZIndexBuffer>(gfx, indices));
 *         }
 *         else
 *         {
 *             // 이후 인스턴스는 정적 리소스 재사용
 *             SetIndexFromStatic();
 *         }
 *     }
 * };
 * @endcode
 * 
 * CRTP 패턴 설명:
 * - 각 파생 클래스 T마다 별도의 staticBinds 정적 변수 생성
 * - Box<Box>와 Sphere<Sphere>는 서로 다른 staticBinds를 가짐
 * - 타입 안전성과 성능을 동시에 확보
 */
template<class T>
class ZRenderableBase : public ZRenderable
{
protected:
    /**
     * @brief 정적 바인딩 리소스가 초기화되었는지 확인
     * 
     * 첫 번째 객체 생성 시에만 GPU 리소스를 초기화하고,
     * 이후 객체들은 기존 리소스를 재사용하기 위해 사용합니다.
     * 
     * @return true 이미 초기화됨 (리소스 재사용 가능)
     * @return false 아직 초기화 안 됨 (리소스 생성 필요)
     * 
     * @note noexcept: 예외를 던지지 않음을 보장
     */
    static bool IsStaticInitialized() noexcept
    {
        return !staticBinds.empty();
    }
    
    /**
     * @brief 정적 바인딩 리소스 추가
     * 
     * 버텍스 버퍼, 셰이더, 입력 레이아웃 등 같은 타입의 모든 객체가
     * 공유할 GPU 리소스를 추가합니다.
     * 
     * @param bind 추가할 바인딩 리소스 (unique_ptr로 소유권 이전)
     * 
     * @warning 인덱스 버퍼는 이 함수 대신 AddStaticIndexBuffer()를 사용해야 함
     * @throws assertion 인덱스 버퍼를 이 함수로 추가하려 하면 assert 실패
     * 
     * @note noxnd: 조건부 noexcept (ZConditionalNoexcept.h 참조)
     * 
     * 사용 예:
     * @code
     * AddStaticBind(std::make_unique<ZVertexBuffer>(gfx, vertices));
     * AddStaticBind(std::make_unique<ZVertexShader>(gfx, L"shader.cso"));
     * AddStaticBind(std::make_unique<ZInputLayout>(gfx, layout, bytecode));
     * @endcode
     */
    static void AddStaticBind(std::unique_ptr<Bind::ZBindable> bind) noxnd
    {
        // 인덱스 버퍼는 특별 처리가 필요하므로 AddStaticIndexBuffer() 사용 강제
        assert("*Must* use AddStaticIndexBuffer to bind index buffer" && typeid(*bind) != typeid(Bind::ZIndexBuffer));
        staticBinds.push_back(std::move(bind));
    }
    
    /**
     * @brief 정적 인덱스 버퍼 추가 (특별 처리)
     * 
     * 인덱스 버퍼는 ZRenderable의 pIndexBuffer 포인터에도 저장되어야 하므로
     * 일반 바인딩과 다르게 처리됩니다.
     * 
     * @param ibuf 추가할 인덱스 버퍼 (unique_ptr로 소유권 이전)
     * 
     * @throws assertion 인덱스 버퍼가 이미 설정되어 있으면 assert 실패
     * 
     * @note 인덱스 버퍼는 Draw 호출 시 인덱스 개수를 알기 위해 별도 포인터 유지
     * 
     * 동작 과정:
     * 1. pIndexBuffer에 raw 포인터 저장 (ZRenderable 멤버)
     * 2. staticBinds에 unique_ptr 저장 (실제 소유권)
     * 
     * 사용 예:
     * @code
     * AddStaticIndexBuffer(std::make_unique<ZIndexBuffer>(gfx, indices));
     * @endcode
     */
    void AddStaticIndexBuffer(std::unique_ptr<Bind::ZIndexBuffer> ibuf) noxnd
    {
        assert("Attempting to add index buffer a second time" && pIndexBuffer == nullptr);
        pIndexBuffer = ibuf.get();  // raw 포인터 저장 (ZRenderable 멤버)
        staticBinds.push_back(std::move(ibuf));  // 소유권 이전
    }
    
    /**
     * @brief 정적 바인딩에서 인덱스 버퍼 포인터 재사용
     * 
     * 두 번째 이후 객체 생성 시, 이미 생성된 정적 인덱스 버퍼를
     * 찾아서 pIndexBuffer 포인터만 설정합니다.
     * 
     * @throws assertion 인덱스 버퍼를 찾지 못하면 assert 실패
     * @throws assertion pIndexBuffer가 이미 설정되어 있으면 assert 실패
     * 
     * 동작 과정:
     * 1. staticBinds를 순회하며 ZIndexBuffer 타입 검색
     * 2. 찾으면 pIndexBuffer에 raw 포인터 저장
     * 3. 못 찾으면 assert 실패 (정적 초기화 오류)
     * 
     * 사용 예:
     * @code
     * if (!IsStaticInitialized())
     * {
     *     AddStaticIndexBuffer(std::make_unique<ZIndexBuffer>(gfx, indices));
     * }
     * else
     * {
     *     SetIndexFromStatic();  // 기존 인덱스 버퍼 재사용
     * }
     * @endcode
     */
    void SetIndexFromStatic() noxnd
    {
        assert("Attempting to add index buffer a second time" && pIndexBuffer == nullptr);
        for (const auto& b : staticBinds)
        {
            if (const auto p = dynamic_cast<Bind::ZIndexBuffer*>(b.get()))
            {
                // 바인더 중에서 인덱스 버퍼 찾아서 포인터 재활용
                pIndexBuffer = p;
                return;
            }
        }
        // 인덱스 버퍼를 찾지 못함 - 정적 초기화 오류
        assert("Failed to find index buffer in static binds" && pIndexBuffer != nullptr);
    }
    
private:
    /**
     * @brief 정적 바인딩 리소스 접근자 (ZRenderable 순수 가상 함수 구현)
     * 
     * ZRenderable::Render()에서 호출되어 정적 바인딩 리소스를
     * GPU 파이프라인에 바인딩합니다.
     * 
     * @return const std::vector<std::unique_ptr<Bind::ZBindable>>& 정적 바인딩 리스트
     * 
     * @note override: ZRenderable::GetStaticBinds() 오버라이드
     * @note noexcept: 예외를 던지지 않음을 보장
     * @note private: ZRenderable만 접근 가능 (friend 선언)
     */
    const std::vector<std::unique_ptr<Bind::ZBindable>>& GetStaticBinds() const noexcept override
    {
        return staticBinds;
    }
    
private:
    /**
     * @brief 정적 바인딩 리소스 저장소
     * 
     * 같은 타입 T의 모든 객체가 공유하는 GPU 리소스 컨테이너입니다.
     * 
     * CRTP 패턴에 의해:
     * - ZRenderableBase<Box>는 Box 전용 staticBinds를 가짐
     * - ZRenderableBase<Sphere>는 Sphere 전용 staticBinds를 가짐
     * - 각 타입마다 별도의 정적 변수가 생성됨
     * 
     * 저장되는 리소스 예:
     * - ZVertexBuffer: 정점 데이터
     * - ZIndexBuffer: 인덱스 데이터
     * - ZVertexShader: 정점 셰이더
     * - ZPixelShader: 픽셀 셰이더
     * - ZInputLayout: 입력 레이아웃
     * - ZTopology: 프리미티브 토폴로지
     * 
     * @note static: 타입 T마다 하나의 인스턴스만 존재
     * @note 클래스 외부에서 정의 필요 (템플릿 정적 멤버)
     */
    static std::vector<std::unique_ptr<Bind::ZBindable>> staticBinds;
};

/**
 * @brief 정적 멤버 변수 정의
 * 
 * 템플릿 클래스의 정적 멤버는 헤더 파일에서 정의해야 합니다.
 * 각 템플릿 인스턴스(T)마다 별도의 staticBinds가 생성됩니다.
 * 
 * 메모리 레이아웃 예:
 * - ZRenderableBase<Box>::staticBinds (Box 전용)
 * - ZRenderableBase<Sphere>::staticBinds (Sphere 전용)
 * - ZRenderableBase<Pyramid>::staticBinds (Pyramid 전용)
 */
template<class T>
std::vector<std::unique_ptr<Bind::ZBindable>> ZRenderableBase<T>::staticBinds;