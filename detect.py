import cv2
import json
import os
from ultralytics import YOLO

# Konfigurasi Path
IMG_PATH = "frame.png"
BOARD_JSON = "board.json"
MODEL_PATH = "yolov8m-chess-piece-detection.pt" # GANTI dengan path model YOLO-mu

def main():
    # 1. Inisiasi papan kosong 8x8 (diisi string ".")
    board = [["." for _ in range(8)] for _ in range(8)]
    
    # 2. Cek ketersediaan gambar dan model
    if not os.path.exists(IMG_PATH):
        print("[ERROR] frame.png tidak ditemukan.")
        return
        
    try:
        model = YOLO(MODEL_PATH)
    except Exception as e:
        print(f"[ERROR] Gagal memuat model YOLO: {e}")
        return

    # 3. Baca gambar untuk mendapatkan dimensi asli
    img = cv2.imread(IMG_PATH)
    img_h, img_w = img.shape[:2]
    
    # Hitung lebar dan tinggi per kotak catur (karena papan dibagi 8x8)
    cell_w = img_w / 8.0
    cell_h = img_h / 8.0

    # 4. Jalankan inferensi YOLO
    # conf=0.5 berarti hanya deteksi dengan keyakinan di atas 50% yang diambil
    results = model.predict(source=IMG_PATH, conf=0.5, save=False, verbose=False)
    
    # 5. Proses hasil deteksi YOLO
    # Asumsi model kamu punya class names seperti: 0: 'P', 1: 'N', 2: 'B', 3: 'R', 4: 'Q', 5: 'K', 6: 'p', 7: 'n', 8: 'b', 9: 'r', 10: 'q', 11: 'k'
    names = model.names
    
    for box in results[0].boxes:
        # Dapatkan koordinat bounding box
        x1, y1, x2, y2 = box.xyxy[0].tolist()
        
        # Hitung titik tengah (center) dari bidak
        cx = (x1 + x2) / 2.0
        cy = (y1 + y2) / 2.0
        
        # Mapping koordinat piksel ke index grid 8x8
        col = int(cx // cell_w)
        row = int(cy // cell_h)
        
        # Pastikan tidak out of bounds
        if 0 <= col < 8 and 0 <= row < 8:
            class_id = int(box.cls[0].item())
            piece_label = names[class_id]
            
            # Timpa kotak kosong dengan label bidak
            board[row][col] = piece_label

    # 6. Ubah format dari array 2D (8x8) menjadi array 1D (64 elemen) 
    # karena fungsi loadBoardFromJson di C++ membacanya secara linear (0 sampai 63)
    flat_board = []
    for r in range(8):
        for c in range(8):
            flat_board.append(board[r][c])

    # 7. Simpan ke JSON agar bisa dibaca oleh aplikasi C++
    with open(BOARD_JSON, "w") as f:
        json.dump(flat_board, f)

    print("[INFO] Deteksi YOLO Selesai")

if __name__ == "__main__":
    main()