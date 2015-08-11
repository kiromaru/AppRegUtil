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
    dwValueType = REG_DWORD;
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

    switch (dwType)
    {
        case REG_DWORD:
        {
            DWORD dwValue;
            key.QueryDWORDValue(pszValueName, dwValue);

            wcout << L"Type: REG_DWORD, Value: " << dwValue << endl;
        }
        break;

        case REG_SZ:
        {
            CString strValue;
            ULONG chars = 1024;
            key.QueryStringValue(pszValueName, strValue.GetBuffer(1024), &chars);
            strValue.ReleaseBuffer();

            wcout << L"Type: REG_SZ, Value: '" << strValue << L"'" << endl;
        }
        break;

        case REG_QWORD:
        {
            ULONGLONG ullValue;
            key.QueryQWORDValue(pszValueName, ullValue);

            wcout << L"Type: REQ_QWORD, Value: " << ullValue << endl;
        }
        break;
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
        // Enumerate values
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

    status = RegCopyTreeW(key.m_hKey, nullptr, subAppKey.m_hKey);
    if (ERROR_SUCCESS != status)
    {
        wcout << L"Failed to copy tree, result: " << status << endl;
        ::RegCloseKey(hAppKey);
        return;
    }

    wcout << L"Tree copied successfully." << endl;

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
        break;

    case Commands::Copy:
        ExecuteCopy(fileName, keyPath, keyRoot);
        break;

    default:
        wcout << L"Unknown command: " << command << endl;
    }
}

