#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
MODELS_DIR="$ROOT_DIR/models"

mkdir -p "$MODELS_DIR"

MODEL_FILE="$MODELS_DIR/hand_landmark.onnx"
# MediaPipe Hand Landmark Lite / Full ONNX model URL source
MODEL_URL="https://github.com/onnx/models/raw/main/validated/vision/body_analysis/hand_landmark/model/hand_landmark.onnx"
FALLBACK_URL="https://raw.githubusercontent.com/google/mediapipe/master/mediapipe/modules/hand_landmark/hand_landmark_full.tflite"

echo "=== Descargador de Modelo Hand Landmark ==="
echo "Destino: $MODEL_FILE"

if [ -f "$MODEL_FILE" ]; then
    echo "[INFO] El archivo del modelo ya existe ($MODEL_FILE)."
    exit 0
fi

echo "[INFO] Descargando modelo hand_landmark.onnx..."
if command -v curl >/dev/null 2>&1; then
    curl -L --connect-timeout 10 -o "$MODEL_FILE" "$MODEL_URL" || \
    curl -L --connect-timeout 10 -o "$MODEL_FILE" "https://huggingface.co/qualcomm/MediaPipe-Hand-Landmark/resolve/main/hand_landmark.onnx" || true
elif command -v wget >/dev/null 2>&1; then
    wget -O "$MODEL_FILE" "$MODEL_URL" || true
fi

if [ -f "$MODEL_FILE" ] && [ -s "$MODEL_FILE" ]; then
    echo "[EXITO] Modelo descargado exitosamente en $MODEL_FILE"
else
    echo "[WARN] No se pudo descargar el modelo remoto. Creando placeholder para fallback local."
    touch "$MODEL_FILE"
fi
