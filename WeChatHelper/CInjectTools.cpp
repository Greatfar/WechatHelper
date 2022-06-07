#include "stdafx.h"
#include "CInjectTools.h"
#include <direct.h>
#include <stdlib.h>
#include <TlHelp32.h>
#include <stdio.h>
#include "PublicFunction.h"
#pragma comment(lib,"advapi32")


#define WECHAT_PROCESS_NAME "WeChat.exe"
#define DLLNAME "WeChatTools.dll"


CString GetAppRegeditPath(CString strAppName)
{
    //定义相关变量
    HKEY hKey;
    CString strAppRegeditPath("");
    TCHAR szProductType[MAX_PATH];
    memset(szProductType, 0, sizeof(szProductType));

    DWORD dwBuflen = MAX_PATH;
    LONG lRet = 0;

    //下面是打开注册表,只有打开后才能做其他操作
    lRet = RegOpenKeyEx(HKEY_CURRENT_USER, //要打开的根键
        LPCTSTR(strAppName), //要打开的子子键
        0, //这个一定为0
        KEY_QUERY_VALUE, //指定打开方式,此为读
        &hKey); //用来返回句柄

    if (lRet != ERROR_SUCCESS) //判断是否打开成功
    {
        return strAppRegeditPath;
    }
    else
    {
        //下面开始查询
        lRet = RegQueryValueEx(hKey, //打开注册表时返回的句柄
            TEXT("Wechat"), //要查询的名称,查询的软件安装目录在这里
            NULL, //一定为NULL或者0
            NULL,
            (LPBYTE)szProductType, //我们要的东西放在这里
            &dwBuflen);

        if (lRet != ERROR_SUCCESS) //判断是否查询成功
        {
            return strAppRegeditPath;
        }
        else
        {
            RegCloseKey(hKey);
            
            strAppRegeditPath = szProductType;

            int nPos = strAppRegeditPath.Find('-');

            if (nPos >= 0)
            {
                CString sSubStr = strAppRegeditPath.Left(nPos - 1);//包含$,不想包含时nPos+1
                strAppRegeditPath = sSubStr;
            }
        }
    }
    return strAppRegeditPath;
}


CString GetAppRegeditPath2(CString strAppName)
{
    //定义相关变量
    HKEY hKey;
    CString strAppRegeditPath("");
    TCHAR szProductType[MAX_PATH];
    memset(szProductType, 0, sizeof(szProductType));

    DWORD dwBuflen = MAX_PATH;
    LONG lRet = 0;

    //下面是打开注册表,只有打开后才能做其他操作
    lRet = RegOpenKeyEx(HKEY_CURRENT_USER, //要打开的根键
        LPCTSTR(strAppName), //要打开的子子键
        0, //这个一定为0
        KEY_QUERY_VALUE, //指定打开方式,此为读
        &hKey); //用来返回句柄

    if (lRet != ERROR_SUCCESS) //判断是否打开成功
    {
        return strAppRegeditPath;
    }
    else
    {
        //下面开始查询
        lRet = RegQueryValueEx(hKey, //打开注册表时返回的句柄
            TEXT("InstallPath"), //要查询的名称,查询的软件安装目录在这里
            NULL, //一定为NULL或者0
            NULL,
            (LPBYTE)szProductType, //我们要的东西放在这里
            &dwBuflen);

        if (lRet != ERROR_SUCCESS) //判断是否查询成功
        {
            return strAppRegeditPath;
        }
        else
        {
            RegCloseKey(hKey);
            strAppRegeditPath = szProductType;

        }
    }
    return strAppRegeditPath;
}


//************************************************************
// 函数名称: InjectDll
// 函数说明: 注入DLL
// 作    者: Greatfar
// 时    间: 2022/03/30
// 参    数: void
// 返 回 值: void
//************************************************************
BOOL InjectDll(HANDLE& wxPid)
{
    // 获取当前工作目录下的dll
    char szPath[MAX_PATH] = { 0 };
    char* buffer = NULL;
    if ((buffer = _getcwd(NULL, 0)) == NULL)
    {
        MessageBoxA(NULL, "获取当前工作目录失败", "错误", 0);
        return FALSE;
    }
    sprintf_s(szPath, "%s\\%s", buffer, DLLNAME);
    // 获取微信Pid
    DWORD dwPid = ProcessNameFindPID(WECHAT_PROCESS_NAME);
    if (dwPid == 0)
    {
        CString wxStrAppName = TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Run");
        CString szProductType = GetAppRegeditPath(wxStrAppName);

        if (szProductType.GetLength()<5)
        {
            wxStrAppName = TEXT("Software\\Tencent\\WeChat");
            szProductType = GetAppRegeditPath2(wxStrAppName);
            szProductType.Append(L"\\WeChat.exe");
        }
        STARTUPINFO si;
        PROCESS_INFORMATION pi;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));

        si.dwFlags = STARTF_USESHOWWINDOW;// 指定wShowWindow成员有效
        si.wShowWindow = TRUE;          // 此成员设为TRUE的话则显示新建进程的主窗口，
                                       // 为FALSE的话则不显示
        // 打开微信登录窗口
        CreateProcess(szProductType, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);

        HWND  hWechatMainForm = NULL; // 微信登录窗口句柄
        int checkWechatLoginFormTimes = 0; // 查找微信登录窗口的次数
        // 循环 查找微信登录窗口的句柄
        while (NULL == hWechatMainForm)
        {
            // 查找一定次数后停止查找
            checkWechatLoginFormTimes++;
            if (checkWechatLoginFormTimes > 120) {
                MessageBoxA(NULL, "微信启动失败，请确认是否已安装微信电脑版", "tips", MB_SYSTEMMODAL);
                break;
            }
            // 查找微信登录窗口
            hWechatMainForm = FindWindow(TEXT("WeChatLoginWndForPC"), NULL);
            Sleep(500);
        }
        if (NULL == hWechatMainForm)
        {
            return FALSE;
        }
        dwPid = pi.dwProcessId;
        wxPid = pi.hProcess;
    }
    // 检测dll是否已经注入
    if (CheckIsInject(dwPid))
    {
        // 打开进程
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);
        if (hProcess == NULL)
        {
            MessageBoxA(NULL, "进程打开失败", "错误", 0);
            return FALSE;
        }
        // 在微信进程中申请内存
        LPVOID pAddress = VirtualAllocEx(hProcess, NULL, MAX_PATH, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
        if (pAddress == NULL)
        {
            MessageBoxA(NULL, "内存分配失败", "错误", 0);
            return FALSE;
        }
        // 写入dll路径到微信进程
        if (WriteProcessMemory(hProcess, pAddress, szPath, MAX_PATH, NULL) == 0)
        {
            MessageBoxA(NULL, "路径写入失败", "错误", 0);
            return FALSE;
        }

        // 获取LoadLibraryA函数地址
        FARPROC pLoadLibraryAddress = GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA");
        if (pLoadLibraryAddress == NULL)
        {
            MessageBoxA(NULL, "获取LoadLibraryA函数地址失败", "错误", 0);
            return FALSE;
        }
        // 远程线程注入dll
        HANDLE hRemoteThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibraryAddress, pAddress, 0, NULL);
        if (hRemoteThread == NULL)
        {
            MessageBoxA(NULL, "远程线程注入失败", "错误", 0);
            return FALSE;
        }

        CloseHandle(hRemoteThread);
        CloseHandle(hProcess);
    }
    else
    {
        MessageBoxA(NULL, "请把微信关闭后，再打开微信助手", "tips", 0);
        // 退出当前进程
        ExitProcess(-1);
        // return FALSE;
    }
    return TRUE;
}


//************************************************************
// 函数名称: CheckIsInject
// 函数说明: 检测是否已经注入dll
// 作    者: Greatfar
// 时    间: 2022/03/30
// 参    数: void
// 返 回 值: BOOL 
//************************************************************
BOOL CheckIsInject(DWORD dwProcessid)
{
    HANDLE hModuleSnap = INVALID_HANDLE_VALUE;
    //初始化模块信息结构体
    MODULEENTRY32 me32 = { sizeof(MODULEENTRY32) };
    //创建模块快照 1 快照类型 2 进程ID
    hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, dwProcessid);
    //如果句柄无效就返回false
    if (hModuleSnap == INVALID_HANDLE_VALUE)
    {
        MessageBoxA(NULL, "创建模块快照失败", "错误", MB_OK);
        return FALSE;
    }
    //通过模块快照句柄获取第一个模块的信息
    if (!Module32First(hModuleSnap, &me32))
    {
        MessageBoxA(NULL, "获取第一个模块的信息失败", "错误", MB_OK);
        //获取失败则关闭句柄
        CloseHandle(hModuleSnap);
        return FALSE;
    }
    do
    {
        if (StrCmpW(me32.szModule, L"WeChatTools.dll") == 0)
        {
            return FALSE;
        }

    } while (Module32Next(hModuleSnap, &me32));
    return TRUE;
}

