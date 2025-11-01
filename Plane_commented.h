#pragma once

#include <vector>
#include <array>
#include "ZIndexedTriangleList.h"
#include "ZMath.h"

// Plane 클래스: 테셀레이션된 평면을 생성하는 클래스
class Plane
{
public:
    // MakeTesselated: 지정된 분할 수에 따라 평면을 여러 삼각형으로 나누어 생성
    // divisions_x: X축 방향 분할 개수
    // divisions_y: Y축 방향 분할 개수
    template<class V>
    static ZIndexedTriangleList<V> MakeTesselated(int divisions_x, int divisions_y)
    {
        namespace dx = DirectX;
        // 최소 1개 이상의 분할이 필요함
        assert(divisions_x >= 1);
        assert(divisions_y >= 1);

        // 평면의 전체 크기 정의 (2x2 크기의 평면)
        constexpr float width = 2.0f;   // 평면의 너비
        constexpr float height = 2.0f;  // 평면의 높이
        
        // 버텍스 개수 계산: 분할 개수 + 1 (예: 1개 분할 = 2개 버텍스)
        const int nVertices_x = divisions_x + 1;  // X축 방향 버텍스 개수
        const int nVertices_y = divisions_y + 1;  // Y축 방향 버텍스 개수
        
        // 전체 버텍스를 저장할 벡터 생성 (총 개수 = nVertices_x * nVertices_y)
        std::vector<V> vertices(nVertices_x * nVertices_y);

        // ========== 1단계: 버텍스 위치 계산 및 생성 ==========
        {
            // 평면의 중심을 원점(0,0)으로 하기 위해 절반 크기 계산
            const float side_x = width / 2.0f;   // X축 절반 길이 = 1.0f
            const float side_y = height / 2.0f;  // Y축 절반 길이 = 1.0f
            
            // 각 분할 구간의 크기 계산
            // 예: width=2.0, divisions_x=2 -> divisionSize_x = 1.0 (각 칸의 크기)
            const float divisionSize_x = width / float(divisions_x);
            const float divisionSize_y = height / float(divisions_y);
            
            // 시작점: 평면의 왼쪽 아래 모서리 (-1.0, -1.0, 0.0)
            const auto bottomLeft = dx::XMVectorSet(-side_x, -side_y, 0.0f, 0.0f);

            // Y축 방향으로 순회 (아래에서 위로)
            // i는 버텍스 배열의 인덱스
            for (int y = 0, i = 0; y < nVertices_y; y++)
            {
                // 현재 Y 좌표 계산 (0부터 시작하여 divisionSize_y씩 증가)
                const float y_pos = float(y) * divisionSize_y;
                
                // X축 방향으로 순회 (왼쪽에서 오른쪽으로)
                for (int x = 0; x < nVertices_x; x++, i++)
                {
                    // 버텍스 위치 계산:
                    // bottomLeft + (x * divisionSize_x, y_pos, 0)
                    // 예: divisions=1일 때
                    //   (0,0) -> (-1, -1, 0)
                    //   (1,0) -> ( 1, -1, 0)
                    //   (0,1) -> (-1,  1, 0)
                    //   (1,1) -> ( 1,  1, 0)
                    const auto v = dx::XMVectorAdd(
                        bottomLeft,
                        dx::XMVectorSet(float(x) * divisionSize_x, y_pos, 0.0f, 0.0f)
                    );
                    // 계산된 위치를 버텍스 배열에 저장
                    dx::XMStoreFloat3(&vertices[i].pos, v);
                }
            }
        }

        // ========== 2단계: 인덱스 생성 (삼각형 구성) ==========
        std::vector<unsigned short> indices;
        // 각 사각형은 2개의 삼각형(6개 인덱스)으로 구성됨
        indices.reserve(sqroot(divisions_x * divisions_y) * 6);
        {
            // vxy2i: (x, y) 좌표를 1차원 배열 인덱스로 변환하는 람다 함수
            // 공식: index = y * nVertices_x + x
            // 예: nVertices_x=3일 때, (1,2) -> 2*3+1 = 7
            const auto vxy2i = [nVertices_x](size_t x, size_t y)
                {
                    return (unsigned short)(y * nVertices_x + x);
                };
            
            // 각 사각형(quad)을 순회하며 삼각형 생성
            for (size_t y = 0; y < divisions_y; y++)
            {
                for (size_t x = 0; x < divisions_x; x++)
                {
                    // 사각형의 4개 모서리 버텍스 인덱스 계산
                    // indexArray[0]: 왼쪽 아래 (x, y)
                    // indexArray[1]: 오른쪽 아래 (x+1, y)
                    // indexArray[2]: 왼쪽 위 (x, y+1)
                    // indexArray[3]: 오른쪽 위 (x+1, y+1)
                    //
                    // 시각적 배치:
                    //   [2]------[3]
                    //    |        |
                    //    |        |
                    //   [0]------[1]
                    const std::array<unsigned short, 4> indexArray =
                    { vxy2i(x,y),vxy2i(x + 1,y),vxy2i(x,y + 1),vxy2i(x + 1,y + 1) };
                    
                    // 첫 번째 삼각형: 왼쪽 아래 삼각형 (0 -> 2 -> 1)
                    // 반시계 방향(CCW)으로 구성하여 앞면이 보이도록 함
                    indices.push_back(indexArray[0]);  // 왼쪽 아래
                    indices.push_back(indexArray[2]);  // 왼쪽 위
                    indices.push_back(indexArray[1]);  // 오른쪽 아래
                    
                    // 두 번째 삼각형: 오른쪽 위 삼각형 (1 -> 2 -> 3)
                    // 반시계 방향(CCW)으로 구성
                    indices.push_back(indexArray[1]);  // 오른쪽 아래
                    indices.push_back(indexArray[2]);  // 왼쪽 위
                    indices.push_back(indexArray[3]);  // 오른쪽 위
                }
            }
        }

        // ========== 3단계: 결과 반환 ==========
        // 생성된 버텍스와 인덱스를 ZIndexedTriangleList로 반환
        // std::move를 사용하여 효율적으로 소유권 이전
        return{ std::move(vertices),std::move(indices) };
    }
    
    // Make: 기본 평면 생성 (1x1 분할, 총 2개의 삼각형)
    template<class V>
    static ZIndexedTriangleList<V> Make()
    {
        return MakeTesselated<V>(1, 1);
    }
};
