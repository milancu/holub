FROM python:3.11-slim

WORKDIR /app

RUN apt-get update && apt-get install -y --no-install-recommends \
    libgl1 libglib2.0-0 \
    && rm -rf /var/lib/apt/lists/*

COPY --from=ghcr.io/astral-sh/uv:latest /uv /usr/local/bin/uv

COPY pyproject.toml ./
RUN uv sync --no-dev --no-install-project

COPY detector.py .

RUN uv run python -c "from ultralytics import YOLO; YOLO('yolov8n.pt')"

CMD ["uv", "run", "python", "-u", "detector.py"]
