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
    # 1. Resize dulu ke ukuran standar (64x64)
    gray = cv2.cvtColor(cell, cv2.COLOR_BGR2GRAY)
    gray = cv2.resize(gray, (64, 64))
    
    # 2. Buat rontgen/kerangka garis putih
    edges = cv2.Canny(gray, 40, 120)
    
    # =======================================================
    # LOGIKA BARU SESUAI IDE KAMU: FOKUS RADIUS TENGAH!
    # Kita buang 12 piksel dari atas, bawah, kiri, dan kanan.
    # Gambar sekarang menyusut jadi 40x40 piksel (HANYA AREA TENGAH).
    # =======================================================
    gray_center = gray[12:52, 12:52]
    edges_center = edges[12:52, 12:52]
    
    return gray_center, edges_center 

def is_empty(gray_center, edges_center):
    # Karena area gambarnya mengecil, threshold kita turunkan drastis
    if cv2.countNonZero(edges_center) < 15:
        return True
        
    if np.std(gray_center) < 15: 
        return True
        
    return False

def get_piece_color(gray_center):
    # Karena ukuran gambar sekarang 40x40, kita hitung langsung dari keseluruhan area tengah ini
    dark_pixels = np.sum(gray_center < 80)
    light_pixels = np.sum(gray_center > 175)
    
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
            gray, edges = preprocess(cell)
            
            shape_label = label.upper() 
            
            if shape_label not in templates_dict:
                templates_dict[shape_label] = edges
                # Simpan agar kamu bisa lihat ukuran barunya yang fokus di tengah
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
        gray, edges = preprocess(cell)

        if is_empty(gray, edges):
            board.append(".")
            continue

        shape, score = match(edges, templates)

        # =======================================================
        # PERMINTAAN USER: "Pokoknya ada kemiripan sedikit aja udah deteksi"
        # Threshold kita banting jadi 0.20 (sangat rendah/toleran).
        # =======================================================
        if score < 0.20: 
            board.append(".")
        else:
            color = get_piece_color(gray)
            if color == "WHITE":
                final_label = shape.upper()
            else:
                final_label = shape.lower()
                
            board.append(final_label)

with open("board.json", "w") as f:
    json.dump(board, f)

print("[INFO] Detection done")