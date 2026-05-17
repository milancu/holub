import time
import requests
import numpy as np
import onnxruntime as ort
from io import BytesIO
from PIL import Image

ESP32_URL = "http://holub.local"
SNAPSHOT_ENDPOINT = f"{ESP32_URL}/snapshot"
SPRAY_ENDPOINT = f"{ESP32_URL}/spray"

CONFIDENCE_THRESHOLD = 0.5
CHECK_INTERVAL_S = 1.0
BIRD_CLASS_ID = 14  # COCO "bird" class
MODEL_PATH = "yolov8n.onnx"
INPUT_SIZE = 640


def load_model():
    session = ort.InferenceSession(MODEL_PATH)
    return session


def preprocess(image):
    img = image.convert("RGB").resize((INPUT_SIZE, INPUT_SIZE))
    arr = np.array(img, dtype=np.float32) / 255.0
    arr = arr.transpose(2, 0, 1)  # HWC -> CHW
    arr = np.expand_dims(arr, axis=0)  # add batch dim
    return arr


def postprocess(output):
    # YOLOv8 ONNX output shape: (1, 84, 8400) — transposed to (8400, 84)
    preds = output[0][0].T  # (8400, 84)
    # columns: x, y, w, h, class_scores[80]
    class_scores = preds[:, 4:]
    bird_scores = class_scores[:, BIRD_CLASS_ID]
    max_score = float(bird_scores.max())
    return max_score >= CONFIDENCE_THRESHOLD, max_score


def grab_snapshot():
    resp = requests.get(SNAPSHOT_ENDPOINT, timeout=5)
    resp.raise_for_status()
    return Image.open(BytesIO(resp.content))


def spray():
    try:
        resp = requests.post(SPRAY_ENDPOINT, params={"ms": 500}, timeout=5)
        if resp.status_code == 200:
            print("[spray] pumpa aktivovana")
        elif resp.status_code == 429:
            print("[spray] cooldown, preskakuji")
        else:
            print(f"[spray] neocekavany status: {resp.status_code}")
    except requests.RequestException as e:
        print(f"[spray] chyba: {e}")


def main():
    print("[detector] startuji — nacitam model...")
    session = load_model()
    input_name = session.get_inputs()[0].name
    print(f"[detector] model nacten, input: {input_name}")

    print("[detector] cekam na ESP32-CAM...")
    while True:
        try:
            requests.get(f"{ESP32_URL}/status", timeout=3)
            break
        except requests.RequestException:
            time.sleep(2)

    print("[detector] ESP32-CAM dostupna, zacinam detekci")

    while True:
        try:
            image = grab_snapshot()
            input_data = preprocess(image)
            outputs = session.run(None, {input_name: input_data})
            found, conf = postprocess(outputs)
            if found:
                print(f"[detector] HOLUB detekovan (conf={conf:.2f}) — spoustim spray")
                spray()
        except requests.RequestException as e:
            print(f"[detector] chyba spojeni: {e}")
        except Exception as e:
            print(f"[detector] chyba: {e}")

        time.sleep(CHECK_INTERVAL_S)


if __name__ == "__main__":
    main()
