FROM python:3.11-slim AS exporter

WORKDIR /export
RUN pip install --no-cache-dir ultralytics
RUN python -c "\
from ultralytics import YOLO; \
model = YOLO('yolov8n.pt'); \
model.export(format='onnx', imgsz=640)"

FROM python:3.11-slim

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends \
    libgl1 libglib2.0-0 \
    && rm -rf /var/lib/apt/lists/*

RUN pip install --no-cache-dir \
    onnxruntime \
    opencv-python-headless \
    numpy \
    requests \
    Pillow

COPY --from=exporter /export/yolov8n.onnx .
COPY detector.py .

CMD ["python", "-u", "detector.py"]
