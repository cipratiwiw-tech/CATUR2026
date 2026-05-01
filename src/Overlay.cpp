#include "Overlay.h"
#include <map>

Overlay* g_overlay = nullptr;

Overlay::Overlay() { 
    g_overlay = this; 
    for(int i = 0; i < 64; i++) boardState[i] = "."; 
}

Overlay::~Overlay() { 
    if (hwnd) DestroyWindow(hwnd); 
}

void Overlay::showContextMenu(HWND hwnd, int x, int y) {
    HMENU hMenu = CreatePopupMenu();
    HMENU hWhite = CreatePopupMenu();
    HMENU hBlack = CreatePopupMenu();

    AppendMenu(hMenu, MF_STRING, ID_EMPTY, "Kosongkan Kotak");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);

    AppendMenu(hWhite, MF_STRING, ID_WP, "Pion (P)"); 
    AppendMenu(hWhite, MF_STRING, ID_WN, "Kuda (N)");
    AppendMenu(hWhite, MF_STRING, ID_WB, "Gajah (B)"); 
    AppendMenu(hWhite, MF_STRING, ID_WR, "Benteng (R)");
    AppendMenu(hWhite, MF_STRING, ID_WQ, "Ster (Q)"); 
    AppendMenu(hWhite, MF_STRING, ID_WK, "Raja (K)");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hWhite, "Pasang Putih");

    AppendMenu(hBlack, MF_STRING, ID_BP, "pion (p)"); 
    AppendMenu(hBlack, MF_STRING, ID_BN, "kuda (n)");
    AppendMenu(hBlack, MF_STRING, ID_BB, "gajah (b)"); 
    AppendMenu(hBlack, MF_STRING, ID_BR, "benteng (r)");
    AppendMenu(hBlack, MF_STRING, ID_BQ, "ster (q)"); 
    AppendMenu(hBlack, MF_STRING, ID_BK, "raja (k)");
    AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hBlack, "Pasang Hitam");

    POINT pt = { x, y };
    ClientToScreen(hwnd, &pt);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    DestroyMenu(hWhite); 
    DestroyMenu(hBlack); 
    DestroyMenu(hMenu);
}

LRESULT CALLBACK Overlay::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_ERASEBKGND: 
            return 1; // KUNCI TRANSPARANSI

        case WM_RBUTTONDOWN: {
            int x = LOWORD(lParam); 
            int y = HIWORD(lParam);
            RECT r; 
            GetClientRect(hwnd, &r);
            
            // Tentukan jarak pinggir yang sama dengan yang digambar
            int padding = 20; 
            int barWidth = 20;
            int barGap = 15;
            int gridW = r.right - (2 * padding) - barWidth - barGap;
            int gridH = r.bottom - (2 * padding);

            // Cek apakah klik berada tepat di dalam area grid (bukan di area kosong pinggiran)
            if (x >= padding && x <= padding + gridW && 
                y >= padding && y <= padding + gridH) {
                
                int cellW = gridW / 8;
                int cellH = gridH / 8;

                if (cellW > 0 && cellH > 0) {
                    // Kurangi koordinat x dan y dengan padding agar kalkulasi index tepat
                    int col = (x - padding) / cellW;
                    int row = (y - padding) / cellH;
                    
                    // Pastikan batas array aman
                    if (col > 7) col = 7;
                    if (row > 7) row = 7;

                    g_overlay->lastClickedIndex = (row * 8) + col;
                    g_overlay->showContextMenu(hwnd, x, y);
                }
            }
        } break;

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            int idx = g_overlay->lastClickedIndex;
            if (idx >= 0 && idx < 64) {
                std::map<int, std::string> pieces = {
                    {ID_EMPTY, "."}, {ID_WP, "P"}, {ID_WN, "N"}, {ID_WB, "B"}, {ID_WR, "R"}, {ID_WQ, "Q"}, {ID_WK, "K"},
                    {ID_BP, "p"}, {ID_BN, "n"}, {ID_BB, "b"}, {ID_BR, "r"}, {ID_BQ, "q"}, {ID_BK, "k"}
                };
                if (pieces.count(id)) g_overlay->boardState[idx] = pieces[id];
                InvalidateRect(hwnd, NULL, TRUE);
            }
            
            // Tangkap klik tombol "Pilih Bidak" dari Menu Bar
        if (id == 9001) {
            g_overlay->showPieceSelector = true;
        }
        
            // Tangkap klik tombol "Start Analyze" dari Menu Bar
            if (id == 9002) {
                g_overlay->isAnalyzing = !g_overlay->isAnalyzing; // Aktifkan/Matikan mode
                InvalidateRect(hwnd, NULL, TRUE);
            }
        } break;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT r; 
            GetClientRect(hwnd, &r);
            
            // 1. Gambar Background Magenta
            HBRUSH hbr = CreateSolidBrush(RGB(255, 0, 255));
            FillRect(hdc, &r, hbr);
            DeleteObject(hbr);

            // --- DEKLARASI PADDING ---
            int padding = 20; // 20 piksel jarak dari batas aplikasi ke grid catur
            int barWidth = 20;
            int barGap = 15;
            int gridW = r.right - (2 * padding) - barWidth - barGap;
            int gridH = r.bottom - (2 * padding);

            // --- GAMBAR INSTRUKSI & FEN DI JENDELA ---
            SetBkMode(hdc, TRANSPARENT);
            SetTextColor(hdc, RGB(255, 255, 0)); // Warna teks kuning agar kontras
            std::string infoText = g_overlay->isAnalyzing 
                ? "Analisa AKTIF | FEN: " + g_overlay->currentFEN 
                : "1. Paskan grid | 2. Klik Kanan = Pasang Bidak | 3. Menu -> Start Analyze";
            
            RECT textRect = { 5, 2, r.right - 5, padding };
            HFONT hFont = CreateFont(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Arial");
            SelectObject(hdc, hFont);
            DrawTextA(hdc, infoText.c_str(), -1, &textRect, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
            DeleteObject(hFont);

            // 2. Gambar Grid Hijau (Hanya digambar jika jendela tidak terlalu kecil)
            if (gridW > 0 && gridH > 0) {
                int cellW = gridW / 8;
                int cellH = gridH / 8;

                HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 255, 0));
                SelectObject(hdc, hPen);
                SelectObject(hdc, GetStockObject(HOLLOW_BRUSH)); 
                SetBkMode(hdc, TRANSPARENT);
                SetTextColor(hdc, RGB(255, 255, 255));

                for(int i = 0; i < 8; i++) {
                    for(int j = 0; j < 8; j++) {
                        // Kalkulasi posisi x dan y ditambah padding
                        int x = padding + (cellW * j);
                        int y = padding + (cellH * i);
                        Rectangle(hdc, x, y, x + cellW, y + cellH);
                        
                        std::string p = g_overlay->boardState[i * 8 + j];
                        if (p != ".") {
                            TextOutA(hdc, x + 15, y + 10, p.c_str(), (int)p.length());
                        }
                    }
                }
                DeleteObject(hPen);

                // --- GAMBAR EVALUATION BAR ---
                int barX = padding + gridW + barGap;
                int barY = padding;
                
                // Kalkulasi proporsi kemenangan berdasarkan variabel currentEval
                float eval = g_overlay->currentEval;
                if (eval > 10.0f) eval = 10.0f;
                if (eval < -10.0f) eval = -10.0f;
                float normalized = (eval + 10.0f) / 20.0f; 
                
                int whiteHeight = (int)(gridH * normalized);
                int blackHeight = gridH - whiteHeight;
                
                RECT blackRect = { barX, barY, barX + barWidth, barY + blackHeight };
                HBRUSH blackBrush = CreateSolidBrush(RGB(40, 40, 40)); 
                FillRect(hdc, &blackRect, blackBrush);
                DeleteObject(blackBrush);
                
                RECT whiteRect = { barX, barY + blackHeight, barX + barWidth, barY + gridH };
                HBRUSH whiteBrush = CreateSolidBrush(RGB(240, 240, 240)); 
                FillRect(hdc, &whiteRect, whiteBrush);
                DeleteObject(whiteBrush);
                
                HPEN barPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
                SelectObject(hdc, barPen);
                SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
                Rectangle(hdc, barX, barY, barX + barWidth, barY + gridH);
                DeleteObject(barPen);
            }

            // 3. Bingkai Resizer (Tidak Terlihat) di ujung jendela
            // Karena grid ditarik ke dalam sejauh 20px, bingkai 15px ini tidak akan bertabrakan dengan grid hijau.
            HPEN hEdgePen = CreatePen(PS_INSIDEFRAME, 15, RGB(1, 1, 1)); 
            SelectObject(hdc, hEdgePen);
            SelectObject(hdc, GetStockObject(HOLLOW_BRUSH)); 
            Rectangle(hdc, 0, 0, r.right, r.bottom);
            DeleteObject(hEdgePen);

            EndPaint(hwnd, &ps);
        } break;
        
        case WM_SIZE: InvalidateRect(hwnd, NULL, TRUE); break;
        case WM_DESTROY: PostQuitMessage(0); break;

        case WM_NCHITTEST: {
            // Ambil koordinat mouse dari lParam (koordinat layar penuh)
            // Di-cast ke (short) agar tetap aman jika kamu pakai lebih dari 1 monitor (koordinat bisa negatif)
            int x = (short)LOWORD(lParam);
            int y = (short)HIWORD(lParam);

            // Dapatkan dimensi jendela saat ini
            RECT rect;
            GetWindowRect(hwnd, &rect);

            // Tentukan margin/ketebalan area hover (dalam piksel). 
            // Angka 15 cukup lebar agar gampang di-hover tanpa harus presisi.
            const int margin = 15; 

            bool onLeft = (x >= rect.left && x < rect.left + margin);
            bool onRight = (x < rect.right && x >= rect.right - margin);
            bool onTop = (y >= rect.top && y < rect.top + margin);
            bool onBottom = (y < rect.bottom && y >= rect.bottom - margin);

            // Beri tahu Windows bagian mana yang sedang di-hover agar kursor otomatis berubah
            if (onTop && onLeft) return HTTOPLEFT;
            if (onTop && onRight) return HTTOPRIGHT;
            if (onBottom && onLeft) return HTBOTTOMLEFT;
            if (onBottom && onRight) return HTBOTTOMRIGHT;
            if (onTop) return HTTOP;
            if (onBottom) return HTBOTTOM;
            if (onLeft) return HTLEFT;
            if (onRight) return HTRIGHT;

            // Jika mouse tidak berada di margin buatan kita, biarkan Windows menanganinya seperti biasa
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        } break;

        default: return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void Overlay::start(const std::string& title, int w, int h) {
    WNDCLASS wc = { 0 };
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "ChessOverlayFrame";
    wc.hCursor = LoadCursor(NULL, IDC_SIZEALL);
    wc.hbrBackground = NULL; 
    RegisterClass(&wc);

    // --- MEMBUAT MENU BAR ---
    HMENU hMenuBar = CreateMenu();
    
    // Kita buat ID sementara 9001 dan 9002 untuk nanti ditangkap di WM_COMMAND
    AppendMenu(hMenuBar, MF_STRING, 9001, "Pilih Bidak");
    AppendMenu(hMenuBar, MF_STRING, 9002, "Start Analyze");
    // ------------------------

    hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED, 
        wc.lpszClassName, 
        title.c_str(), 
        WS_OVERLAPPEDWINDOW, 
        CW_USEDEFAULT, CW_USEDEFAULT, w, h, 
        NULL, 
        hMenuBar, // <-- PASANG MENU BAR DI SINI
        wc.hInstance, 
        NULL
    );

    // Kunci Transparansi
    SetLayeredWindowAttributes(hwnd, RGB(255, 0, 255), 0, LWA_COLORKEY);
    
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd); 
    isReady = true;
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}