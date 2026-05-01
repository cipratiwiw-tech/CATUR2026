#pragma once
#include <windows.h>
#include <string>
#include <atomic>
#include <map>

// ID Menu Lengkap[cite: 11]
enum PieceID {
    ID_EMPTY = 0,
    ID_WP, ID_WN, ID_WB, ID_WR, ID_WQ, ID_WK, // Putih (Besar)
    ID_BP, ID_BN, ID_BB, ID_BR, ID_BQ, ID_BK  // Hitam (Kecil)
};

class Overlay {
public:
    Overlay();
    ~Overlay();
    void start(const std::string& title, int w, int h);
    RECT getFrameRect();
    std::atomic<bool> isReady{ false };
    std::atomic<bool> isAnalyzing{ false }; // Penanda tombol diklik
    std::string currentFEN = ""; // Menyimpan teks FEN untuk digambar
    
    std::string boardState[64]; // Menyimpan karakter FEN[cite: 11]
    int lastClickedIndex = -1;
    HWND hwnd;

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void showContextMenu(HWND hwnd, int x, int y);
};