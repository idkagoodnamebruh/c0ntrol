# Inventario del repositorio

**Baseline:** `554c3afffaa7d2a1287bd675afb56daa40d0d086`. No había cambios tracked/untracked al inicio.

## Árbol completo relevante

```text
.
├── .gitignore
├── CMakeLists.txt
├── Makefile
├── README.md
├── analisis_arquitectura_referencia.md
├── informe_arquitectura.md
├── refactor_vision.md
├── docs/{HAND_TRACKING_AUDIT.md,HAND_TRACKING_FINAL.md}
├── models/{hand_detector.tflite,hand_landmark.onnx,hand_landmarker.task,hand_landmarks_detector.tflite}
├── scripts/download_hand_model.sh
├── src/main.cpp
├── src/core/actions/{CursorController.cpp,CursorController.h}
├── src/core/config/Settings.h
├── src/core/filters/OneEuroFilter.h
├── src/core/gestures/{DynamicGestureTracker.h,GestureClassifier.cpp,GestureClassifier.h,Landmarks.h}
├── src/core/vision/{DisplayTransform.h,FrameSynchronizer.h,VisionWorker.cpp,VisionWorker.h}
├── src/gui/{DeveloperModeWidget.cpp,DeveloperModeWidget.h,MainWindow.cpp,MainWindow.h,
│            MatrixRainWidget.cpp,MatrixRainWidget.h,MinimalistModeWidget.cpp,MinimalistModeWidget.h}
└── tests/{test_display_transform.cpp,test_dynamic_gestures.cpp,test_frame_sync.cpp,
          test_hand_geometry.cpp,test_one_euro.cpp}
```

## Build, configuración, modelos y scripts

| Archivo | Responsabilidad/dependencias | Conexión/estado |
|---|---|---|
| `CMakeLists.txt` | C++20, AUTOMOC/UIC/RCC, Qt6 Core/Gui/Widgets, OpenCV core/imgproc/videoio/video | Activo; glob de todo `src`; no tests/install/package/warnings |
| `Makefile` | wrapper configure/build/run/test | Activo; `BUILD_TESTS` no tiene consumidor CMake |
| `.gitignore` | build, binarios, IDE/OS | Activo; ignora `build/` y nombres de tests |
| `Settings.h` | defaults cámara/filter/gestos/modelo | No conectado; duplicado por literales runtime; sin persistencia |
| `hand_detector.tflite` | binario, 2,339,878 B | No vacío; nadie lo carga; validez semántica no verificada |
| `hand_landmark.onnx` | supuesto ONNX, 0 B | Placeholder vacío; inválido; nadie lo carga |
| `hand_landmarker.task` | binario, 7,819,105 B | No vacío; nadie lo carga; contenedor no inspeccionado |
| `hand_landmarks_detector.tflite` | binario, 5,478,949 B | No vacío; nadie lo carga; validez semántica no verificada |
| `download_hand_model.sh` | descarga cuatro artefactos con curl/wget | Marcadores de conflicto; ignora fallos; crea vacío; sin checksum |

La extensión no prueba el formato. `file(1)` no está instalado; tamaños y SHA-256 sí se midieron. Ningún modelo participa en el target ni runtime.

## Código: responsabilidades y grafo de llamadas

| Archivo | Responsabilidad aparente | Llamado por → llama a | Runtime | Observaciones |
|---|---|---|---|---|
| `src/main.cpp` | bootstrap QApplication | SO → `MainWindow`, `app.exec` | Sí | mínimo |
| `MainWindow.*` | composición, worker, overlay, clasificación | main/worker → classifier, cursor, Developer UI, QPainter | Sí | ownership incompleto de worker; shutdown riesgoso |
| `VisionWorker.*` | captura y emisión | thread started → OpenCV, mock, filter | Sí | mock explícito; loop bloqueante; sin inferencia |
| `Landmarks.h` | `Point3D`, distancia y vector de puntos | vision/tests/classifier/UI | Sí | sin metatype; sólo una mano implícita; sin metadata |
| `OneEuroFilter.h` | low-pass adaptativo | VisionWorker/tests | Sí | estado único compartido; `dCutoff` placeholder efectivo |
| `GestureClassifier.*` | heurísticas estáticas | MainWindow/tests → `distanceTo` | Sí | frame-by-frame; umbrales hardcodeados |
| `DynamicGestureTracker.h` | ventana de wrists/swipes | sólo test | No | sin tiempo/velocidad; funcionalidad desconectada |
| `CursorController.*` | map, cursor Qt, señal click | MainWindow → QScreen/QCursor | Sí | cursor real; click mock/señal; duplica DisplayTransform |
| `DisplayTransform.h` | mapa normalizado/clamp | sólo test | No | duplicado en CursorController |
| `FrameSynchronizer.h` | sleep a FPS | sólo test | No | no autocontenido: falta `<thread>`; división entera |
| `DeveloperModeWidget.*` | etiquetas/log | MainWindow | Sí | FPS recibido es constante; log crece |
| `MinimalistModeWidget.*` | overlay HUD | nadie | No | compilado por glob, no instanciado |
| `MatrixRainWidget.*` | animación a 20 FPS, máx. 50 drops | nadie | No | compilado por glob; timer sólo existiría al instanciar |

## Tests

| Archivo | Valida | No valida / estado |
|---|---|---|
| `test_display_transform.cpp` | centro y clamp | offsets, DPI, espejado, tamaños inválidos; compila manualmente |
| `test_dynamic_gestures.cpp` | swipe derecho sintético | tiempo, falsos positivos, otros sentidos/reset; compila manualmente |
| `test_frame_sync.cpp` | dos sleeps total ≥15 ms | jitter, FPS sostenido, integración; no compila solo por falta de `<thread>` |
| `test_hand_geometry.cpp` | PINCH y POINTING sintéticos | otros gestos, escala/orientación/ruido; compila con classifier |
| `test_one_euro.cpp` | pequeños deltas quedan <0.1 | matemática, dCutoff, separación de estados; compila manualmente |

Ninguno está declarado como target ni registrado en CTest.

## Documentación y miscelánea

`README.md` describe uso y capacidades; `analisis_arquitectura_referencia.md` describe principalmente el proyecto Python de referencia; `informe_arquitectura.md` presenta arquitectura objetivo; `refactor_vision.md` admite que ONNX es hoja de ruta; los dos documentos `HAND_TRACKING_*` sobrestiman el estado. No existen recursos Qt `.ui/.qrc`, configuración persistida, licencias, CI, packaging ni otros archivos visibles fuera de `.git`.

## Clasificación transversal

- **Activos:** main, MainWindow, VisionWorker, OneEuroFilter, Landmarks, GestureClassifier, CursorController, DeveloperModeWidget.
- **No conectados:** Settings, DynamicGestureTracker, DisplayTransform, FrameSynchronizer, MinimalistModeWidget, MatrixRainWidget, todos los modelos.
- **Mocks/placeholders:** `extractLandmarksMock`, click como señal sin backend, ONNX vacío, `touch` ante descarga fallida.
- **Duplicaciones:** transformación normalizada y clamp en DisplayTransform/CursorController; defaults Settings/literales.
- **Documentación discordante:** “inferencia”, “completo”, “5 suites passing”, filtro “3D correcto” y clics.
