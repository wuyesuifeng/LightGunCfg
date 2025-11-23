#include <windows.h>
#include <tchar.h>
#include <setupapi.h>
#include <iostream>
#include <string>
#include <fstream>
#include "nlohmann/json.hpp"
#include "utils/boot.hpp"
#include "utils/process.hpp"

using namespace std;
using Json = nlohmann::json;

// 托盘图标的ID
#define ID_TRAY_ICON 1001
#define WM_TRAY_MESSAGE (WM_APP + 1)

#define PATH_DEVICES "devices.txt"
#define PATH_SETTING "settings.ini"

#define CHECK_EXCEPTION(jsonData)                                                                      \
    {                                                                                                  \
        for (const auto &exception : jsonData["exception"]) {                                          \
            if (ProcessHelper::ifProcess(string(exception["process"]), string(exception["module"]))) { \
                return;                                                                                \
            }                                                                                          \
        }                                                                                              \
    }

string absolutePath;
Json jsonData;
string rootDir;

string getAbsolutePath(const char *name) {
    string result = rootDir;
    absolutePath = result.append(name).c_str();
    return absolutePath;
}

void Log(const char *str) {
    ofstream ofs;
    static const char *path = getAbsolutePath("debug.log").c_str();
    ofs.open(path, ios::app);
    while (!ofs.is_open()) {
        Sleep(1000);
        ofs.open(path, ios::app);
    }
    ofs << str << endl;
    ofs.close();
}

void typeButton(LPARAM lParam) {
    static HRAWINPUT hRawInput;
    hRawInput = (HRAWINPUT)lParam;
    static RAWINPUT *input;
    input = (RAWINPUT *)malloc(sizeof(RAWINPUT));
    static UINT size;
    size = sizeof(*input);
    GetRawInputData(hRawInput, RID_INPUT, input, &size, sizeof(RAWINPUTHEADER));

    static DWORD type;
    type = input->header.dwType;

    if (type > 1) {
        return;
    }

    static RID_DEVICE_INFO info;
    size = sizeof(RID_DEVICE_INFO);
    static HANDLE hDevice;
    hDevice = input->header.hDevice;

    if (size = GetRawInputDeviceInfo(hDevice, RIDI_DEVICEINFO, &info, &size)) {
        static unsigned int nameSize;
        GetRawInputDeviceInfo(hDevice, RIDI_DEVICENAME, nullptr, &nameSize);
        static char *name;
        name = (char *)malloc(nameSize);
        GetRawInputDeviceInfo(hDevice, RIDI_DEVICENAME, name, &nameSize);

        static USHORT btn;
        if (type) {
            for (const auto &keyboard : jsonData["keyboard"]) {
                if (name == keyboard["id"]) {
                    if (input->data.keyboard.VKey) {
                        // cout << "Flags: " << input->data.keyboard.Flags << endl;
                        // cout << "VKey: " << input->data.keyboard.VKey << endl;
                        btn = input->data.keyboard.VKey;
                        for (const auto &handle : keyboard["handles"]) {
                            if (btn == handle["btn"]) {
                                CHECK_EXCEPTION(jsonData);
                                keybd_event(handle["vk"], 0, 0, 0);
                            }
                        }
                    }
                    break;
                }
            }
        } else {
            for (const auto &mouse : jsonData["mouse"]) {

                if (name == mouse["id"]) {
                    // cout << "usButtonFlags: " << input->data.mouse.usButtonFlags << endl;
                    // cout << "name: " << name << endl;
                    btn = input->data.mouse.usButtonFlags;
                    string process, module;
                    for (const auto &handle : mouse["handles"]) {
                        CHECK_EXCEPTION(jsonData);
                        if (btn == handle["btnDown"]) {
                            keybd_event(handle["vk"], 0, 0, 0);
                            break;
                        } else if (btn == handle["btnUp"]) {
                            keybd_event(handle["vk"], 0, KEYEVENTF_KEYUP, 0);
                            break;
                        }
                    }
                    break;
                }
            }
        }

        free(name);
    }

    free(input);
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

void printName(PRAWINPUTDEVICELIST &pRawInputDeviceList, int i, string &str) {
    unsigned int nameSize;
    GetRawInputDeviceInfo(pRawInputDeviceList[i].hDevice, RIDI_DEVICENAME, nullptr, &nameSize);
    char *name = (char *)malloc(nameSize);
    GetRawInputDeviceInfo(pRawInputDeviceList[i].hDevice, RIDI_DEVICENAME, name, &nameSize);
    str.append("name: ");
    str.append(name);
    str.append("\n\n");
    free(name);
}

void printDevices() {
    UINT nDevices;
    PRAWINPUTDEVICELIST pRawInputDeviceList = NULL;

    GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST));
    if (nDevices == 0) {
        return;
    }
    if (pRawInputDeviceList = (PRAWINPUTDEVICELIST)malloc(sizeof(RAWINPUTDEVICELIST) * nDevices)) {
        nDevices = GetRawInputDeviceList(pRawInputDeviceList, &nDevices, sizeof(RAWINPUTDEVICELIST));
        RID_DEVICE_INFO info;
        unsigned int size = sizeof(RID_DEVICE_INFO);
        string str;
        for (int i = 0; i < nDevices; i++) {
            if (size = GetRawInputDeviceInfo(pRawInputDeviceList[i].hDevice, RIDI_DEVICEINFO, &info, &size)) {
                switch (info.dwType) {
                case RIM_TYPEMOUSE:
                    str.append("type: 鼠标\n");
                    printName(pRawInputDeviceList, i, str);
                    break;
                case RIM_TYPEKEYBOARD:
                    str.append("type: 键盘\n");
                    printName(pRawInputDeviceList, i, str);
                }
            }
        }

        ofstream ofs;
        ofs.open(getAbsolutePath(PATH_DEVICES).c_str(), ios::out);
        if (ofs.is_open()) {
            ofs << str;
            ofs.close();
        }
    }
    free(pRawInputDeviceList);
}

std::string ReplaceAll(std::string str, const std::string &src, const std::string &dst) {
    std::string::size_type pos(0);
    int diff = dst.length() - src.length();
    diff = diff > 0 ? diff + 1 : 1;
    while ((pos = str.find(src, pos)) != std::string::npos) {
        str.replace(pos, src.length(), dst);
        pos += diff;
    }
    return str;
}

int main() {
    NOTIFYICONDATA nid = {0};

    {
        char namePath[MAX_PATH + 1] = {0};
        GetModuleFileNameA(NULL, namePath, MAX_PATH);
        rootDir.append(namePath);
        rootDir = rootDir.substr(0, rootDir.find_last_of("\\") + 1);

        WNDCLASSEX wcx = {0};
        wcx.cbSize = sizeof(WNDCLASSEX);
        wcx.lpfnWndProc = WindowProc;
        wcx.hInstance = GetModuleHandle(NULL);
        wcx.lpszClassName = TEXT("LightGunCfg");
        RegisterClassEx(&wcx);
        HWND hWnd = CreateWindowEx(0, wcx.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);

        // 创建并初始化托盘图标数据结构
        nid.cbSize = sizeof(NOTIFYICONDATA);                           // 设置结构体大小
        nid.hWnd = hWnd;                                               // 设置窗口句柄，托盘图标和该窗口相关联
        nid.uID = ID_TRAY_ICON;                                        // 图标的唯一标识符
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;                 // 设置托盘图标显示内容，包括图标、消息和提示文本
        nid.uCallbackMessage = WM_TRAY_MESSAGE;                        // 设置回调消息，当用户与图标交互时，消息会发送到指定窗口
        nid.hIcon = ::ExtractIcon(GetModuleHandle(NULL), namePath, 0); // 设置托盘图标
        lstrcpyn(nid.szTip, _T("LightGunCfg"), ARRAYSIZE(nid.szTip));  // 设置图标的提示文本

        // 将托盘图标添加到系统托盘
        while (!Shell_NotifyIcon(NIM_ADD, &nid)) {
            Sleep(1000);
        }

        RAWINPUTDEVICE rid[2];
        rid[0].usUsagePage = 0x01;
        rid[0].usUsage = 0x06; // keyboard
        rid[0].dwFlags = RIDEV_INPUTSINK;
        rid[0].hwndTarget = hWnd;

        rid[1].usUsagePage = 0x01;
        rid[1].usUsage = 0x02; // mouse
        rid[1].dwFlags = RIDEV_INPUTSINK;
        rid[1].hwndTarget = hWnd;

        while (!RegisterRawInputDevices(rid, 2, sizeof(RAWINPUTDEVICE))) {
            Sleep(1000);
        }

        ifstream ifs2(getAbsolutePath(PATH_SETTING).c_str());

        if (ifs2.good()) {
            ifstream ifs;
            ifs.open(getAbsolutePath(PATH_SETTING).c_str(), ios::in);

            while (!ifs.is_open()) {
                Sleep(1000);
                ifs.open(getAbsolutePath(PATH_SETTING).c_str(), ios::in);
            }
            string buff, jsonStr;
            while (getline(ifs, buff)) {
                jsonStr.append(buff);
            }
            jsonStr = ReplaceAll(jsonStr, "\\", "\\\\");

            jsonData = Json::parse(jsonStr);
            // std::cout << "id:\n"
            //           << (string)jsonData["keyboard"][0]["id"] << "\n\n";

            if (jsonData["startup"]) {
                Boot::AutoPowerOn();
            } else {
                Boot::CanclePowerOn();
            }

            ifs.close();

            printDevices();
        } else {
            printDevices();
            return 1;
        }
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // 程序退出前，删除托盘图标
    while (!Shell_NotifyIcon(NIM_DELETE, &nid)) {
        Sleep(1000);
    }

    return 0;
}