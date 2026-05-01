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

// Struktur dan fungsi Callback untuk Mouse di Jendela Popup
struct MouseCallbackData {
    cv::Mat dialogImg;
    cv::Mat boardImage;
    int width, height;
    std::vector<std::string> pieces;
    std::vector<std::string> pieceKeys;
    std::vector<cv::Rect> buttonRects;
    std::vector<ChessSquare> grid;
    int activePieceIndex = -1;
    bool needsRedraw = false;
};

void onMouse(int event, int x, int y, int flags, void* userdata) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        MouseCallbackData* data = static_cast<MouseCallbackData*>(userdata);
        
        // Cek klik di area tombol (panel kanan)
        for (int i = 0; i < data->buttonRects.size(); i++) {
            if (data->buttonRects[i].contains(cv::Point(x, y))) {
                data->activePieceIndex = i;
                data->needsRedraw = true; // Picu penggambaran ulang UI
                return;
            }
        }
        
        // Cek klik di area grid papan catur (panel kiri)
        if (x >= 0 && x < data->width && y >= 0 && y < data->height) {
            if (data->activePieceIndex != -1) {
                for (const auto& sq : data->grid) {
                    if (sq.area.contains(cv::Point(x, y))) {
                        cv::Mat cellImg = data->boardImage(sq.area);
                        CreateDirectoryA("templates", NULL); // Buat folder templates jika belum ada
                        std::string filename = "templates/" + data->pieceKeys[data->activePieceIndex] + ".png";
                        cv::imwrite(filename, cellImg); // Ekspor gambar!
                        
                        // Efek visual sementara (kotak merah) saat gambar di-crop
                        cv::rectangle(data->dialogImg, sq.area, cv::Scalar(0, 0, 255, 255), 2);
                        cv::imshow("Screenshot Area Grid", data->dialogImg);
                        cv::waitKey(50); // Tahan sebentar
                        
                        data->needsRedraw = true; // Gambar ulang agar kotak merah hilang
                        return;
                    }
                }
            }
        }
    }
}

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
            
            // 3. Bagi gambar ke dalam grid 8x8 kotak (Dipindah agar bisa digunakan di Callback)
            std::vector<ChessSquare> grid = vision.createGrid(cv::Point(0, 0), width);

            // Tampilkan POPUP SCREENSHOT jika menu Pilih Bidak diklik
            if (frame.showPieceSelector) {
                // --- MEMBUAT PANEL UI DI KANAN ---
                int panelWidth = 200;
                int minHeight = 420; // Minimal tinggi agar 12 tombol muat
                int dialogHeight = height > minHeight ? height : minHeight;
                
                // Siapkan kumpulan data untuk dikirim ke event detektor mouse
                MouseCallbackData cbData;
                cbData.width = width;
                cbData.height = height;
                cbData.boardImage = boardImage;
                cbData.grid = grid;
                cbData.dialogImg = cv::Mat(dialogHeight, width + panelWidth, CV_8UC4, cv::Scalar(45, 45, 45, 255));
                 // Gambar list tombol bidak di sisi kanan
                cbData.pieces = {
                    "Putih: Pawn (P)", "Putih: Knight (N)", "Putih: Bishop (B)", 
                    "Putih: Rook (R)", "Putih: Queen (Q)", "Putih: King (K)",
                    "Hitam: Pawn (p)", "Hitam: Knight (n)", "Hitam: Bishop (b)", 
                    "Hitam: Rook (r)", "Hitam: Queen (q)", "Hitam: King (k)"
                };
                cbData.pieceKeys = {"P", "N", "B", "R", "Q", "K", "p", "n", "b", "r", "q", "k"}; // Nama File Referensi FEN

                int startX = width + 10;
                int startY = 10;
                int btnHeight = (dialogHeight - 20) / 12; // Bagi rata tinggi ruang
                if (btnHeight > 35) btnHeight = 35; // Batasi tinggi maksimal tombol
                
                for (int i = 0; i < cbData.pieces.size(); i++) {
                    cbData.buttonRects.push_back(cv::Rect(startX, startY + (i * btnHeight), panelWidth - 20, btnHeight - 5));
                }
                // ---------------------------------
                cbData.needsRedraw = true; // Nyalakan render UI awal
                
                // Buat jendelanya terlebih dahulu secara eksplisit
                cv::namedWindow("Screenshot Area Grid", cv::WINDOW_AUTOSIZE);
                cv::setWindowProperty("Screenshot Area Grid", cv::WND_PROP_TOPMOST, 1); // Memaksa OpenCV merender di paling depan
                
                // Daftarkan event deteksi Mouse
                cv::setMouseCallback("Screenshot Area Grid", onMouse, &cbData);

                // Kunci (disable) jendela utama agar tidak bisa diklik
                EnableWindow(frame.hwnd, FALSE);

                // Memastikan popup bertindak sebagai dialog (anak) dari jendela utama Overlay
                HWND popupHwnd = FindWindowA(NULL, "Screenshot Area Grid");
                if (popupHwnd) {
                    SetWindowLongPtrA(popupHwnd, GWLP_HWNDPARENT, (LONG_PTR)frame.hwnd);
                    // Ubah Kursor Mouse jadi Normal Arrow (Menghilangkan Crosshair bawaan OpenCV)
                    SetClassLongPtrA(popupHwnd, GCLP_HCURSOR, (LONG_PTR)LoadCursor(NULL, IDC_ARROW));
                }
                
                // Tahan popup sampai user menekan tombol di keyboard atau menekan tombol 'X'
                while (true) {
                    // Pengecekan Dinamis: Gambar Ulang Antarmuka Jendela saat Mouse Mengeklik 
                    if (cbData.needsRedraw) {
                        // 1. Reset background UI panel
                        cv::rectangle(cbData.dialogImg, cv::Rect(width, 0, panelWidth, dialogHeight), cv::Scalar(45, 45, 45, 255), cv::FILLED);
                        
                        // 2. Gambar ulang Papan (Biar efek klik sebelumnya terhapus)
                        cv::Mat leftROI = cbData.dialogImg(cv::Rect(0, 0, width, height));
                        boardImage.copyTo(leftROI);
                        
                        // 3. Gambar overlay garis hijau pada Papan agar kamu mudah meng-klik di dalam kotak bidak!
                        for (const auto& sq : grid) {
                            cv::rectangle(cbData.dialogImg, sq.area, cv::Scalar(0, 255, 0, 100), 1);
                        }

                        // 4. Gambar ulang status masing-masing tombol
                        for (int i = 0; i < cbData.pieces.size(); i++) {
                            cv::Rect btn = cbData.buttonRects[i];
                            bool isActive = (i == cbData.activePieceIndex); // Ngecek tombol mana yang diklik
                            
                            // Warna tombol jadi Hijau terang kalau Aktif
                            cv::Scalar btnColor = isActive ? cv::Scalar(0, 200, 0, 255) : 
                                ((i < 6) ? cv::Scalar(200, 200, 200, 255) : cv::Scalar(80, 80, 80, 255));
                            cv::Scalar textColor = (i < 6 && !isActive) ? cv::Scalar(0, 0, 0, 255) : cv::Scalar(255, 255, 255, 255);
                            
                            cv::rectangle(cbData.dialogImg, btn, btnColor, cv::FILLED);
                            
                            int thickness = isActive ? 2 : 1;
                            cv::Scalar borderColor = isActive ? cv::Scalar(0, 255, 0, 255) : cv::Scalar(0, 0, 0, 255);
                            cv::rectangle(cbData.dialogImg, btn, borderColor, thickness);
                            
                            cv::putText(cbData.dialogImg, cbData.pieces[i], cv::Point(btn.x + 10, btn.y + (btn.height / 2) + 5), 
                                        cv::FONT_HERSHEY_SIMPLEX, 0.45, textColor, 1);
                        }
                        
                        cv::imshow("Screenshot Area Grid", cbData.dialogImg);
                        cbData.needsRedraw = false;
                    }

                    if (cv::waitKey(50) >= 0) break; // Delay sengaja dibuat lebih kecil (50ms) agar klik mouse sangat responsif
                    
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