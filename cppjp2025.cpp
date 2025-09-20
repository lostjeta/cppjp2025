// cppjp2025.cpp : 이 파일에는 'main' 함수가 포함됩니다. 거기서 프로그램 실행이 시작되고 종료됩니다.
//

#include <iostream>
#include "ZVector3.h"

int main()
{
    ZVector3 v1(1, 1, 1);
    std::cout << v1 << std::endl;

    ZVector3 v2(-1, 1, 1); // 외적
    std::cout << v2.cross(v1) << std::endl;

    std::cout << v1.dot(v2) << std::endl;

}