# Documento de Cierre: Hand Tracking C++ Qt6 (`c0ntrol`)

## Resumen del Progreso

Se ha completado la migración y refinamiento de la arquitectura de la aplicación de control por gestos **c0ntrol** de Python a **C++20 nativo con Qt6 y OpenCV**.

---

## Arquitectura Implementada

```
Stark45/
├── CMakeLists.txt              # Configuración de CMake (Qt6 Core/Gui/Widgets, OpenCV)
├── Makefile                    # Wrapper para compilación rápida y ejecución de pruebas
├── models/                     # Archivos de modelo MediaPipe (TFLite / Task / ONNX)
│   ├── hand_detector.tflite
│   ├── hand_landmarker.task
│   ├── hand_landmark.onnx
│   └── hand_landmarks_detector.tflite
├── scripts/
│   └── download_hand_model.sh  # Script de descarga y verificación de modelos
├── docs/
│   ├── HAND_TRACKING_AUDIT.md  # Auditoría inicial y matriz de equivalencias
│   └── HAND_TRACKING_FINAL.md  # Documento final de arquitectura y estado
├── src/
│   ├── main.cpp                # Punto de entrada de la aplicación Qt
│   ├── core/
│   │   ├── actions/            # Movimiento de cursor y simulación de clics
│   │   ├── config/             # Parámetros de configuración globales
│   │   ├── filters/            # One Euro Filter (suavizado de temblor)
│   │   ├── gestures/           # Clasificación de gestos y heurísticas geométricas
│   │   └── vision/             # Captura OpenCV, transformaciones y sincronización
│   └── gui/                    # Ventanas MainWindow, DeveloperMode, MinimalistMode, MatrixRain
└── tests/                      # Suite de pruebas unitarias (5 suites passing)
```

---

## Suite de Pruebas Unitarias

Se cuenta con **5 ejecutables de prueba** validados correctamente:

1. **`test_one_euro`**: Valida que la respuesta del filtro suavice pequeños temblores espaciales y reaccione a movimientos veloces.
2. **`test_hand_geometry`**: Valida el cálculo de distancias euclidianas 3D y la clasificación de gestos (`POINTING`, `PINCH`, `PALM_OPEN`, `FIST`, `VICTORY`).
3. **`test_display_transform`**: Valida el mapeo de coordenadas $(x, y) \in [0, 1]$ a la resolución objetivo del monitor.
4. **`test_frame_sync`**: Valida que los frames de cámara y los conjuntos de landmarks no sufran desincronizaciones de tiempo.
5. **`test_dynamic_gestures`**: Valida el registro de trayectorias en ventana temporal para reconocer deslices (*Swipes*).

---

## Verificación de Compilación y Ejecución

Para compilar y ejecutar el proyecto localmente en Linux con Qt6 y OpenCV instalado:

```bash
# Compilar proyecto principal
make

# Ejecutar aplicación
make run

# Compilar y ejecutar pruebas unitarias
make test
```
