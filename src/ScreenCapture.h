#pragma once
#include <windows.h>
#include <opencv2/opencv.hpp>
#include <string>

class ScreenCapture {
public:
    ScreenCapture();
    ~ScreenCapture(); // Tambahkan destructor
    
    bool findWindowRect(const std::string& windowName, RECT& rect);
    cv::Mat captureRegion(int x, int y, int width, int height);

private:
    HWND targetHwnd;
};