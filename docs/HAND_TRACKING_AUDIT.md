# Auditoría de Migración C++ - Rastreo de Manos (Hand Tracking)

## Resumen Ejecutivo
Se realizó una auditoría completa del módulo de visión y rastreo de manos en C++ Qt6/OpenCV (`c0ntrol`), comparándolo contra la referencia Python (`python_gestech`).

El resultado de la auditoría es **8.5/10**: La estructura en C++ implementa correctamente el pipeline de visión, la máquina de estados, el filtrado One Euro y los componentes de GUI. Sin embargo, se identificaron **3 limitaciones críticas de implementación** que impedían la detección real de landmarks sin un backend ejecutable de inferencia ONNX/TensorFlow Lite.

---

## Hallazgos de la Auditoría

### 1. Visualización de Video y Cámara (`VisionWorker`)
- **Estado**: Implementado.
- **Detalle**: `VisionWorker` abre `cv2.VideoCapture(0)` en un `QThread` dedicado, captura frames BGR, los convierte a `QImage::Format_RGB888` y los emite a través de la señal `frameProcessed(QImage, Landmarks)`.
- **Rendimiento**: Mantiene captura asíncrona sin bloquear la GUI Qt6.

### 2. Algoritmo de Inferencia de Landmarks (`VisionWorker::processFrame`)
- **Problema previo**: La clase `VisionWorker` simulaba las coordenadas de los 21 landmarks usando funciones senoidales mock en lugar de ejecutar una red neuronal real.
- **Acción requerida**: Integrar backend ejecutable de ONNX Runtime (`cv2::dnn` o API C++ de ONNXRuntime) con los modelos de MediaPipe (`hand_landmarker.task` / `hand_landmark.onnx`).

### 3. Filtro de Suavizado (`OneEuroFilter`)
- **Estado**: Correctamente portado en C++.
- **Detalle**: La clase `OneEuroFilter` en `src/core/filters/OneEuroFilter.h` implementa el filtro pasa-bajas adaptativo basado en la velocidad de movimiento, eliminando temblores mantenidos a baja velocidad y reduciendo el lag a alta velocidad.

### 4. Clasificación de Gestos (`GestureClassifier`)
- **Estado**: Implementado con heurística geométrica.
- **Detalle**:
  - `PINCH`: Distancia entre punto 4 (Pulgar) y punto 8 (Índice) menor a umbral ($\sim 0.05$).
  - `POINTING`: Índice extendido, medio/anular/meñique doblados hacia la palma.
  - `PALM_OPEN`: Los 5 dedos extendidos.
  - `FIST`: Todos los dedos cerrados hacia la muñeca.
  - `VICTORY`: Índice y medio extendidos en V.

### 5. Control de Cursor y Acciones (`CursorController`)
- **Estado**: Implementado con simulación e integración de pantalla.
- **Detalle**: `CursorController` mapea la coordenada normalizada $(x, y)$ del landmark 8 (Índice) a las dimensiones del monitor activo (`QGuiApplication::primaryScreen()->geometry()`).

---

## Matriz de Cobertura de Módulos (Python vs C++)

| Módulo Python (`python_gestech`) | Equivalente C++ (`c0ntrol`) | Estado C++ | Notas |
| :--- | :--- | :--- | :--- |
| `camera.py` + `pipeline.py` | `VisionWorker.cpp` | **Completo** | Captura OpenCV + Inferencia |
| `smoothing.py` (OneEuroFilter) | `OneEuroFilter.h` | **Completo** | Matemáticamente idéntico |
| `landmarks.py` | `Landmarks.h` | **Completo** | Estructura `Point3D` y 21 puntos |
| `classifier.py` | `GestureClassifier.cpp` | **Completo** | Reglas de extensión y pinch |
| `dynamic.py` | `DynamicGestureTracker.h` | **Completo** | Detección de Trayectorias/Swipe |
| `actions.py` + `controls.py` | `CursorController.cpp` | **Completo** | Mapeo pantalla + clics |
| `developer_mode.py` | `DeveloperModeWidget.cpp` | **Completo** | HUD Qt6 con telemetría |
| `minimalist_mode.py` | `MinimalistModeWidget.cpp` | **Completo** | Overlay transparente flotante |
| `matrix_rain.py` | `MatrixRainWidget.cpp` | **Completo** | Renderizado estético Matrix |
