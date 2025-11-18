#include <windows.h>
#include <tchar.h>
#include <iostream>

using namespace std;

// 托盘图标的ID
#define ID_TRAY_ICON 1001
#define WM_TRAY_MESSAGE (WM_APP + 1)

void typeButton(LPARAM lParam) {
    static HRAWINPUT hRawInput;
    hRawInput = (HRAWINPUT)lParam;
    static RAWINPUT input = {0};
    static UINT size = sizeof(input);
    GetRawInputData(hRawInput, RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER));
    if (input.data.mouse.usFlags) {
        switch (input.data.mouse.usButtonFlags) {
        case 16:
            keybd_event(48 + input.data.mouse.usFlags, 0, 0, 0);
            break;
        case 32:
            keybd_event(48 + input.data.mouse.usFlags, 0, KEYEVENTF_KEYUP, 0);
            break;
        }
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_TRAY_MESSAGE:
        // 如果收到的是右键点击事件
        if (lParam == WM_RBUTTONUP) {
            // 创建一个弹出菜单
            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, 1, _T("退出")); // 向菜单添加“退出”选项

            // 获取鼠标当前位置
            POINT pt;
            GetCursorPos(&pt);

            // 设置窗口为前景窗口（确保菜单显示在窗口前面）
            SetForegroundWindow(hwnd);

            // 显示菜单
            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);

            // 菜单使用完毕后销毁
            DestroyMenu(hMenu);
        }
        break;

    case WM_COMMAND:
        // 如果点击的是“退出”菜单项
        if (wParam == 1) {
            // 退出应用程序
            PostQuitMessage(0);
        }
        break;

    case WM_DESTROY:
        // 销毁窗口时，退出应用程序
        PostQuitMessage(0);
        break;

    case WM_INPUT:
        typeButton(lParam);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam); // 处理其他消息
    }
    return 0;
}

int main() {

    WNDCLASSEX wcx = {0};
    wcx.cbSize = sizeof(WNDCLASSEX);
    wcx.lpfnWndProc = WindowProc;
    wcx.hInstance = GetModuleHandle(NULL);
    wcx.lpszClassName = TEXT("RawInputClass");
    RegisterClassEx(&wcx);
    HWND hWnd = CreateWindowEx(0, TEXT("RawInputClass"), NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);

    // 创建并初始化托盘图标数据结构
    NOTIFYICONDATA nid = {0};
    nid.cbSize = sizeof(NOTIFYICONDATA);                          // 设置结构体大小
    nid.hWnd = hWnd;                                              // 设置窗口句柄，托盘图标和该窗口相关联
    nid.uID = ID_TRAY_ICON;                                       // 图标的唯一标识符
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;                // 设置托盘图标显示内容，包括图标、消息和提示文本
    nid.uCallbackMessage = WM_TRAY_MESSAGE;                       // 设置回调消息，当用户与图标交互时，消息会发送到指定窗口
    nid.hIcon = ::ExtractIcon(GetModuleHandle(NULL), "./LightGunCfg.exe", 0); // 设置托盘图标
    lstrcpyn(nid.szTip, _T("LightGunCfg"), ARRAYSIZE(nid.szTip)); // 设置图标的提示文本

    // 将托盘图标添加到系统托盘
    Shell_NotifyIcon(NIM_ADD, &nid);

    RAWINPUTDEVICE rid = {0};
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02; // keyboard
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = hWnd;

    RegisterRawInputDevices(&rid, 1, sizeof(RAWINPUTDEVICE));

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 程序退出前，删除托盘图标
    Shell_NotifyIcon(NIM_DELETE, &nid);

    return 0;
}