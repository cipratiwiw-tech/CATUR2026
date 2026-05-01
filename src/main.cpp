#define NOMINMAX
#include <windows.h>
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <map>
#include "Overlay.h"
#include "ScreenCapture.h"
#include "Vision.h"

int main() {
    SetProcessDPIAware();
    Overlay frame;
    
    std::thread t([&]() { frame.start("EDITOR BIDAK CATUR", 450, 450); });
    t.detach();

    while (!frame.isReady) { Sleep(10); }

    ScreenCapture capture;
    Vision vision;

    // TODO: Siapkan dan muat gambar template bidak catur Anda ke dalam map ini.
    // Kamu butuh potongan gambar .png dari masing-masing bidak untuk dicocokkan.
    // Contoh:
    // std::map<std::string, cv::Mat> pieceTemplates;
    // pieceTemplates["P"] = cv::imread("templates/white_pawn.png", cv::IMREAD_COLOR);
    // pieceTemplates["p"] = cv::imread("templates/black_pawn.png", cv::IMREAD_COLOR);
    // ...

    while (true) {
        // Jika menu belum diklik, aplikasi cukup "tidur" dan cek kembali
        if (!frame.isAnalyzing && !frame.showPieceSelector) {
            Sleep(100);
            continue;
        }

        // 1. Dapatkan posisi & dimensi kotak grid dari jendela overlay layar
        RECT clientRect;
        GetClientRect(frame.hwnd, &clientRect);
        
        int padding = 20; // Sesuai dengan ukuran padding yang digambar di Overlay.cpp
        int barWidth = 20;
        int barGap = 15;
        POINT topLeft = { clientRect.left + padding, clientRect.top + padding };
        POINT bottomRight = { clientRect.right - padding - barWidth - barGap, clientRect.bottom - padding };
        
        ClientToScreen(frame.hwnd, &topLeft);
        ClientToScreen(frame.hwnd, &bottomRight);
        
        int width = bottomRight.x - topLeft.x;
        int height = bottomRight.y - topLeft.y;

        if (width > 0 && height > 0) {
            // 2. Ambil screenshot murni dari area papan catur yang ditargetkan
            cv::Mat boardImage = capture.captureRegion(topLeft.x, topLeft.y, width, height);
            
            // Tampilkan POPUP SCREENSHOT jika menu Pilih Bidak diklik
            if (frame.showPieceSelector) {
                // Buat jendelanya terlebih dahulu secara eksplisit
                cv::namedWindow("Screenshot Area Grid", cv::WINDOW_AUTOSIZE);
                cv::setWindowProperty("Screenshot Area Grid", cv::WND_PROP_TOPMOST, 1); // Memaksa OpenCV merender di paling depan
                
                // Kunci (disable) jendela utama agar tidak bisa diklik
                EnableWindow(frame.hwnd, FALSE);

                // Memastikan popup bertindak sebagai dialog (anak) dari jendela utama Overlay
                HWND popupHwnd = FindWindowA(NULL, "Screenshot Area Grid");
                if (popupHwnd) {
                    SetWindowLongPtrA(popupHwnd, GWLP_HWNDPARENT, (LONG_PTR)frame.hwnd);
                }
                
                cv::imshow("Screenshot Area Grid", boardImage);
                
                // Tahan popup sampai user menekan tombol di keyboard atau menekan tombol 'X'
                while (true) {
                    if (cv::waitKey(100) >= 0) break; // Sembarang tombol ditekan
                    
                    try {
                        // Periksa apakah jendela masih ada (belum ditutup manual via tombol 'X')
                        if (cv::getWindowProperty("Screenshot Area Grid", cv::WND_PROP_VISIBLE) < 1) {
                            break;
                        }
                    } catch (...) {
                        break; // Jendela sudah tidak ditemukan
                    }
                }
                
                // Buka kembali kunci jendela utama TERLEBIH DAHULU sebelum menutup popup
                EnableWindow(frame.hwnd, TRUE);
                SetForegroundWindow(frame.hwnd);
                
                // Tutup popup secara aman, abaikan jika jendela sudah terlanjur hancur
                try { cv::destroyWindow("Screenshot Area Grid"); } catch (...) {}
                
                frame.showPieceSelector = false; // Reset status penanda
            }

            // 3. Bagi gambar ke dalam grid 8x8 kotak
            std::vector<ChessSquare> grid = vision.createGrid(cv::Point(0, 0), width);

            // 4. Analisa masing-masing kotak (Kode pencocokan template bidak)
            /*
            for (int i = 0; i < 64; i++) {
                cv::Mat squareImg = boardImage(grid[i].area);
                std::string detectedPiece = ".";
                
                // Loop melalui `pieceTemplates` dan periksa tiap template.
                // Gunakan fungsi vision.findTemplate(squareImg, templateImg).
                // Jika template cocok dan skor melebihi batas, perbarui `detectedPiece`.
                
                frame.boardState[i] = detectedPiece; // Update papan state
            }
            InvalidateRect(frame.hwnd, NULL, TRUE); // Refresh render overlay
            */
        }

        // Hanya update logika FEN jika mode Analyze AKTIF
        if (frame.isAnalyzing) {
            std::string fen = "";
            for (int i = 0; i < 8; i++) {
                int emptyCount = 0;
                for (int j = 0; j < 8; j++) {
                    std::string p = frame.boardState[i * 8 + j];
                    if (p == ".") {
                        emptyCount++;
                    } else {
                        if (emptyCount > 0) { 
                            fen += std::to_string(emptyCount); 
                            emptyCount = 0; 
                        }
                        fen += p;
                    }
                }
                if (emptyCount > 0) fen += std::to_string(emptyCount);
                if (i < 7) fen += "/";
            }
            
            // 5. Update FEN ke Overlay UI agar tampil di jendela aplikasi
            frame.currentFEN = fen + " w - - 0 1";
            InvalidateRect(frame.hwnd, NULL, TRUE); // Refresh tampilan overlay
            
            Sleep(1000); // Jeda 1 detik agar screen capture tidak memberatkan performa laptop
        }
    }
    return 0;
}