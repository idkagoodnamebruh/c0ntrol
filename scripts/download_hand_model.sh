#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
MODELS_DIR="$ROOT_DIR/models"

mkdir -p "$MODELS_DIR"

echo "=== Descargador de Modelos de Rastreo de Manos (MediaPipe / ONNX / TFLite) ==="
echo "Directorio destino: $MODELS_DIR"

download_if_missing() {
    local file="$1"
    local url="$2"

    if [ -f "$file" ] && [ -s "$file" ]; then
        echo "[INFO] El modelo $(basename "$file") ya existe en disco."
    else
        echo "[INFO] Descargando $(basename "$file")..."
        if command -v curl >/dev/null 2>&1; then
            curl -L --connect-timeout 15 -o "$file" "$url" || true
        elif command -v wget >/dev/null 2>&1; then
            wget -O "$file" "$url" || true
        fi

        if [ -f "$file" ] && [ -s "$file" ]; then
            echo "[EXITO] $(basename "$file") guardado exitosamente."
        else
            echo "[WARN] No se pudo descargar $(basename "$file"). Creando placeholder."
            touch "$file"
        fi
    fi
}

# 1. Hand Landmarker Task
download_if_missing "$MODELS_DIR/hand_landmarker.task" \
    "https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/latest/hand_landmarker.task"

# 2. Hand Detector TFLite
download_if_missing "$MODELS_DIR/hand_detector.tflite" \
    "https://raw.githubusercontent.com/google/mediapipe/master/mediapipe/modules/hand_landmark/hand_detector.tflite"

# 3. Hand Landmarks Detector TFLite
download_if_missing "$MODELS_DIR/hand_landmarks_detector.tflite" \
    "https://raw.githubusercontent.com/google/mediapipe/master/mediapipe/modules/hand_landmark/hand_landmark_full.tflite"

# 4. Hand Landmark ONNX
download_if_missing "$MODELS_DIR/hand_landmark.onnx" \
    "https://github.com/onnx/models/raw/main/validated/vision/body_analysis/hand_landmark/model/hand_landmark.onnx"

echo "=== Proceso de modelos completado ==="

