#include <windows.h>
#include <wchar.h>
#include <vector>

HWND hCombo, hLupa;
HWND hHwndEdit, hCoordEdit, hRgbEdit, hHexEdit;
HWND hCopyHwndBtn, hCopyCoordBtn, hCopyRgbBtn, hCopyHexBtn, hTopBtn;
bool freezeMode = false;
POINT frozenCursor = { 0,0 };
std::vector<HWND> windowList;
HWND selectedWindow = NULL;

#define IDT_TIMER1 1
#define ID_COPY_HWND 11
#define ID_COPY_COORD 13
#define ID_COPY_RGB 15
#define ID_COPY_HEX 17
#define ID_ALWAYS_ON_TOP 18

// копирование текста из Edit в буфер обмена
void CopyToClipboard(HWND hEdit) {
    int len = GetWindowTextLength(hEdit);
    if (len > 0) {
        wchar_t* buf = new wchar_t[len + 1];
        GetWindowText(hEdit, buf, len + 1);

        if (OpenClipboard(NULL)) {
            EmptyClipboard();
            HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, (len + 1) * sizeof(wchar_t));
            void* p = GlobalLock(hMem);
            memcpy(p, buf, (len + 1) * sizeof(wchar_t));
            GlobalUnlock(hMem);
            SetClipboardData(CF_UNICODETEXT, hMem);
            CloseClipboard();
        }
        delete[] buf;
    }
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    if (IsWindowVisible(hwnd)) {
        wchar_t title[256];
        GetWindowText(hwnd, title, 256);
        if (wcslen(title) > 0) {
            windowList.push_back(hwnd);
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)title);
        }
    }
    return TRUE;
}

void ShowCoords(POINT pt, COLORREF c) {
    wchar_t buf[64];
    if (selectedWindow) {
        POINT rel = pt;
        ScreenToClient(selectedWindow, &rel);
        swprintf_s(buf, 64, L"%d,%d", rel.x, rel.y);
        SetWindowText(hCoordEdit, buf);
    }
    else {
        swprintf_s(buf, 64, L"%d,%d", pt.x, pt.y);
        SetWindowText(hCoordEdit, buf);
    }
    swprintf_s(buf, 64, L"%d,%d,%d", GetRValue(c), GetGValue(c), GetBValue(c));
    SetWindowText(hRgbEdit, buf);

    swprintf_s(buf, 64, L"#%02X%02X%02X", GetRValue(c), GetGValue(c), GetBValue(c));
    SetWindowText(hHexEdit, buf);
}

// отдельный WndProc для лупы
LRESULT CALLBACK LupaProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_LBUTTONDOWN:
        if (freezeMode) {
            int clickX = LOWORD(lParam);
            int clickY = HIWORD(lParam);

            int size = 50;
            int half = size / 2;

            int screenX = frozenCursor.x - half + (clickX * size / 200);
            int screenY = frozenCursor.y - half + (clickY * size / 200);

            HDC hdcScreen = GetDC(NULL);
            COLORREF c = GetPixel(hdcScreen, screenX, screenY);
            ReleaseDC(NULL, hdcScreen);

            POINT pt = { screenX, screenY };
            ShowCoords(pt, c);
        }
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HFONT hFont = NULL;
    static bool topmost = false;
    switch (msg) {
    case WM_CREATE: {
        hCombo = CreateWindowEx(0, L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
            10, 10, 200, 200, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        // HWND
        CreateWindowEx(0, L"STATIC", L"HWND:", WS_CHILD | WS_VISIBLE,
            220, 10, 40, 20, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
        hHwndEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_READONLY,
            270, 10, 110, 20, hwnd, (HMENU)10, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
        hCopyHwndBtn = CreateWindowEx(0, L"BUTTON", L"Copy", WS_CHILD | WS_VISIBLE,
            390, 10, 60, 20, hwnd, (HMENU)ID_COPY_HWND, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        // Coords
        CreateWindowEx(0, L"STATIC", L"Coords:", WS_CHILD | WS_VISIBLE,
            220, 40, 50, 20, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
        hCoordEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_READONLY,
            270, 40, 110, 20, hwnd, (HMENU)12, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
        hCopyCoordBtn = CreateWindowEx(0, L"BUTTON", L"Copy", WS_CHILD | WS_VISIBLE,
            390, 40, 60, 20, hwnd, (HMENU)ID_COPY_COORD, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        // RGB
        CreateWindowEx(0, L"STATIC", L"RGB:", WS_CHILD | WS_VISIBLE,
            220, 70, 50, 20, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
        hRgbEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_READONLY,
            270, 70, 110, 20, hwnd, (HMENU)14, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
        hCopyRgbBtn = CreateWindowEx(0, L"BUTTON", L"Copy", WS_CHILD | WS_VISIBLE,
            390, 70, 60, 20, hwnd, (HMENU)ID_COPY_RGB, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        // HEX
        CreateWindowEx(0, L"STATIC", L"HEX:", WS_CHILD | WS_VISIBLE,
            220, 100, 50, 20, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
        hHexEdit = CreateWindowEx(WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_READONLY,
            270, 100, 110, 20, hwnd, (HMENU)16, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
        hCopyHexBtn = CreateWindowEx(0, L"BUTTON", L"Copy", WS_CHILD | WS_VISIBLE,
            390, 100, 60, 20, hwnd, (HMENU)ID_COPY_HEX, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        // Always on top
        hTopBtn = CreateWindowEx(0, L"BUTTON", L"On top", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            220, 130, 80, 25, hwnd, (HMENU)ID_ALWAYS_ON_TOP, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        // Подсказка
        CreateWindowEx(0, L"STATIC",
            L"Ctrl+Shift+C — check coords\nCtrl+Shift+F — freeze/unfreeze",
            WS_CHILD | WS_VISIBLE,
            220, 220, 230, 40,
            hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        // Лупа
        WNDCLASS wcLupa = { 0 };
        wcLupa.lpfnWndProc = LupaProc;
        wcLupa.hInstance = ((LPCREATESTRUCT)lParam)->hInstance;
        wcLupa.lpszClassName = L"LupaClass";
        wcLupa.hCursor = LoadCursor(NULL, IDC_CROSS);
        wcLupa.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        RegisterClass(&wcLupa);

        hLupa = CreateWindowEx(0, L"LupaClass", L"", WS_CHILD | WS_VISIBLE,
            10, 70, 200, 200, hwnd, (HMENU)3, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

        RegisterHotKey(hwnd, 1, MOD_CONTROL | MOD_SHIFT, 'C');
        RegisterHotKey(hwnd, 1, MOD_CONTROL | MOD_SHIFT, 'C'); // взять координаты
        RegisterHotKey(hwnd, 2, MOD_CONTROL | MOD_SHIFT, 'F'); // freeze/unfreeze

        // Заполнение списка окон
        EnumWindows(EnumWindowsProc, 0);

        // Таймер лупы
        SetTimer(hwnd, IDT_TIMER1, 300, NULL);

        // Шрифт Consolas
        HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FIXED_PITCH | FF_MODERN, L"Consolas");
        SendMessage(hHwndEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hCoordEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hRgbEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessage(hHexEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
    }
                  break;

    case WM_COMMAND:
        if (HIWORD(wParam) == CBN_SELCHANGE && (HWND)lParam == hCombo) {
            int index = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
            if (index >= 0 && index < (int)windowList.size()) {
                selectedWindow = windowList[index];
                wchar_t buf[64];
                swprintf_s(buf, 64, L"%llu", (unsigned long long)(uintptr_t)selectedWindow);
                SetWindowText(hHwndEdit, buf);
            }
        }
        switch (LOWORD(wParam)) {
        case ID_COPY_HWND:  CopyToClipboard(hHwndEdit);  break;
        case ID_COPY_COORD: CopyToClipboard(hCoordEdit); break;
        case ID_COPY_RGB:   CopyToClipboard(hRgbEdit);   break;
        case ID_COPY_HEX:   CopyToClipboard(hHexEdit);   break;
        case ID_ALWAYS_ON_TOP: {
            static bool topmost = false;
            topmost = !topmost;
            SetWindowPos(hwnd,
                topmost ? HWND_TOPMOST : HWND_NOTOPMOST,
                0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE);
            SetWindowText(hTopBtn, topmost ? L"Normal" : L"On top");
        } break;
        }
        break;

    case WM_HOTKEY:
        if (wParam == 1) {
            POINT pt; GetCursorPos(&pt);
            HDC hdcScreen = GetDC(NULL);
            COLORREF c = GetPixel(hdcScreen, pt.x, pt.y);
            ReleaseDC(NULL, hdcScreen);
            ShowCoords(pt, c);
        }
        else if (wParam == 2) {
            freezeMode = !freezeMode;
            if (freezeMode) {
                GetCursorPos(&frozenCursor);
            }
        }
        break;

    case WM_TIMER:
        if (wParam == IDT_TIMER1 && !freezeMode) {
            static int frame = 0;

            HDC hdcScreen = GetDC(NULL);
            HDC hdcLupa = GetDC(hLupa);

            POINT pt; GetCursorPos(&pt);
            int size = 50;
            int half = size / 2;

            // растягиваем 50x50 в 200x200
            StretchBlt(hdcLupa, 0, 0, 200, 200,
                hdcScreen, pt.x - half, pt.y - half, size, size,
                SRCCOPY);

            // рисуем перекрестие
            COLORREF colors[] = { RGB(255,255,255), RGB(255,0,0), RGB(0,255,0), RGB(0,0,255) };
            HPEN pen = CreatePen(PS_SOLID, 1, colors[frame % 4]);
            HGDIOBJ oldPen = SelectObject(hdcLupa, pen);
            MoveToEx(hdcLupa, 95, 100, NULL); LineTo(hdcLupa, 105, 100);
            MoveToEx(hdcLupa, 100, 95, NULL); LineTo(hdcLupa, 100, 105);
            SelectObject(hdcLupa, oldPen);
            DeleteObject(pen);

            frame++;

            ReleaseDC(NULL, hdcScreen);
            ReleaseDC(hLupa, hdcLupa);
        }
        break;

    case WM_DESTROY:
        KillTimer(hwnd, IDT_TIMER1);
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nShow) {
    WNDCLASS wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"ColorCheckerMain";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, L"ColorCheckerMain", L"ColorChecker — координаты и цвет пикселя",
        WS_OVERLAPPEDWINDOW, 100, 100, 500, 320,
        NULL, NULL, hInst, NULL);
    ShowWindow(hwnd, nShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}
