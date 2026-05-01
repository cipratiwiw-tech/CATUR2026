import cv2
import json
import numpy as np
import os

IMG_PATH = "frame.png"
TEMPLATE_CACHE = "templates_cache.npz"
MAPPING_PATH = "template_mapping.json"
DEBUG_DIR = "templates_debug"

if not os.path.exists(DEBUG_DIR):
    os.makedirs(DEBUG_DIR)

def get_cell(img, row, col):
    x1 = col * (img.shape[1] // 8)
    y1 = row * (img.shape[0] // 8)
    return img[y1:y1 + (img.shape[0] // 8), x1:x1 + (img.shape[1] // 8)]

def preprocess(cell):
    gray = cv2.cvtColor(cell, cv2.COLOR_BGR2GRAY)
    gray = cv2.resize(gray, (64, 64))
    edges = cv2.Canny(gray, 40, 120)
    
    # PERUBAHAN: Sekarang kita kembalikan KEDUA gambarnya (gray asli & edges)
    return gray, edges 

def is_empty(cell_edges):
    return cv2.countNonZero(cell_edges) < 30

# =========================
# LOGIKA DETEKSI WARNA (BARU)
# =========================
def get_piece_color(gray_64):
    # Ambil potongan tepat di area tengah bidak (ukuran 24x24 piksel)
    # Ini untuk menghindari warna latar belakang kotak (hijau/coklat)
    center_crop = gray_64[20:44, 20:44]
    
    # Hitung jumlah piksel gelap (hitam) dan terang (putih)
    dark_pixels = np.sum(center_crop < 80)
    light_pixels = np.sum(center_crop > 175)
    
    # Jika piksel terang lebih banyak, berarti itu bidak Putih
    if light_pixels > dark_pixels:
        return "WHITE"
    else:
        return "BLACK"

def build_templates(img):
    if not os.path.exists(MAPPING_PATH):
        return None

    with open(MAPPING_PATH, "r") as f:
        mapping = json.load(f)

    templates_dict = {}

    for i in range(64):
        label = mapping[i]
        if label != ".":
            r, c = i // 8, i % 8
            cell = get_cell(img, r, c)
            gray, edges = preprocess(cell) # Sesuaikan return value
            
            # Kita menggunakan .upper() agar template yang disimpan 
            # murni hanya mewakili BENTUK saja (tanpa peduli warna)
            shape_label = label.upper() 
            
            if shape_label not in templates_dict:
                templates_dict[shape_label] = edges
                cv2.imwrite(os.path.join(DEBUG_DIR, f"{shape_label}.png"), edges)

    np.savez(TEMPLATE_CACHE, **templates_dict)
    return templates_dict

def load_templates():
    if os.path.exists(TEMPLATE_CACHE):
        data = np.load(TEMPLATE_CACHE, allow_pickle=True)
        return {key: data[key] for key in data.files}
    return None

def match(cell_edges, templates_dict):
    best_score = -1
    best_shape = "."

    for shape_label, t_img in templates_dict.items():
        res = cv2.matchTemplate(cell_edges, t_img, cv2.TM_CCOEFF_NORMED)
        score = res[0][0]

        if score > best_score:
            best_score = score
            best_shape = shape_label

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
        gray, edges = preprocess(cell) # Sesuaikan return value

        if is_empty(edges):
            board.append(".")
            continue

        # 1. Deteksi BENTUK (Apakah ini Pion, Kuda, atau Raja?)
        shape, score = match(edges, templates)

        if score < 0.35: 
            board.append(".")
        else:
            # 2. Deteksi WARNA (Cek kecerahan di tengah gambar asli)
            color = get_piece_color(gray)
            
            # 3. Format FEN: Huruf BESAR untuk Putih, huruf kecil untuk Hitam
            if color == "WHITE":
                final_label = shape.upper()
            else:
                final_label = shape.lower()
                
            board.append(final_label)

with open("board.json", "w") as f:
    json.dump(board, f)

print("[INFO] Detection done")