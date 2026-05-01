#include "ScreenCapture.h"
#include <iostream>

ScreenCapture::ScreenCapture() {
    targetHwnd = GetDesktopWindow();
}

ScreenCapture::~ScreenCapture() {}

// Struktur bantuan untuk mencari jendela
struct SearchData {
    std::string targetTitle;
    HWND foundHwnd;
};

// Fungsi internal Windows untuk mengecek semua jendela yang terbuka
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    SearchData* data = (SearchData*)lParam;
    char title[256];
    GetWindowTextA(hwnd, title, sizeof(title));
    std::string windowTitle(title);

    // Jika judul jendela mengandung kata kunci dan jendelanya sedang tampil
    if (windowTitle.find(data->targetTitle) != std::string::npos && IsWindowVisible(hwnd)) {
        data->foundHwnd = hwnd;
        return FALSE; // Berhenti mencari karena sudah ketemu
    }
    return TRUE; // Lanjut cari jendela berikutnya
}

bool ScreenCapture::findWindowRect(const std::string& windowName, RECT& rect) {
    SearchData data = { windowName, NULL };
    
    // Cari jendela secara otomatis di sistem
    EnumWindows(EnumWindowsProc, (LPARAM)&data);

    if (data.foundHwnd) {
        targetHwnd = data.foundHwnd;
        
        // Cetak judul jendela yang ditemukan biar kita yakin
        char finalTitle[256];
        GetWindowTextA(targetHwnd, finalTitle, sizeof(finalTitle));
        std::cout << "Target Ditemukan: [" << finalTitle << "]" << std::endl;

        GetWindowRect(targetHwnd, &rect);
        return true;
    }
    return false;
}

cv::Mat ScreenCapture::captureRegion(int x, int y, int width, int height) {
    // Kode capture tetap sama dengan yang sebelumnya
    HDC hwindowDC = GetDC(GetDesktopWindow());
    HDC hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
    HBITMAP hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
    BITMAPINFOHEADER bi = { 0 };
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width; bi.biHeight = -height;
    bi.biPlanes = 1; bi.biBitCount = 32; bi.biCompression = BI_RGB;
    SelectObject(hwindowCompatibleDC, hbwindow);
    BitBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, x, y, SRCCOPY);
    cv::Mat mat(height, width, CV_8UC4);
    GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);
    DeleteObject(hbwindow);
    DeleteDC(hwindowCompatibleDC);
    ReleaseDC(GetDesktopWindow(), hwindowDC);
    return mat;
}