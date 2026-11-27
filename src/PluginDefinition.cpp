//this file is part of notepad++
//Original Copyright (C)2022 Don HO <don.h@free.fr>
//Modifications Copyright (C)2026 bsagarzazu
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "PluginDefinition.h"
#include "menuCmdID.h"
#include <shlobj.h>
#include <tchar.h>
#include <shellapi.h>
#include <cstring>
#include <vector>

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

ShortcutKey syslogShortcut = { false, true, true, 'L' };
ShortcutKey highlightPrefixShortcut = { false, true, true, 'P' };
ShortcutKey folderShortcut = { false, true, true, 'F' };
ShortcutKey rootShortcut = { false, true, true, 'R' };
ShortcutKey cleanShortcut = { false, true, true, 'C' };

TCHAR szIniFilePath[MAX_PATH] = {0};
TCHAR szRootPath[MAX_PATH] = {0};
TCHAR szSyslogPath[MAX_PATH] = {0};
TCHAR szSyslogFilePattern[MAX_PATH] = {0};
TCHAR szCustomPrefix[MAX_PATH] = {0};
TCHAR szCleanupFilePattern[MAX_PATH] = {0};
TCHAR szThreshold[MAX_PATH] = {0};
TCHAR szDebugColor[MAX_PATH] = {0};
TCHAR szInfoColor[MAX_PATH] = {0};
TCHAR szWarningColor[MAX_PATH] = {0};
TCHAR szErrorColor[MAX_PATH] = {0};
TCHAR szOtherColor[MAX_PATH] = {0};

TCHAR szIniSection[] = TEXT("Settings");
TCHAR szKeyRootPath[] = TEXT("LogsRootPath");
TCHAR szKeySyslogPath[] = TEXT("SyslogPath");
TCHAR szKeySyslogFilePattern[] = TEXT("SyslogFilePattern");
TCHAR szKeyCustomPrefix[] = TEXT("CustomPrefix");
TCHAR szKeyCleanupFilePattern[] = TEXT("CleanupFilePattern");
TCHAR szKeyThreshold[] = TEXT("CleanupThreshold");
TCHAR szKeyDebugColor[] = TEXT("DebugColor");
TCHAR szKeyInfoColor[] = TEXT("InfoColor");
TCHAR szKeyWarningColor[] = TEXT("WarningColor");
TCHAR szKeyErrorColor[] = TEXT("ErrorColor");
TCHAR szKeyOtherColor[] = TEXT("OtherColor");

void loadConfig() 
{
    if (szIniFilePath[0] == 0) {
        ::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)szIniFilePath);
        CreateDirectory(szIniFilePath, NULL);
        _tcscat_s(szIniFilePath, MAX_PATH, TEXT("\\TCSyslogFinder.ini"));
    }
    
    GetPrivateProfileString(szIniSection, szKeyRootPath, TEXT(""), szRootPath, MAX_PATH, szIniFilePath);
    GetPrivateProfileString(szIniSection, szKeySyslogPath, TEXT(""), szSyslogPath, MAX_PATH, szIniFilePath);
    GetPrivateProfileString(szIniSection, szKeySyslogFilePattern, TEXT("tcserver*.syslog"), szSyslogFilePattern, MAX_PATH, szIniFilePath);
    GetPrivateProfileString(szIniSection, szKeyCustomPrefix, TEXT(""), szCustomPrefix, MAX_PATH, szIniFilePath);
    GetPrivateProfileString(szIniSection, szKeyCleanupFilePattern, TEXT("tcserver*.syslog"), szCleanupFilePattern, MAX_PATH, szIniFilePath);
    GetPrivateProfileString(szIniSection, szKeyThreshold, TEXT("7"), szThreshold, MAX_PATH, szIniFilePath);
    GetPrivateProfileString(szIniSection, szKeyDebugColor, TEXT("#91A0AE"), szDebugColor, MAX_PATH, szIniFilePath);
    GetPrivateProfileString(szIniSection, szKeyInfoColor, TEXT("#7FAFD4"), szInfoColor, MAX_PATH, szIniFilePath);
    GetPrivateProfileString(szIniSection, szKeyWarningColor, TEXT("#D6B66C"), szWarningColor, MAX_PATH, szIniFilePath);
    GetPrivateProfileString(szIniSection, szKeyErrorColor, TEXT("#D17C7C"), szErrorColor, MAX_PATH, szIniFilePath);
    GetPrivateProfileString(szIniSection, szKeyOtherColor, TEXT("#A6A6A6"), szOtherColor, MAX_PATH, szIniFilePath);

    if (GetFileAttributes(szIniFilePath) == INVALID_FILE_ATTRIBUTES) {
        TCHAR szEnvValue[MAX_PATH] = {0};

        if (GetEnvironmentVariable(TEXT("SIEMENS_LOGGING_ROOT"), szEnvValue, MAX_PATH) > 0)
        {
            _tcscpy_s(szRootPath, MAX_PATH, szEnvValue);

            TCHAR szGuessed[MAX_PATH];
            _stprintf_s(szGuessed, MAX_PATH, TEXT("%s\\Teamcenter\\ServerManagers\\TcCluster\\PoolA\\TcServer"), szRootPath);

            DWORD dwAttributes = GetFileAttributes(szGuessed);
            if (dwAttributes != INVALID_FILE_ATTRIBUTES && (dwAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {
                _tcscpy_s(szSyslogPath, MAX_PATH, szGuessed);
            }
        }

        saveConfigValue(szKeyRootPath, szRootPath);
        saveConfigValue(szKeySyslogPath, szSyslogPath);
        saveConfigValue(szKeySyslogFilePattern, szSyslogFilePattern);
        saveConfigValue(szKeyCustomPrefix, szCustomPrefix);
        saveConfigValue(szKeyCleanupFilePattern, szCleanupFilePattern);
        saveConfigValue(szKeyThreshold, szThreshold);
        saveConfigValue(szKeyDebugColor, szDebugColor);
        saveConfigValue(szKeyInfoColor, szInfoColor);
        saveConfigValue(szKeyWarningColor, szWarningColor);
        saveConfigValue(szKeyErrorColor, szErrorColor);
        saveConfigValue(szKeyOtherColor, szOtherColor);
    }
}

void saveConfigValue(const TCHAR* szKey, const TCHAR* szValue) 
{
    WritePrivateProfileString(szIniSection, szKey, szValue, szIniFilePath);
}

HWND getCurrentScintilla()
{
    int which = -1;
    ::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
    if (which == -1) return NULL;

    return (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;
}

void CALLBACK timerToScroll(HWND /*hWnd*/, UINT /*uMsg*/, UINT_PTR idEvent, DWORD /*dwTime*/)
{
    ::KillTimer(NULL, idEvent);

    HWND hCurScintilla = getCurrentScintilla();
    if (!hCurScintilla) return;

    // Track the new content
    ::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_VIEW_MONITORING);

    // Go to the end of the file
    int nLines = (int)::SendMessage(hCurScintilla, SCI_GETLINECOUNT, 0, 0);
    int nLastLine = nLines - 1;

    if (nLastLine >= 0)
    {
        ::SendMessage(hCurScintilla, SCI_ENSUREVISIBLE, nLastLine, 0);
        ::SendMessage(hCurScintilla, SCI_GOTOLINE, nLastLine, 0);
        ::SendMessage(hCurScintilla, SCI_SCROLLCARET, 0, 0);
    }
}

enum class LogLevel
{
    Debug,
    Info,
    Warning,
    Error,
    Other
};

const int logLevelCount = 5;
int logLevelIndicatorIds[logLevelCount] = { -1, -1, -1, -1, -1 };

int hexDigitValue(TCHAR ch)
{
    if (ch >= TEXT('0') && ch <= TEXT('9'))
        return ch - TEXT('0');
    if (ch >= TEXT('A') && ch <= TEXT('F'))
        return ch - TEXT('A') + 10;
    if (ch >= TEXT('a') && ch <= TEXT('f'))
        return ch - TEXT('a') + 10;
    return -1;
}

bool parseRgbColor(const TCHAR* text, COLORREF* color)
{
    if (!text || _tcslen(text) != 7 || text[0] != TEXT('#'))
        return false;

    int digits[6];
    for (int i = 0; i < 6; ++i)
    {
        digits[i] = hexDigitValue(text[i + 1]);
        if (digits[i] < 0)
            return false;
    }

    *color = RGB(digits[0] * 16 + digits[1], digits[2] * 16 + digits[3], digits[4] * 16 + digits[5]);

    return true;
}

bool isLevelWordCharacter(char ch)
{
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_';
}

bool containsLevelToken(const char* line, size_t lineLength, const char* token)
{
    size_t tokenLength = strlen(token);
    if (tokenLength > lineLength)
        return false;

    for (size_t i = 0; i + tokenLength <= lineLength; ++i)
    {
        if (i > 0 && isLevelWordCharacter(line[i - 1]))
            continue;
        if (i + tokenLength < lineLength && isLevelWordCharacter(line[i + tokenLength]))
            continue;

        size_t j = 0;
        while (j < tokenLength)
        {
            char ch = line[i + j];
            if (ch >= 'a' && ch <= 'z')
                ch = (char)(ch - 'a' + 'A');
            if (ch != token[j])
                break;
            ++j;
        }

        if (j == tokenLength)
            return true;
    }

    return false;
}

LogLevel classifyLogLevel(const char* line, size_t lineLength)
{
    if (containsLevelToken(line, lineLength, "ERROR") || containsLevelToken(line, lineLength, "FATAL") || containsLevelToken(line, lineLength, "CRITICAL"))
        return LogLevel::Error;

    if (containsLevelToken(line, lineLength, "WARNING") || containsLevelToken(line, lineLength, "WARN"))
        return LogLevel::Warning;

    if (containsLevelToken(line, lineLength, "INFO"))
        return LogLevel::Info;

    if (containsLevelToken(line, lineLength, "DEBUG") || containsLevelToken(line, lineLength, "TRACE"))
        return LogLevel::Debug;

    return LogLevel::Other;
}

void markPrefixLinesByLevel(const TCHAR* customPrefix)
{
    if (!customPrefix || customPrefix[0] == 0)
        return;

    HWND curScintilla = getCurrentScintilla();
    if (!curScintilla)
        return;

    COLORREF colors[logLevelCount];
    const TCHAR* colorValues[logLevelCount] = {
        szDebugColor,
        szInfoColor,
        szWarningColor,
        szErrorColor,
        szOtherColor
    };

    for (int i = 0; i < logLevelCount; ++i)
    {
        if (!parseRgbColor(colorValues[i], &colors[i]))
        {
            ::MessageBox(nppData._nppHandle, TEXT("A lint color is invalid. Use the #RRGGBB format in the settings."), TEXT("TC Syslog Finder"), MB_OK | MB_ICONWARNING);
            return;
        }
    }

    // Reserve plugin-owned indicators once instead of assuming an ID is free.
    if (logLevelIndicatorIds[0] < 0)
    {
        int firstIndicatorId = 0;
        if (!::SendMessage(nppData._nppHandle, NPPM_ALLOCATEINDICATOR, logLevelCount, (LPARAM)&firstIndicatorId))
        {
            ::MessageBox(nppData._nppHandle, TEXT("Unable to allocate indicators for log highlighting."), TEXT("TC Syslog Finder"), MB_OK | MB_ICONWARNING);
            return;
        }

        for (int i = 0; i < logLevelCount; ++i)
            logLevelIndicatorIds[i] = firstIndicatorId + i;
    }

    // Scintilla searches UTF-8 bytes; the configured prefix is TCHAR text.
    int utf8BufferLength = ::WideCharToMultiByte(CP_UTF8, 0, customPrefix, -1, NULL, 0, NULL, NULL);
    if (utf8BufferLength <= 1)
        return;

    std::vector<char> prefixUtf8((size_t)utf8BufferLength);
    if (::WideCharToMultiByte(CP_UTF8, 0, customPrefix, -1, &prefixUtf8[0], utf8BufferLength, NULL, NULL) != utf8BufferLength)
    {
        ::MessageBox(nppData._nppHandle, TEXT("Unable to convert the configured prefix to UTF-8."), TEXT("TC Syslog Finder"), MB_OK | MB_ICONWARNING);
        return;
    }

    const int prefixByteLength = utf8BufferLength - 1;
    const int docLength = (int)::SendMessage(curScintilla, SCI_GETLENGTH, 0, 0);
    const int lineCount = (int)::SendMessage(curScintilla, SCI_GETLINECOUNT, 0, 0);

    int previousIndicatorId = (int)::SendMessage(curScintilla, SCI_GETINDICATORCURRENT, 0, 0);
    int previousSearchFlags = (int)::SendMessage(curScintilla, SCI_GETSEARCHFLAGS, 0, 0);
    int previousTargetStart = (int)::SendMessage(curScintilla, SCI_GETTARGETSTART, 0, 0);
    int previousTargetEnd = (int)::SendMessage(curScintilla, SCI_GETTARGETEND, 0, 0);

    // Configure the indicators and remove this plugin's previous highlights.
    for (int i = 0; i < logLevelCount; ++i)
    {
        ::SendMessage(curScintilla, SCI_INDICSETSTYLE, logLevelIndicatorIds[i], INDIC_FULLBOX);
        ::SendMessage(curScintilla, SCI_INDICSETFORE, logLevelIndicatorIds[i], colors[i]);
        ::SendMessage(curScintilla, SCI_INDICSETALPHA, logLevelIndicatorIds[i], 55);

        ::SendMessage(curScintilla, SCI_SETINDICATORCURRENT, logLevelIndicatorIds[i], 0);
        ::SendMessage(curScintilla, SCI_INDICATORCLEARRANGE, 0, docLength);
    }

    // Without SCFIND_MATCHCASE, the prefix search is case-insensitive.
    ::SendMessage(curScintilla, SCI_SETSEARCHFLAGS, SCFIND_NONE, 0);

    for (int lineNumber = 0; lineNumber < lineCount; ++lineNumber)
    {
        int lineStart = (int)::SendMessage(curScintilla, SCI_POSITIONFROMLINE, lineNumber, 0);
        int lineEnd = (int)::SendMessage(curScintilla, SCI_GETLINEENDPOSITION, lineNumber, 0);

        ::SendMessage(curScintilla, SCI_SETTARGETRANGE, lineStart, lineEnd);
        int foundPos = (int)::SendMessage(curScintilla, SCI_SEARCHINTARGET, prefixByteLength, (LPARAM)&prefixUtf8[0]);

        if (foundPos < 0)
            continue;

        int lineBufferLength = (int)::SendMessage(curScintilla, SCI_LINELENGTH, lineNumber, 0);
        std::vector<char> lineText((size_t)lineBufferLength + 1, '\0');
        int bytesRead = (int)::SendMessage(curScintilla, SCI_GETLINE, lineNumber, (LPARAM)&lineText[0]);

        LogLevel level = classifyLogLevel(&lineText[0], (size_t)bytesRead);
        int indicatorId = logLevelIndicatorIds[(int)level];

        ::SendMessage(curScintilla, SCI_SETINDICATORCURRENT, indicatorId, 0);
        ::SendMessage(curScintilla, SCI_INDICATORFILLRANGE, lineStart, lineEnd - lineStart);
    }

    // Leave Scintilla's search settings as they were before highlighting.
    ::SendMessage(curScintilla, SCI_SETSEARCHFLAGS, previousSearchFlags, 0);
    ::SendMessage(curScintilla, SCI_SETTARGETRANGE, previousTargetStart, previousTargetEnd);
    ::SendMessage(curScintilla, SCI_SETINDICATORCURRENT, previousIndicatorId, 0);
}

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE /*hModule*/)
{
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{

    //--------------------------------------------//
    //-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
    //--------------------------------------------//
    // with function :
    // setCommand(int index,                      // zero based number to indicate the order of command
    //            TCHAR *commandName,             // the command name that you want to see in plugin menu
    //            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
    //            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
    //            bool check0nInit                // optional. Make this menu item be checked visually
    //            );
    setCommand(0, TEXT("Open Latest TC Syslog"), openLatestSyslog, &syslogShortcut, false);
    setCommand(1, TEXT("Highlight Lines Containing Prefix by Level"), highlightLinesContainingPrefixByLevel, &highlightPrefixShortcut, false);
    setCommand(2, TEXT("---"), NULL, NULL, false);
    setCommand(3, TEXT("Open Syslog Folder"), openSyslogFolder, &folderShortcut, false);
    setCommand(4, TEXT("Open TC Logs Root Folder"), openRootFolder, &rootShortcut, false);
    setCommand(5, TEXT("Clean Old Syslogs"), cleanOldSyslogs, &cleanShortcut, false);
    setCommand(6, TEXT("---"), NULL, NULL, false);
    setCommand(7, TEXT("Set TC Syslog Path"), setSyslogPath, NULL, false);
    setCommand(8, TEXT("Edit Settings"), openSettings, NULL, false);
    setCommand(9, TEXT("---"), NULL, NULL, false);
    setCommand(10, TEXT("GitHub Repository"), openGitHubRepo, NULL, false);
    setCommand(11, TEXT("About TC Syslog Finder"), showAbout, NULL, false);
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
    if (index >= nbFunc)
        return false;

    if (!pFunc)
        return false;

    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;

    return true;
}

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//
void openLatestSyslog()
{
    // Load the latest configuration
    loadConfig();

    // Read TC Syslog Path from settings
    if (szSyslogPath[0] == 0)
    {
        ::MessageBox(nppData._nppHandle, TEXT("Path not set. Use 'Set TC Syslog Path'."), TEXT("TC Syslog Finder"), MB_OK | MB_ICONWARNING);
        return;
    }

    // Search for the latest syslog file in TC Syslog Path
    TCHAR szSearch[MAX_PATH];
    _stprintf_s(szSearch, MAX_PATH, TEXT("%s\\%s"), szSyslogPath, szSyslogFilePattern);
    
    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(szSearch, &fd);
    
    if (hFind == INVALID_HANDLE_VALUE) 
    {
        MessageBox(nppData._nppHandle, TEXT("No syslog files found."), TEXT("TC Syslog Finder"), MB_OK | MB_ICONWARNING);
        return;
    }
    
    FILETIME ftLatest = fd.ftLastWriteTime;
    TCHAR szLatestFile[MAX_PATH];
    _stprintf_s(szLatestFile, MAX_PATH, TEXT("%s\\%s"), szSyslogPath, fd.cFileName);

    while (FindNextFile(hFind, &fd)) 
    {
        if (CompareFileTime(&fd.ftLastWriteTime, &ftLatest) > 0) 
        {
            ftLatest = fd.ftLastWriteTime;
            _stprintf_s(szLatestFile, MAX_PATH, TEXT("%s\\%s"), szSyslogPath, fd.cFileName);
        }
    }
    FindClose(hFind);  
    
    // Open the latest syslog file in Notepad++
    if (::SendMessage(nppData._nppHandle, NPPM_DOOPEN, 0, (LPARAM)szLatestFile)) 
    {
        ::SetTimer(NULL, 0, 0, (TIMERPROC)timerToScroll);
    }
}

void highlightLinesContainingPrefixByLevel()
{
    // Load the latest configuration
    loadConfig();

    if (szCustomPrefix[0] == 0)
    {
        ::MessageBox(nppData._nppHandle, TEXT("Custom prefix not set. Use 'Edit Settings' to set it."), TEXT("TC Syslog Finder"), MB_OK | MB_ICONWARNING);
        return;
    }

    markPrefixLinesByLevel(szCustomPrefix);
}

void openSyslogFolder()
{
    // Load the latest configuration
    loadConfig();

    if (szSyslogPath[0] == 0)
    {
        ::MessageBox(nppData._nppHandle, TEXT("Path not set. Use 'Set TC Syslog Path'."), TEXT("TC Syslog Finder"), MB_OK | MB_ICONWARNING);
        return;
    }
    else
    {
        ShellExecute(NULL, TEXT("explore"), szSyslogPath, NULL, NULL, SW_SHOWNORMAL);
    }
}

void openRootFolder()
{
    // Load the latest configuration
    loadConfig();

    if (szRootPath[0] == 0)
    {
        ::MessageBox(nppData._nppHandle, TEXT("Path not set. It is usually detected automatically from SIEMENS_LOGGING_ROOT or can be set in 'Edit Settings'."), TEXT("TC Syslog Finder"), MB_OK | MB_ICONWARNING);
        return;
    }
    else
    {
        ShellExecute(NULL, TEXT("explore"), szRootPath, NULL, NULL, SW_SHOWNORMAL);
    }
}

void cleanOldSyslogs()
{
    // Load the latest configuration
    loadConfig();

    if (szSyslogPath[0] == 0)
    {
        ::MessageBox(nppData._nppHandle, TEXT("Path not set. Use 'Set TC Syslog Path'."), TEXT("TC Syslog Finder"), MB_OK | MB_ICONWARNING);
        return;
    }

    int iThreshold = _ttoi(szThreshold);
    if (iThreshold <= 0 && _tcscmp(szThreshold, TEXT("0")) != 0)
    {
        iThreshold = 7;
    }

    TCHAR szMessage[MAX_PATH];
    _stprintf_s(szMessage, MAX_PATH, TEXT("Delete all syslogs older than %d days?"), iThreshold);
    
    if (::MessageBox(nppData._nppHandle, szMessage, TEXT("Clean Logs"), MB_YESNO | MB_ICONQUESTION) == IDYES)
    {
        TCHAR szSearch[MAX_PATH];
        _stprintf_s(szSearch, MAX_PATH, TEXT("%s\\%s"), szSyslogPath, szCleanupFilePattern);

        WIN32_FIND_DATA fd;
        HANDLE hFind = FindFirstFile(szSearch, &fd);

        if (hFind != INVALID_HANDLE_VALUE) 
        {
            FILETIME ftNow;
            GetSystemTimeAsFileTime(&ftNow);

            ULONGLONG thresholdDays = (ULONGLONG)iThreshold * 24ULL * 60ULL * 60ULL * 1000ULL * 1000ULL * 10ULL;

            do 
            {
                ULARGE_INTEGER uiLastWrite;
                uiLastWrite.LowPart = fd.ftLastWriteTime.dwLowDateTime;
                uiLastWrite.HighPart = fd.ftLastWriteTime.dwHighDateTime;

                ULARGE_INTEGER uiNow;
                uiNow.LowPart = ftNow.dwLowDateTime;
                uiNow.HighPart = ftNow.dwHighDateTime;

                if (uiNow.QuadPart - uiLastWrite.QuadPart > thresholdDays) 
                {
                    TCHAR szFileToDelete[MAX_PATH];
                    _stprintf_s(szFileToDelete, MAX_PATH, TEXT("%s\\%s"), szSyslogPath, fd.cFileName);
                    DeleteFile(szFileToDelete);
                }
            } 
            while (FindNextFile(hFind, &fd));
            FindClose(hFind);

            MessageBox(nppData._nppHandle, TEXT("Old logs cleaned."), TEXT("Success"), MB_OK);
        }
    }
}

void setSyslogPath()
{
    // Load the latest configuration
    loadConfig();

    // Open a folder selection dialog to set TC Syslog Path
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileDialog *pFileDialog = NULL;
        hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, IID_IFileDialog, (void**)&pFileDialog);
        if (SUCCEEDED(hr))
        {
            DWORD dwOptions;
            pFileDialog->GetOptions(&dwOptions);
            pFileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM);
            pFileDialog->SetTitle(L"Select Teamcenter syslog directory");

            if (SUCCEEDED(pFileDialog->Show(nppData._nppHandle)))
            {
                IShellItem *pItem;
                if (SUCCEEDED(pFileDialog->GetResult(&pItem)))
                {
                    PWSTR pszFilePath = NULL;
                    if (SUCCEEDED(pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath)))
                    {
                        _tcscpy_s(szSyslogPath, MAX_PATH, pszFilePath);
                        saveConfigValue(TEXT("SyslogPath"), szSyslogPath);
                        CoTaskMemFree(pszFilePath);
                    }
                    pItem->Release();
                }
            }
            pFileDialog->Release();
        }
        CoUninitialize();
    }
}

void openSettings()
{
    // Load the latest configuration
    loadConfig();
    
    // Open the configuration file in Notepad++ to allow user to edit settings
    ::SendMessage(nppData._nppHandle, NPPM_DOOPEN, 0, (LPARAM)szIniFilePath);
    ::MessageBox(nppData._nppHandle, 
        TEXT("Configuration file opened.\n\n"
             "Update the settings and save the file (Ctrl+S).\n"
             "Changes will apply immediately on your next search."), 
        TEXT("TC Syslog Finder"), MB_OK | MB_ICONINFORMATION);
}

void openGitHubRepo()
{
    // Open the GitHub repository in the default web browser
    ShellExecute(NULL, TEXT("open"), TEXT("https://github.com/bsagarzazu/npp-tc-syslog-finder"), NULL, NULL, SW_SHOWNORMAL);
}

void showAbout()
{
    TCHAR szAbout[1024];
    _stprintf_s(szAbout, 1024,
        TEXT("TC Syslog Finder v1.1.0\n\n")
        TEXT("Instantly find and open the latest Teamcenter syslog in Notepad++.\n\n")
        TEXT("Author: bsagarzazu\n")
        TEXT("License: GPL-2.0")
    );

    ::MessageBox(nppData._nppHandle, szAbout, TEXT("About TC Syslog Finder"), MB_OK | MB_ICONINFORMATION);
}