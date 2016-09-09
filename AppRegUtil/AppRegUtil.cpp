// AppRegUtil.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>

using namespace std;

enum Commands
{
    Get,
    Set,
    Copy
};


void Usage(LPCSTR szAppName)
{
    LPSTR szFileName = ::PathFindFileNameA(szAppName);

    cout << "Usage of " << szFileName << endl << endl;
    cout << szFileName << " get  FileName KeyPath [ValueName]" << endl;
    cout << szFileName << " set  FileName KeyPath ValueName Value [ValueType]" << endl;
    cout << szFileName << " copy FileName KeyRoot KeyPath" << endl;
    cout << endl;
    cout << "Where:" << endl;
    cout << "FileName  is the path to the file that contains the local App registry hive." << endl;
    cout << "KeyPath   is the path to the key where the value resides or is going to be set." << endl;
    cout << "ValueName is the name of the value that is being queried or that is being set." << endl;
    cout << "Value     is the value that is going to be set." << endl;
    cout << "ValueType is the type of the value that is going to be set. If not specified," << endl;
    cout << "          the utility will try to find the most appropriate type for the given Value." << endl;
    cout << "KeyRoot   is HKCU|HKLM|HKCR" << endl;
    cout << endl;
    cout << "For " << szFileName << " get: if ValueName is not specified, will get default value." << endl;
    cout << "If ValueName is '*', will list all values under the given KeyPath." << endl;
    cout << endl;
    cout << "For " << szFileName << " copy: All keys under KeyRoot\\KeyPath will be copied to the" << endl;
    cout << "local App registry hive in FileName." << endl;
}

bool ValidateParameters(int argc, char** argv, Commands& command, CString& fileName, CString& keyPath, CString& valueName, CString& value, DWORD& dwValueType, HKEY& keyRoot) 
{
    command = Commands::Get;
    fileName = L"";
    keyPath = L"";
    valueName = L"";
    value = L"";
    dwValueType = REG_SZ; // String by default
    keyRoot = HKEY_CURRENT_USER;

    // Need at least command and FileName
    if (argc < 2)
    {
        return false;
    }

    // Command is first
    if (StrCmpIA("get", argv[0]) == 0)
    {
        command = Commands::Get;
    }
    else if (StrCmpIA("set", argv[0]) == 0)
    {
        command = Commands::Set;
    }
    else if (StrCmpIA("copy", argv[0]) == 0)
    {
        command = Commands::Copy;
    }
    else
    {
        // Not a recognized command.
        return false;
    }

    // Second is filename. Allow anything and allow it to fail when opening.
    fileName = argv[1];

    if (command == Commands::Get)
    {
        // If command is Get, we need KeyPath and an optional ValueName
        if (argc < 3)
        {
            return false;
        }

        keyPath = argv[2];

        if (argc >= 4)
        {
            // ValueName was provided.
            valueName = argv[3];
        }
        else
        {
            valueName = L"";
        }
    }
    else if (command == Commands::Set)
    {
        // If command is Set, we need a KeyPath, a ValueName, a Value and an optional ValueType.
        if (argc < 5)
        {
            return false;
        }

        keyPath   = argv[2];
        valueName = argv[3];
        value     = argv[4];

        if (argc >= 6)
        {
            // Value type.
            if (StrCmpIA(argv[5], "REG_BINARY") == 0)
            {
                dwValueType = REG_BINARY;
            }
            else if (StrCmpIA(argv[5], "REG_DWORD") == 0)
            {
                dwValueType = REG_DWORD;
            }
            else if (StrCmpIA(argv[5], "REG_QWORD") == 0)
            {
                dwValueType = REG_QWORD;
            }
            else if (StrCmpIA(argv[5], "REG_SZ") == 0)
            {
                dwValueType = REG_SZ;
            }
            else
            {
                return false;
            }
        }
    }
    else if (command == Commands::Copy)
    {
        // If command is copy, we need a KeyRoot and KeyPath
        if (argc < 4)
        {
            return false;
        }

        keyPath = argv[3];

        if (StrCmpIA(argv[2], "HKCU") == 0)
        {
            keyRoot = HKEY_CURRENT_USER;
        }
        else if (StrCmpIA(argv[2], "HKLM") == 0)
        {
            keyRoot = HKEY_LOCAL_MACHINE;
        }
        else if (StrCmpIA(argv[2], "HKCR") == 0)
        {
            keyRoot = HKEY_CLASSES_ROOT;
        }
        else
        {
            return false;
        }
    }
    else
    {
        // What happened?
        return false;
    }

    return true;
}

void GetSpecificBinaryValue(HKEY hKey, LPCWSTR pszValueName)
{
    CRegKey key;
    LSTATUS status = key.Open(hKey, nullptr, KEY_READ);
    if (ERROR_SUCCESS != status)
    {
        wcout << L"Failed to re-open key for Get, result: " << status << endl;
        return;
    }

    DWORD dwDataSize = 0;
    status = ::RegGetValueW(hKey, L"", pszValueName, RRF_RT_REG_BINARY, nullptr, nullptr, &dwDataSize);

    if (ERROR_SUCCESS != status)
    {
        wcout << L"Failed to get size of binary data, result: " << status << endl;
        return;
    }

    ULONG ulCount = dwDataSize;
    BYTE* pBuffer = new BYTE[dwDataSize];
    key.QueryBinaryValue(pszValueName, pBuffer, &ulCount);

    wcout << L"REG_BINARY, Byte count: " << ulCount << L", Value: ";

    for (ULONG i = 0; i < ulCount; i++)
    {
        wcout << std::hex << pBuffer[i];
    }

    wcout << endl;

    delete[] pBuffer;
}

void GetSpecificValue(HKEY hKey, LPCWSTR pszValueName)
{
    DWORD dwType;
    LSTATUS status = ::RegQueryValueExW(hKey, pszValueName, nullptr, &dwType, nullptr, nullptr);
    if (ERROR_SUCCESS != status)
    {
        wcout << L"Failed to get type of value: " << pszValueName << L", result: " << status << endl;
        return;
    }

    CRegKey key;
    status = key.Open(hKey, nullptr, KEY_READ);
    if (ERROR_SUCCESS != status)
    {
        wcout << L"Failed to re-open key for Get, result: " << status << endl;
        return;
    }

    if (StrCmp(pszValueName, L"") == 0)
    {
        wcout << L"(default), ";
    }
    else
    {
        wcout << pszValueName << L", ";
    }

    switch (dwType)
    {
        case REG_DWORD:
        {
            DWORD dwValue;
            key.QueryDWORDValue(pszValueName, dwValue);

            wcout << L"REG_DWORD: " << dwValue << endl;
        }
        break;

        case REG_SZ:
        {
            CString strValue;
            ULONG chars = 1024;
            key.QueryStringValue(pszValueName, strValue.GetBuffer(1024), &chars);
            strValue.ReleaseBuffer();

            wcout << L"REG_SZ: '" << strValue.GetBuffer() << L"'" << endl;
        }
        break;

        case REG_QWORD:
        {
            ULONGLONG ullValue;
            key.QueryQWORDValue(pszValueName, ullValue);

            wcout << L"REG_QWORD: " << ullValue << endl;
        }
        break;

        case REG_BINARY:
        {
            GetSpecificBinaryValue(hKey, pszValueName);
        }
        break;

        default:
            wcout << L"Unrecognized value type: " << dwType << endl;
    }
}

void ExecuteGet(LPCWSTR pszFileName, LPCWSTR pszKeyPath, LPCWSTR pszValueName)
{
    HKEY hAppKey;
    LSTATUS status = ::RegLoadAppKeyW(pszFileName, &hAppKey, KEY_ALL_ACCESS, 0, 0);
    if (ERROR_SUCCESS != status)
    {
        wcout << L"Failed to RegLoadAppKey: " << pszFileName << L", result: " << status << endl;
        return;
    }

    CRegKey key;
    status = key.Open(hAppKey, pszKeyPath, KEY_READ);
    if (ERROR_SUCCESS != status)
    {
        wcout << L"Failed to open key: " << pszKeyPath << L", result: " << status << endl;
        ::RegCloseKey(hAppKey);
        return;
    }

    if (StrCmp(pszValueName, L"*") == 0)
    {
        // Enumerate keys
        for (int i = 0; ; i++)
        {
            CString strKeyName;
            DWORD dwNameLength = 1024;
            status = key.EnumKey(i, strKeyName.GetBuffer(dwNameLength), &dwNameLength);
            strKeyName.ReleaseBuffer();
            if (ERROR_SUCCESS != status)
            {
                break;
            }

            wcout << L"Key: " << strKeyName.GetBuffer() << endl;
        }

        // Enumerate values
        for (int i = 0; ; i++)
        {
            CString strValueName;
            DWORD dwNameLength = 1024;
            status = ::RegEnumValueW(key.m_hKey, i, strValueName.GetBuffer(dwNameLength), &dwNameLength, nullptr, nullptr, nullptr, nullptr);
            strValueName.ReleaseBuffer();
            if (ERROR_SUCCESS != status)
            {
                break;
            }

            GetSpecificValue(key.m_hKey, strValueName);
        }
    }
    else
    {
        // Get specific value
        GetSpecificValue(key.m_hKey, pszValueName);
    }

    ::RegCloseKey(hAppKey);
}

void ExecuteSet(LPCWSTR pszFileName, LPCWSTR pszKeyPath, LPCWSTR pszValueName, LPCWSTR pszValue, DWORD dwValueType)
{
    HKEY hAppKey;
    LSTATUS status = ::RegLoadAppKeyW(pszFileName, &hAppKey, KEY_ALL_ACCESS, 0, 0);
    if (ERROR_SUCCESS != status)
    {
        wcout << L"Failed to RegLoadAppKey: " << pszFileName << L", result: " << status << endl;
        return;
    }

    CRegKey key;
    status = key.Open(hAppKey, pszKeyPath, KEY_READ | KEY_WRITE);
    if (ERROR_SUCCESS != status)
    {
        wcout << L"Failed to open key: " << pszKeyPath << L", result: " << status << endl;
        ::RegCloseKey(hAppKey);
        return;
    }

    int iValue;
    ULONGLONG ullValue;

    switch (dwValueType)
    {
    case REG_DWORD:
        iValue = _wtoi(pszValue);
        status = key.SetDWORDValue(pszValueName, iValue);
        break;

    case REG_QWORD:
        ullValue = _wtoi64(pszValue);
        status = key.SetQWORDValue(pszValueName, ullValue);
        break;

    case REG_BINARY:
        wcout << L"Do not support setting binary for the moment." << endl;
        status = ERROR_ACCESS_DENIED;
        break;

    case REG_SZ:
        status = key.SetStringValue(pszValueName, pszValue, dwValueType);
        break;

    default:
        wcout << L"Unrecognized value type: " << dwValueType << "." << endl;
        return;
    }

    if (ERROR_SUCCESS != status)
    {
        wcout << L"Failed to set value. Key: " << pszKeyPath << L", Value name: " << pszValueName << L", result: " << status << endl;
    }

    ::RegCloseKey(hAppKey);
}

bool CopyKey(HKEY hKeySrc, HKEY hKeyDest)
{
    bool hadErrors = false;

    CRegKey srcKey(hKeySrc);
    CRegKey destKey(hKeyDest);

    for (int i = 0; ; i++)
    {
        CString strKeyName;
        DWORD dwKeyNameLength = 256;

        LSTATUS status = srcKey.EnumKey(i, strKeyName.GetBuffer(dwKeyNameLength), &dwKeyNameLength);
        strKeyName.ReleaseBuffer();

        if (ERROR_SUCCESS == status)
        {
            CRegKey subKey;
            status = subKey.Open(srcKey.m_hKey, strKeyName, KEY_READ);

            if (ERROR_SUCCESS == status)
            {
                CRegKey newSubKey;
                status = newSubKey.Create(destKey.m_hKey, strKeyName, nullptr, 0, KEY_ALL_ACCESS);

                if (ERROR_SUCCESS == status)
                {
                    hadErrors |= !CopyKey(subKey.m_hKey, newSubKey.m_hKey);
                }
                else
                {
                    wcout << L"Failed to create new subkey: " << strKeyName.GetBuffer() << endl;
                    hadErrors = true;
                }
            }
            else
            {
                wcout << L"Failed to open subkeyL " << strKeyName.GetBuffer() << endl;
                hadErrors = true;
            }
        }
        else
        {
            break;
        }
    }

    // Once we have enumerated all keys, copy the values
    // Enumerate values
    for (int i = 0; ; i++)
    {
        CString strValueName;
        DWORD dwNameLength = 1024;
        LSTATUS status = ::RegEnumValueW(hKeySrc, i, strValueName.GetBuffer(dwNameLength), &dwNameLength, nullptr, nullptr, nullptr, nullptr);
        strValueName.ReleaseBuffer();
        if (ERROR_SUCCESS != status)
        {
            break;
        }

        DWORD dwType;
        ULONG ulCount;
        BYTE* pData;
        if (ERROR_SUCCESS != ::RegQueryValueExW(srcKey.m_hKey, strValueName, nullptr, nullptr, nullptr, &ulCount))
        {
            wcout << "Failed to get size of value: " << strValueName.GetBuffer() << endl;
            hadErrors = true;
            continue;
        }

        pData = new BYTE[ulCount];
        status = srcKey.QueryValue(strValueName, &dwType, pData, &ulCount);

        if (ERROR_SUCCESS != status)
        {
            wcout << L"Failed to get value: " << strValueName.GetBuffer() << endl;
            hadErrors = true;
            delete[] pData;
            continue;
        }

        status = destKey.SetValue(strValueName, dwType, pData, ulCount);
        delete[] pData;

        if (ERROR_SUCCESS != status)
        {
            wcout << L"Failed to set value: " << strValueName.GetBuffer() << endl;
            hadErrors = true;
        }
    }

    return !hadErrors;
}

void ExecuteCopy(LPCWSTR pszFileName, LPCWSTR pszKeyPath, HKEY keyRoot)
{
    HKEY hAppKey;
    LSTATUS status = ::RegLoadAppKeyW(pszFileName, &hAppKey, KEY_ALL_ACCESS, 0, 0);

    if (ERROR_SUCCESS != status)
    {
        wcout << L"Failed to RegLoadAppKey: " << pszFileName << L", result: " << status << endl;
        return;
    }

    CRegKey key;
    status = key.Open(keyRoot, pszKeyPath, KEY_ALL_ACCESS);
    if (ERROR_SUCCESS != status)
    {
        wcout << L"Failed to open path: " << pszKeyPath << L", result: " << status << endl;
        ::RegCloseKey(hAppKey);
        return;
    }

    CRegKey subAppKey;
    status = subAppKey.Create(hAppKey, pszKeyPath, nullptr, 0, KEY_ALL_ACCESS);
    if (ERROR_SUCCESS != status)
    {
        wcout << L"Failed to create sub key under App registry: " << pszKeyPath << ", result: " << status << endl;
        ::RegCloseKey(hAppKey);
        return;
    }

    if (CopyKey(key.m_hKey, subAppKey.m_hKey))
    {
        wcout << L"Tree copied successfully." << endl;
    }
    else
    {
        wcout << L"Errors found copying tree." << endl;
    }

    ::RegCloseKey(hAppKey);
}

int main(int argc, char** argv)
{
    Commands command;
    CString fileName;
    CString keyPath;
    CString valueName;
    CString value;
    HKEY keyRoot;
    DWORD dwValueType;

    if (!ValidateParameters(argc - 1, &argv[1], command, fileName, keyPath, valueName, value, dwValueType, keyRoot))
    {
        Usage(argv[0]);
        return -1;
    }

    switch (command)
    {
    case Commands::Get:
        ExecuteGet(fileName, keyPath, valueName);
        break;

    case Commands::Set:
        ExecuteSet(fileName, keyPath, valueName, value, dwValueType);
        break;

    case Commands::Copy:
        ExecuteCopy(fileName, keyPath, keyRoot);
        break;

    default:
        wcout << L"Unknown command: " << command << endl;
    }
}

