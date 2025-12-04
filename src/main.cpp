#include <windows.h>
#include <tchar.h>
#include <setupapi.h>
#include <iostream>
#include <string>
#include <fstream>
#include <thread>
#include "nlohmann/json.hpp"
#include "utils/boot.hpp"
#include "utils/process.hpp"
#include "utils/Time.hpp"

using namespace std;
using Json = nlohmann::json;

// 托盘图标的ID
#define ID_TRAY_ICON 1001
#define WM_TRAY_MESSAGE (WM_APP + 1)

#define PATH_DEVICES "devices.txt"
#define PATH_SETTING "settings.ini"
#define APP_NAME "LightGunCfg"

#define JSON_KEY_ACTIVED "actived"
#define JSON_KEY_VK "vk"
#define JSON_KEY_KEYBOARD "keyboard"
#define JSON_KEY_MOUSE "mouse"
#define JSON_KEY_ID "id"
#define JSON_KEY_HANDLES "handles"
#define JSON_KEY_BTN "btn"
#define JSON_KEY_SINGLE "single"
#define JSON_KEY_BTNUP "btnUp"
#define JSON_KEY_BTNDOWN "btnDown"
#define JSON_KEY_DURATION "duration"
#define JSON_KEY_STARTUP "startup"
#define JSON_KEY_EXCEPTION "exception"
#define JSON_KEY_PROCESS "process"
#define JSON_KEY_MODULE "module"

#define CHECK_EXCEPTION(jsonData)                                                                                    \
    {                                                                                                                \
        for (const auto &exception : jsonData[JSON_KEY_EXCEPTION]) {                                                 \
            if (ProcessHelper::ifProcess(string(exception[JSON_KEY_PROCESS]), string(exception[JSON_KEY_MODULE]))) { \
                return;                                                                                              \
            }                                                                                                        \
        }                                                                                                            \
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

void longPress(Json *handle, const int handleIndex, const int activedLen, const int duration, const unsigned long long timestamp) {
    unsigned long long now = utils::timestamp();
    int j;
    Json &actived = (*(handle))[JSON_KEY_ACTIVED];
    do {
        for (j = 0; j < activedLen; j++) {
            if (!actived[j]) {
                return;
            }
        }
        now = utils::timestamp();
    } while (now - timestamp < duration);
    // cout << "vk: " << (*handle)[JSON_KEY_VK] << endl;
    keybd_event((*handle)[JSON_KEY_VK], 0, 0, 0);
    Sleep(500);
    keybd_event((*handle)[JSON_KEY_VK], 0, KEYEVENTF_KEYUP, 0);
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
            // cout << "keyboard name: " << name << endl;
            for (const auto &keyboard : jsonData[JSON_KEY_KEYBOARD]) {
                if (name == keyboard[JSON_KEY_ID]) {
                    if (input->data.keyboard.VKey) {
                        // cout << "Flags: " << input->data.keyboard.Flags << endl;
                        // cout << "VKey: " << input->data.keyboard.VKey << endl;
                        btn = input->data.keyboard.VKey;
                        for (const auto &handle : keyboard[JSON_KEY_HANDLES]) {
                            if (btn == handle[JSON_KEY_BTN]) {
                                CHECK_EXCEPTION(handle);
                                if (input->data.keyboard.Flags) {
                                    keybd_event(handle[JSON_KEY_VK], 0, KEYEVENTF_KEYUP, 0);
                                } else {
                                    keybd_event(handle[JSON_KEY_VK], 0, 0, 0);
                                }
                            }
                        }
                    }
                    break;
                }
            }
        } else {
            for (Json &mouse : jsonData[JSON_KEY_MOUSE]) {

                if (name == mouse[JSON_KEY_ID]) {
                    // cout << "usButtonFlags: " << input->data.mouse.usButtonFlags << endl;
                    // cout << "name: " << name << endl;
                    btn = input->data.mouse.usButtonFlags;

                    static int i, len;
                    Json &handles = mouse[JSON_KEY_HANDLES];
                    for (i = 0, len = handles.size(); i < len; i++) {
                        Json &handle = handles[i];
                        CHECK_EXCEPTION(handle);

                        if (handle[JSON_KEY_SINGLE]) {
                            if (btn == handle[JSON_KEY_BTNDOWN]) {
                                keybd_event(handle[JSON_KEY_VK], 0, 0, 0);
                                break;
                            } else if (btn == handle[JSON_KEY_BTNUP]) {
                                keybd_event(handle[JSON_KEY_VK], 0, KEYEVENTF_KEYUP, 0);
                                break;
                            }
                        } else {
                            static int j, lenJ;

                            static long duration;

                            Json &btnDownArr = handle[JSON_KEY_BTNDOWN];
                            for (j = 0, lenJ = btnDownArr.size(); j < lenJ; j++) {
                                Json &btnDown = btnDownArr[j];
                                if (btn == btnDown) {
                                    // cout << "btn: " << btn << endl;
                                    duration = handle[JSON_KEY_DURATION];
                                    if (duration > 0) {
                                        Json &actived = handle[JSON_KEY_ACTIVED];
                                        actived[j] = true;

                                        for (j = 0; j < lenJ; j++) {
                                            // cout << "index: " << j << ", actived: " << actived[j] << endl;
                                            if (!actived[j]) {
                                                goto Done;
                                            }
                                        }

                                        thread t1(longPress, &handle, i, lenJ, duration, utils::timestamp());
                                        t1.detach();
                                    }
                                    goto Done;
                                }
                            }

                            Json &btnUpArr = handle[JSON_KEY_BTNUP];
                            for (j = 0, lenJ = btnUpArr.size(); j < lenJ; j++) {
                                Json &btnUp = btnUpArr[j];
                                if (btn == btnUp) {
                                    if (handle[JSON_KEY_DURATION] > 0) {

                                        // cout << "handle up: " << handle.dump() << endl;

                                        handle[JSON_KEY_ACTIVED][j] = false;
                                    } else {
                                        keybd_event(handle[JSON_KEY_VK], 0, 0, 0);
                                    }
                                    goto Done;
                                }
                            }
                        }
                    Done:
                        continue;
                    }
                }
            }
        }

        free(name);
    }

    free(input);
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

void readCfg() {
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

        ifs.close();

        jsonStr = ReplaceAll(jsonStr, "\\", "\\\\");

        jsonData = Json::parse(jsonStr);

        int btnCnt, handleCnt, j;
        for (Json &mouse : jsonData[JSON_KEY_MOUSE]) {
            Json &handles = mouse[JSON_KEY_HANDLES];
            for (j = 0, handleCnt = handles.size(); j < handleCnt; j++) {
                Json &handle = handles[j];
                handle[JSON_KEY_SINGLE] = handle[JSON_KEY_BTNDOWN].is_number();
                if (handle[JSON_KEY_DURATION] > 0) {
                    // vector<unsigned long long> v;
                    // for (int i = 0; i < handle[JSON_KEY_BTNDOWN].size(); i++) {
                    //     v.push_back(0);
                    // }
                    // handle["dTime"] = Json(v);
                    // handle["uTime"] = Json(v);
                    vector<bool> v;
                    for (int i = 0, btnCnt = handle[JSON_KEY_BTNDOWN].size(); i < btnCnt; i++) {
                        v.push_back(false);
                    }
                    handle[JSON_KEY_ACTIVED] = v;
                }
                if (!handle.contains(JSON_KEY_EXCEPTION)) {
                    handle[JSON_KEY_EXCEPTION] = {};
                }
            }
        }

        for (Json &keyboard : jsonData[JSON_KEY_KEYBOARD]) {
            for (Json &handle : keyboard[JSON_KEY_HANDLES]) {
                if (!handle.contains(JSON_KEY_EXCEPTION)) {
                    handle[JSON_KEY_EXCEPTION] = {};
                }
            }
        }
        // cout << jsonData.dump() << endl;

        if (jsonData[JSON_KEY_STARTUP]) {
            Boot::AutoPowerOn();
        } else {
            Boot::CanclePowerOn();
        }

        printDevices();
    } else {
        printDevices();
    }
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_TRAY_MESSAGE:
        // 如果收到的是右键点击事件
        if (lParam == WM_RBUTTONUP) {
            // 创建一个弹出菜单
            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, 2, _T("读取配置")); // 向菜单添加“退出”选项
            AppendMenu(hMenu, MF_STRING, 1, _T("退出程序")); // 向菜单添加“退出”选项

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
        switch (wParam) {
        case 1:
            // 退出应用程序
            PostQuitMessage(0);
            break;
        case 2:
            readCfg();
            break;
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
        wcx.lpszClassName = TEXT(APP_NAME);
        RegisterClassEx(&wcx);
        HWND hWnd = CreateWindowEx(0, wcx.lpszClassName, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);

        // 创建并初始化托盘图标数据结构
        nid.cbSize = sizeof(NOTIFYICONDATA);                           // 设置结构体大小
        nid.hWnd = hWnd;                                               // 设置窗口句柄，托盘图标和该窗口相关联
        nid.uID = ID_TRAY_ICON;                                        // 图标的唯一标识符
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;                 // 设置托盘图标显示内容，包括图标、消息和提示文本
        nid.uCallbackMessage = WM_TRAY_MESSAGE;                        // 设置回调消息，当用户与图标交互时，消息会发送到指定窗口
        nid.hIcon = ::ExtractIcon(GetModuleHandle(NULL), namePath, 0); // 设置托盘图标
        lstrcpyn(nid.szTip, _T(APP_NAME), ARRAYSIZE(nid.szTip));  // 设置图标的提示文本

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

        readCfg();
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