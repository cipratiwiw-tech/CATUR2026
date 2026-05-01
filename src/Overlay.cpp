#include "Overlay.h"

Overlay::Overlay() : hwnd(NULL) {}
Overlay::~Overlay() { if (hwnd) DestroyWindow(hwnd); }

LRESULT CALLBACK Overlay::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // GUNAKAN GetClientRect agar seluruh jendela ter-refresh, bukan cuma area sisa
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);

            // Bersihkan background dengan warna Magenta (Transparan)
            HBRUSH hbr = CreateSolidBrush(RGB(255, 0, 255));
            FillRect(hdc, &clientRect, hbr);
            DeleteObject(hbr);
            
            // Gambar Border merah
            HPEN hPen = CreatePen(PS_SOLID, 5, RGB(255, 0, 0));
            SelectObject(hdc, hPen);
            SelectObject(hdc, GetStockObject(NULL_BRUSH));
            Rectangle(hdc, 0, 0, clientRect.right, clientRect.bottom);
            DeleteObject(hPen);
            
            EndPaint(hwnd, &ps);
        } break;

        // --- TAMBAHKAN INI: Paksa gambar ulang saat di-resize ---
        case WM_SIZE: {
            InvalidateRect(hwnd, NULL, TRUE);
        } break;

        case WM_ERASEBKGND:
            return 1; // Beritahu Windows kita urus sendiri hapus background-nya

        case WM_DESTROY: PostQuitMessage(0); break;
        default: return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void Overlay::start(const std::string& title, int w, int h) {
    WNDCLASS wc = { 0 };
    // --- TAMBAHKAN STYLE INI: Penting agar tidak berbayang ---
    wc.style = CS_HREDRAW | CS_VREDRAW; 
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "ChessOverlayFrame";
    wc.hCursor = LoadCursor(NULL, IDC_SIZEALL);
    RegisterClass(&wc);

    // ... sisa kode CreateWindowEx tetap sama ...
    hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED,
        wc.lpszClassName, title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, w, h,
        NULL, NULL, wc.hInstance, NULL
    );

    SetLayeredWindowAttributes(hwnd, RGB(255, 0, 255), 0, LWA_COLORKEY);
    ShowWindow(hwnd, SW_SHOW);
    
    isReady = true;
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

RECT Overlay::getFrameRect() {
    RECT rect = {0};
    if (hwnd) {
        GetWindowRect(hwnd, &rect);
        rect.left += 8; rect.top += 31; // Penyesuaian border standar Windows
        rect.right -= 8; rect.bottom -= 8;
    }
    return rect;
}