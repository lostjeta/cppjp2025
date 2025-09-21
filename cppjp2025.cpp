// cppjp2025.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include "ZVector3.h"
#include "ZMatrix.h"

int main()
{
    ZVector3 v1(1, 2, 3);
    std::cout << v1 << std::endl;

    ZVector3 v2(4, -5, 6); // 외적
    std::cout << v2.Cross(v1) << std::endl;

    std::cout << v1.Dot(v2) << std::endl;

    std::cout << ZVector3::Dot(v1, v2) << std::endl;
    std::cout << v1.radBetween(v1, v2) << std::endl;
    std::cout << v1.degBetween(v1, v2) << std::endl;

    ZVector3 localPoint(1.0, 1.0, 1.0);
    std::cout << "Local Space Point : " << localPoint << std::endl;

    //Scale 모든 축으로 2배 확대
    ZMatrix matScale = ZMatrix::CreateScale(2.0, 2.0, 2.0); 

    // Rotation : Y축 90도 회전(어느 방향으로 몇만큼 회전할지까지 알면 좋음
    const float PI = 3.1415926535897932;
    ZMatrix matRoatation = ZMatrix::CreateRotationY(PI / 2.0);

    //Translation : 월드 공간 (5,6,7)위치로 이동
    ZMatrix matTranslation = ZMatrix::CreateTranslation(5.0, 6.0, 7.0);

    // 변환을 순차적으로 적용
    ZVector3 pointAferScale = localPoint.Transform(matScale);
    std::cout << pointAferScale << std::endl;

    ZVector3 pointAfterRotation = pointAferScale.Transform(matRoatation);
    std::cout << pointAfterRotation << std::endl;

    ZVector3 finalWoldPoint = pointAfterRotation.Transform(matTranslation);
    std::cout << finalWoldPoint << std::endl;

    // 한번에 월드 매트릭스
    ZMatrix worldMatrix = matScale * matRoatation * matTranslation;
    ZVector3 finalWorldPointByMatrix = localPoint.Transform(worldMatrix);
    std::cout << finalWorldPointByMatrix << std::endl;
}