import cv2
import json
import numpy as np
import os

IMG_PATH = "frame.png"
TEMPLATE_CACHE = "templates_cache_v3.npz" # Ubah nama agar memaksa pembuatan ulang cache template baru
MAPPING_PATH = "template_mapping.json"
DEBUG_DIR = "templates_debug"
BOARD_JSON = "board.json"

if not os.path.exists(DEBUG_DIR):
    os.makedirs(DEBUG_DIR)

def get_cell(img, row, col):
    h, w = img.shape[:2]
    cell_w = w // 8
    cell_h = h // 8
    x1 = col * cell_w
    y1 = row * cell_h
    return img[y1:y1 + cell_h, x1:x1 + cell_w]

def preprocess(cell):
    # 1. Ubah ke Grayscale
    gray = cv2.cvtColor(cell, cv2.COLOR_BGR2GRAY)
    
    # 2. Pengaman jika frame di-resize terlalu kecil
    h, w = gray.shape
    if w <= 10 or h <= 10:
        return np.zeros((64, 64), dtype=np.uint8)

    # 3. Crop 12% dari tiap sisi untuk membuang garis papan / bingkai highlight "last move"
    crop_x = int(w * 0.12)
    crop_y = int(h * 0.12)
    gray_safe = gray[crop_y:h-crop_y, crop_x:w-crop_x]
    
    # 4. Resize ke 64x64 agar seragam untuk masking
    gray_std = cv2.resize(gray_safe, (64, 64))
    
    # 5. Hapus noise huruf/angka (koordinat papan di pojok) dengan meniban warna dominan
    median_bg = np.median(gray_std)
    gray_std[0:14, 0:14] = median_bg   # Pojok Kiri Atas
    gray_std[-14:, -14:] = median_bg   # Pojok Kanan Bawah
    gray_std[-14:, 0:14] = median_bg   # Pojok Kiri Bawah
    gray_std[0:14, -14:] = median_bg   # Pojok Kanan Atas
    
    # 6. Kurangi noise dengan Gaussian Blur
    blurred = cv2.GaussianBlur(gray_std, (5, 5), 0)
    
    # 7. Canny Edge dengan Threshold tinggi agar tekstur kayu/background terabaikan
    edges_std = cv2.Canny(blurred, 80, 200)

    return edges_std

def is_empty(edges_std):
    # Karena tekstur dan bingkai sudah dibersihkan, sel kosong batas tepinya pasti sedikit
    return np.count_nonzero(edges_std) < 100

def build_templates(img):
    if not os.path.exists(MAPPING_PATH):
        return None

    with open(MAPPING_PATH, "r") as f:
        mapping = json.load(f)

    # Load cache lama agar saat edit sepotong-sepotong template lama tidak hilang tereset
    templates_dict = load_templates()
    if templates_dict is None:
        templates_dict = {}

    for i in range(64):
        label = mapping[i]
        
        if label != "." and label != "E":
            r, c = i // 8, i % 8
            cell = get_cell(img, r, c)
            processed_img = preprocess(cell)
            
            # Hanya simpan atau timpa(overwrite) jika bidak itu di atas kotak yang TIDAK KOSONG
            if not is_empty(processed_img):
                templates_dict[label] = processed_img
                cv2.imwrite(os.path.join(DEBUG_DIR, f"{label}.png"), processed_img)

    np.savez(TEMPLATE_CACHE, **templates_dict)
    return templates_dict

def load_templates():
    if os.path.exists(TEMPLATE_CACHE):
        data = np.load(TEMPLATE_CACHE, allow_pickle=True)
        return {key: data[key] for key in data.files}
    return None

def match(processed_img, templates_dict):
    best_score = -1
    best_shape = "."

    # Langsung tes ke semua template variasi bidak yang tersimpan
    for key, t_img in templates_dict.items():
        res = cv2.matchTemplate(processed_img, t_img, cv2.TM_CCOEFF_NORMED)
        score = res[0][0]

        if score > best_score:
            best_score = score
            best_shape = key # Ambil huruf aslinya langsung

    return best_shape, best_score

# =========================
# MAIN EXECUTION
# =========================
img = cv2.imread(IMG_PATH)
if img is None: exit()

templates = load_templates()

if templates is None or (os.path.exists(MAPPING_PATH) and os.path.getmtime(MAPPING_PATH) > os.path.getmtime(TEMPLATE_CACHE)):
    templates = build_templates(img)

board = []

for r in range(8):
    for c in range(8):
        cell = get_cell(img, r, c)
        processed_img = preprocess(cell)

        if is_empty(processed_img):
            board.append(".")
            continue

        shape, score = match(processed_img, templates)

        # Threshold untuk Canny Edge matching ditingkatkan agar aman dari sisa noise/ilusi bentuk
        if score < 0.55: 
            board.append(".")
        else:
            board.append(shape) # Langsung masukkan hurufnya, tidak perlu cek warna lagi!

with open(BOARD_JSON, "w") as f:
    json.dump(board, f)

print("[INFO] Detection done")