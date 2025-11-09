//-----------------------------------------------------------------------------
// ZInitFile.cpp - Windows INI 파일 관리 구현
//-----------------------------------------------------------------------------

#include "ZInitFile.h"
#include <sstream>
#include <fstream>
#include <locale>
#include <codecvt>
#include <iostream>  // For debugging

// ZString과 ZStringList는 프로젝트에 이미 존재한다고 가정
// 여기서는 인터페이스만 사용

//-----------------------------------------------------------------------------

ZInitFile::ZInitFile()
{
    m_FilePath = "";
    m_bLoaded = FALSE;
}

//-----------------------------------------------------------------------------

ZInitFile::~ZInitFile()
{
    Clear();
}

//-----------------------------------------------------------------------------

BOOL ZInitFile::LoadIFT(const char* pFilePath)
{
    if (!pFilePath || strlen(pFilePath) == 0)
        return FALSE;

    m_FilePath = pFilePath;
    
    // 파일 존재 여부 확인
    DWORD dwAttrib = GetFileAttributesA(pFilePath);
    if (dwAttrib == INVALID_FILE_ATTRIBUTES || (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        m_bLoaded = FALSE;
        return FALSE;
    }

    // 캐시 로드
    LoadCache();
    
    m_bLoaded = TRUE;
    return TRUE;
}

//-----------------------------------------------------------------------------

BOOL ZInitFile::LoadIFT(const std::string& filePath)
{
    return LoadIFT(filePath.c_str());
}

//-----------------------------------------------------------------------------

void ZInitFile::Clear()
{
    m_Cache.clear();
    m_FilePath.clear();
    m_bLoaded = FALSE;
}

//-----------------------------------------------------------------------------

std::string ZInitFile::GetValue(const char* pSection, const char* pKey, const char* pDefault)
{
    if (!m_bLoaded)
        return std::string(pDefault);

    // 캐시에서 검색 (LoadCache에서 모두 로드됨)
    std::string section(pSection);
    std::string key(pKey);
    
    if (m_Cache.find(section) != m_Cache.end())
    {
        if (m_Cache[section].find(key) != m_Cache[section].end())
        {
            return m_Cache[section][key];
        }
    }

    // 캐시에 없으면 기본값 반환
    return std::string(pDefault);
}

//-----------------------------------------------------------------------------

std::string ZInitFile::GetValue(const std::string& section, const std::string& key, const std::string& defaultValue)
{
    return GetValue(section.c_str(), key.c_str(), defaultValue.c_str());
}

//-----------------------------------------------------------------------------

std::string ZInitFile::GetValue(int iIndex, const char* pKey, const char* pDefault)
{
    char szSection[32];
    sprintf_s(szSection, sizeof(szSection), "%d", iIndex);
    return GetValue(szSection, pKey, pDefault);
}

//-----------------------------------------------------------------------------

std::string ZInitFile::GetValue(int iIndex, const std::string& key, const std::string& defaultValue)
{
    return GetValue(iIndex, key.c_str(), defaultValue.c_str());
}

//-----------------------------------------------------------------------------

BOOL ZInitFile::SetValue(const char* pSection, const char* pKey, const char* pValue)
{
    if (!m_bLoaded)
        return FALSE;

    // 캐시 업데이트
    m_Cache[std::string(pSection)][std::string(pKey)] = pValue;
    
    // UTF-8 파일로 저장
    std::ofstream file(m_FilePath, std::ios::out | std::ios::trunc);
    if (!file.is_open())
        return FALSE;
    
    // 모든 섹션과 키-값 쌍을 파일에 쓰기
    for (auto& sectionPair : m_Cache)
    {
        file << "[" << sectionPair.first << "]" << std::endl;
        
        for (auto& keyValuePair : sectionPair.second)
        {
            file << keyValuePair.first << "=" << keyValuePair.second << std::endl;
        }
        
        file << std::endl;  // 섹션 간 빈 줄
    }
    
    file.close();
    return TRUE;
}

//-----------------------------------------------------------------------------

BOOL ZInitFile::SetValue(const std::string& section, const std::string& key, const std::string& value)
{
    return SetValue(section.c_str(), key.c_str(), value.c_str());
}

//-----------------------------------------------------------------------------

BOOL ZInitFile::SetValue(int iIndex, const char* pKey, const char* pValue)
{
    char szSection[32];
    sprintf_s(szSection, sizeof(szSection), "%d", iIndex);
    return SetValue(szSection, pKey, pValue);
}

//-----------------------------------------------------------------------------

BOOL ZInitFile::GetSectionList(std::vector<std::string>& outList)
{
    return GetTitleList(outList);
}

//-----------------------------------------------------------------------------

BOOL ZInitFile::GetTitleList(std::vector<std::string>& outList)
{
    if (!m_bLoaded)
        return FALSE;

    outList.clear();

    // 캐시에서 모든 섹션명 가져오기
    for (auto& sectionPair : m_Cache)
    {
        outList.push_back(sectionPair.first);
    }

    return TRUE;
}

//-----------------------------------------------------------------------------

BOOL ZInitFile::GetKeyList(const char* pSection, std::vector<std::string>& outList)
{
    if (!m_bLoaded)
        return FALSE;

    outList.clear();

    // 캐시에서 해당 섹션의 모든 키 가져오기
    std::string section(pSection);
    
    if (m_Cache.find(section) != m_Cache.end())
    {
        for (auto& keyValuePair : m_Cache[section])
        {
            outList.push_back(keyValuePair.first);
        }
        return TRUE;
    }

    return FALSE;
}

//-----------------------------------------------------------------------------

BOOL ZInitFile::HasSection(const char* pSection)
{
    if (!m_bLoaded)
        return FALSE;

    // 캐시에서 섹션 존재 여부 확인
    std::string section(pSection);
    return (m_Cache.find(section) != m_Cache.end());
}

//-----------------------------------------------------------------------------

BOOL ZInitFile::HasSection(int iIndex)
{
    char szSection[32];
    sprintf_s(szSection, sizeof(szSection), "%d", iIndex);
    return HasSection(szSection);
}

//-----------------------------------------------------------------------------
// Private 헬퍼 함수들
//-----------------------------------------------------------------------------

void ZInitFile::LoadCache()
{
    m_Cache.clear();

    if (m_FilePath.empty())
        return;

    // Windows 콘솔을 UTF-8 모드로 설정 (한글 출력용)
    #ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    #endif

    // UTF-8 파일을 직접 읽기
    std::ifstream file(m_FilePath, std::ios::binary);
    if (!file.is_open())
        return;

    std::string currentSection;
    std::string line;
    bool firstLine = true;
    
    while (std::getline(file, line))
    {
        // UTF-8 BOM 제거 (첫 번째 줄에서만)
        if (firstLine && line.size() >= 3 && 
            (unsigned char)line[0] == 0xEF && 
            (unsigned char)line[1] == 0xBB && 
            (unsigned char)line[2] == 0xBF)
        {
            line = line.substr(3);
        }
        firstLine = false;
        
        // 공백 및 개행 문자 제거 (안전하게)
        size_t startPos = line.find_first_not_of(" \t\r\n");
        if (startPos == std::string::npos)
        {
            // 빈 줄
            continue;
        }
        
        size_t endPos = line.find_last_not_of(" \t\r\n");
        line = line.substr(startPos, endPos - startPos + 1);
        
        // 빈 줄이나 주석 무시
        if (line.empty() || line[0] == ';' || line[0] == '#')
            continue;
        
        // 섹션 헤더 확인 [SectionName]
        if (line[0] == '[' && line.length() > 1 && line[line.length() - 1] == ']')
        {
            currentSection = line.substr(1, line.length() - 2);
            // 섹션 이름의 공백 제거
            size_t secStart = currentSection.find_first_not_of(" \t");
            size_t secEnd = currentSection.find_last_not_of(" \t");
            if (secStart != std::string::npos && secEnd != std::string::npos)
            {
                currentSection = currentSection.substr(secStart, secEnd - secStart + 1);
            }
            continue;
        }
        
        // 키:값 쌍 파싱
        size_t equalPos = line.find(':');
        if (equalPos != std::string::npos && !currentSection.empty())
        {
            std::string key = line.substr(0, equalPos);
            // ':' 다음부터 끝까지 가져오기 (substr은 자동으로 범위 처리)
            std::string value;
            if (equalPos + 1 < line.length())
            {
                value = line.substr(equalPos + 1);
            }
            else
            {
                value = "";  // ':' 뒤에 아무것도 없음
            }
            
            // 키의 앞뒤 공백 제거
            size_t keyStart = key.find_first_not_of(" \t");
            size_t keyEnd = key.find_last_not_of(" \t");
            if (keyStart != std::string::npos && keyEnd != std::string::npos)
            {
                key = key.substr(keyStart, keyEnd - keyStart + 1);
            }
            
            // 값의 앞뒤 공백 제거
            size_t valStart = value.find_first_not_of(" \t");
            if (valStart != std::string::npos)
            {
                size_t valEnd = value.find_last_not_of(" \t");
                value = value.substr(valStart, valEnd - valStart + 1);
            }
            else
            {
                value = "";  // 값이 비어있음
            }
            
            if (!key.empty())
            {
                m_Cache[currentSection][key] = value;
                // Debug output
                std::cout << "[" << currentSection << "] " << key << " : " << value << std::endl;
            }
        }
    }
    
    file.close();
}

//-----------------------------------------------------------------------------

std::string ZInitFile::ConvertToMultiByte(const wchar_t* wstr)
{
    if (!wstr)
        return "";

    int size = WideCharToMultiByte(CP_ACP, 0, wstr, -1, NULL, 0, NULL, NULL);
    if (size == 0)
        return "";

    char* buffer = new char[size];
    WideCharToMultiByte(CP_ACP, 0, wstr, -1, buffer, size, NULL, NULL);
    
    std::string result(buffer);
    delete[] buffer;
    
    return result;
}

//-----------------------------------------------------------------------------

std::wstring ZInitFile::ConvertToWideChar(const char* str)
{
    if (!str)
        return L"";

    int size = MultiByteToWideChar(CP_ACP, 0, str, -1, NULL, 0);
    if (size == 0)
        return L"";

    wchar_t* buffer = new wchar_t[size];
    MultiByteToWideChar(CP_ACP, 0, str, -1, buffer, size);
    
    std::wstring result(buffer);
    delete[] buffer;
    
    return result;
}

//-----------------------------------------------------------------------------
