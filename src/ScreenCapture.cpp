#include "ScreenCapture.h"

ScreenCapture::ScreenCapture() {
    // Mendapatkan akses ke seluruh layar desktop Windows
    hwndDesktop = GetDesktopWindow();
}

ScreenCapture::~ScreenCapture() {}

cv::Mat ScreenCapture::captureRegion(int x, int y, int width, int height) {
    HDC hwindowDC = GetDC(hwndDesktop);
    HDC hwindowCompatibleDC = CreateCompatibleDC(hwindowDC);
    SetStretchBltMode(hwindowCompatibleDC, COLORONCOLOR);

    // Membuat kanvas kosong di memori
    HBITMAP hbwindow = CreateCompatibleBitmap(hwindowDC, width, height);
    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = width;
    bi.biHeight = -height;  // Negatif agar gambar tidak terbalik
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    // Menyalin piksel layar ke kanvas kosong tadi
    SelectObject(hwindowCompatibleDC, hbwindow);
    BitBlt(hwindowCompatibleDC, 0, 0, width, height, hwindowDC, x, y, SRCCOPY);

    // Mengubah kanvas Windows menjadi format gambar OpenCV (cv::Mat)
    cv::Mat mat(height, width, CV_8UC4);
    GetDIBits(hwindowCompatibleDC, hbwindow, 0, height, mat.data, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    // Membersihkan memori agar RAM tidak bocor (Memory Leak)
    DeleteObject(hbwindow);
    DeleteDC(hwindowCompatibleDC);
    ReleaseDC(hwndDesktop, hwindowDC);

    return mat;
}