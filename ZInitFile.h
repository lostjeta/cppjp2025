#pragma once

#include <Windows.h>
#include <map>
#include <string>
#include <vector>

//---------------------------------------------------------------------------
// ZInitFile - Windows INI 파일 관리 클래스
// .ift (Init File Text) 확장자를 사용하지만 표준 INI 파일 형식
//---------------------------------------------------------------------------
class ZInitFile
{
private:
    std::string m_FilePath;             // INI 파일 경로
    BOOL m_bLoaded;                     // 파일이 로드되었는가
    
    // 캐시된 데이터 (성능 향상을 위해)
    std::map<std::string, std::map<std::string, std::string>> m_Cache;

public:
    ZInitFile();
    ~ZInitFile();

    // INI 파일 로드
    BOOL LoadIFT(const char* pFilePath);
    BOOL LoadIFT(const std::string& filePath);

    // 파일 언로드 및 캐시 클리어
    void Clear();

    // 값 읽기
    std::string GetValue(const char* pSection, const char* pKey, const char* pDefault = "");
    std::string GetValue(const std::string& section, const std::string& key, const std::string& defaultValue = "");
    
    // 인덱스 기반 섹션에서 값 읽기 (섹션명이 "0", "1", "2" 등인 경우)
    std::string GetValue(int iIndex, const char* pKey, const char* pDefault = "");
    std::string GetValue(int iIndex, const std::string& key, const std::string& defaultValue = "");

    // 값 쓰기
    BOOL SetValue(const char* pSection, const char* pKey, const char* pValue);
    BOOL SetValue(const std::string& section, const std::string& key, const std::string& value);
    BOOL SetValue(int iIndex, const char* pKey, const char* pValue);

    // 섹션 리스트 가져오기
    BOOL GetSectionList(std::vector<std::string>& outList);
    
    // 섹션명 리스트 가져오기 (GetTitleList와 동일)
    BOOL GetTitleList(std::vector<std::string>& outList);

    // 특정 섹션의 모든 키 가져오기
    BOOL GetKeyList(const char* pSection, std::vector<std::string>& outList);

    // 섹션 존재 여부
    BOOL HasSection(const char* pSection);
    BOOL HasSection(int iIndex);

    // 파일 경로 가져오기
    const std::string& GetFilePath() const { return m_FilePath; }
    
    // 로드 여부
    BOOL IsLoaded() const { return m_bLoaded; }

private:
    // 내부 헬퍼 함수
    void LoadCache();
    std::string ConvertToMultiByte(const wchar_t* wstr);
    std::wstring ConvertToWideChar(const char* str);
};

//---------------------------------------------------------------------------
