# Fase 0 — Auditoría del repositorio

**Fecha:** 2026-08-13 · **SHA auditado:** `554c3afffaa7d2a1287bd675afb56daa40d0d086` · **rama:** `work`

## 1. Executive summary

El repositorio es un prototipo Qt/OpenCV: captura cámara y muestra frames, pero los 21 puntos son una trayectoria senoidal generada por `VisionWorker::extractLandmarksMock`; ningún modelo se carga ni se ejecuta. El cursor real sí se mueve mediante Qt, mientras que el “clic” sólo emite una señal sin receptores. La aplicación no pudo configurarse en el ambiente auditado porque faltan Qt6 y OpenCV. Además, CMake no define pruebas, el tipo `Landmarks` cruza hilos sin registro de metatype, y el shutdown combina un loop que ocupa el event loop del worker con una invocación bloqueante a `stop()`.

Estado medido: **build BLOCKED**, **tests NOT DISCOVERED**. Hay cinco bloqueadores P0, once hallazgos P1, nueve P2 y dos P3.

## 2. Objetivo entendido

Aplicación nativa C++20/Qt6/OpenCV, reproducible y de baja latencia, con tracking real de una o dos manos, 21 landmarks por mano, estabilización temporal, clasificación robusta y máquina de estados, telemetría y acciones reales multiplataforma. MediaPipe Hand Landmarker es candidato futuro, no parte de esta fase.

## 3. Estado real general

`main()` crea `MainWindow`; éste crea un `VisionWorker` en `QThread`, abre cámara 0 a 640×480, convierte BGR→RGB y emite una copia de `QImage` con landmarks mock. El GUI clasifica cada frame, intenta controlar el cursor, pinta puntos y añade telemetría. No hay inferencia, detector de palma, ROI, handedness, confidence, tracking de identidad, multihand, timestamps de captura ni contabilidad de frames perdidos.

## 4. Hallazgos P0 (5)

1. **P0-01, visión ficticia.** Archivo `src/core/vision/VisionWorker.cpp`, símbolos `processLoop`/`extractLandmarksMock`, líneas 61–86: genera senos/cosenos; no observa manos.
2. **P0-02, backend ausente.** Ningún `src/` referencia `models/`, DNN, TFLite, MediaPipe u ONNX; no existe inferencia real.
3. **P0-03, build no reproducible en baseline.** `find_package(Qt6 ... REQUIRED)` falla por Qt6 ausente; OpenCV tampoco está disponible por `pkg-config`.
4. **P0-04, parada no entregable.** `VisionWorker::start()` entra directamente en `processLoop()` y monopoliza el event loop; el destructor usa `BlockingQueuedConnection` para `stop()`. La llamada en cola no puede ejecutarse mientras el loop continúe por sí solo.
5. **P0-05, transporte Qt inseguro/no registrado.** `Landmarks` cruza worker→GUI con conexión automática queued, pero no tiene `Q_DECLARE_METATYPE` ni `qRegisterMetaType`; existe riesgo de que Qt rechace el argumento y no entregue frames.

## 5. Hallazgos P1 (11)

- Modelo ONNX vacío; los otros binarios no son consumidos ni validados.
- No hay landmarks reales, tracking, multihand, handedness, confidence, detector de palma ni ROI.
- Un único `OneEuroFilter` comparte estado entre X/Y/Z y los 21 puntos; `dCutoff` se almacena pero no interviene.
- Clasificación estática frame a frame, dependiente de orientación vertical, y pinch 3D con umbral fijo no normalizado.
- No existen histéresis, debounce, dwell, cooldown ni máquina de estados.
- `performClick()` sólo emite `clickPerformed`; no genera input del SO y la señal no tiene receptores.
- Acciones faltantes: click derecho real, down/up, drag, doble click, scroll, teclado/hotkeys/volumen.
- `DynamicGestureTracker` no forma parte del runtime.
- Mapeo limitado a pantalla primaria, ignora origen de geometría, escritorio virtual, DPI y espejado.
- Tests no están declarados en CMake ni descubiertos por CTest.
- Script de modelos contiene conflicto textual, ignora errores y crea archivos vacíos.

## 6. Hallazgos P2/P3

**P2 (9):** `m_running` no es atómico; worker sin `deleteLater`; cámara puede bloquear; FPS fijo 30.0; log sin límite; copia de imagen y overlay por frame; settings desconectados; lógica de transformación duplicada; `FrameSynchronizer` desconectado y no autocontenido (`<thread>` ausente).

**P3 (2):** `MinimalistModeWidget` y `MatrixRainWidget` se compilan pero nunca se instancian; no hay install/packaging ni política explícita de warnings/build type.

## 7. Auditoría por subsistema

### Visión

- `VideoCapture::open(0)`; peticiones 640×480, sin comprobar valores efectivos; no establece FPS ni `CAP_PROP_BUFFERSIZE`.
- Loop dedicado con `m_cap >> frame`, espera 10 ms ante vacío y 33 ms tras cada frame. El periodo real incluye captura, conversión, emisión y sleep; no se mide.
- Conversión `cv::cvtColor(BGR, RGB)` y `QImage` sobre memoria de `cv::Mat`; `image.copy()` da ownership válido antes de emitir.
- Tiempo mock incrementado exactamente 0.033, no timestamp real. Sin dropped-frame accounting.

### Threading, lifecycle y metatypes

`m_worker` no tiene padre tras `moveToThread`, no se conecta `QThread::finished` a `deleteLater`. `m_running` es `bool`; actualmente lectura/escritura pretendidamente inter-thread sería race, aunque la parada queued queda bloqueada antes de ejecutarse. Si la cámara falla, `start()` retorna y el event loop sí queda libre. `QImage` es metatype Qt copiable; `Landmarks` contiene `std::vector<Point3D>`, es copiable pero no está declarado/registrado.

### One Euro Filter

Actualiza frecuencia con `1/dt`; deriva respecto del último valor filtrado, pero usa `lastValue()==0` como sustituto de estado inicial. `m_dxFilter` mantiene alpha 1.0: no usa `dCutoff`, por tanto la derivada no se suaviza. `minCutoff` y `beta` sí determinan el cutoff adaptativo. Una instancia global en `VisionWorker` procesa en secuencia x/y/z de cada uno de 21 landmarks con el mismo `m_lastTime`, `m_xFilter` y `m_dxFilter`: mezcla dimensiones y puntos, y las llamadas repetidas con timestamp igual no corrigen frecuencia. No hay aislamiento por mano.

### Gestos estáticos

| Gesto | Regla real | Riesgo geométrico |
|---|---|---|
| PINCH | distancia euclídea 3D `4↔8 < 0.05` | no normalizada: escala/distancia/perspectiva/ruido; prioridad sobre los demás |
| POINTING | `8.y<6.y`; 12/16/20 no menores que 10/14/18 | presupone dedos verticales; sensible a rotación, perspectiva y ruido |
| VICTORY | índice y medio extendidos; anular/meñique no | misma dependencia de eje Y; no valida separación en V |
| PALM_OPEN | cuatro dedos extendidos por Y | no evalúa pulgar pese a afirmar “5 dedos” |
| FIST | cuatro dedos no extendidos por Y | no evalúa pulgar; orientación puede invertir resultado |
| UNKNOWN/NONE | UNKNOWN para configuración restante; NONE si <21 puntos | sin confidence ni temporalidad |

No hay tratamiento de handedness. La clasificación es por frame, sin estado temporal.

### Gestos dinámicos

Guarda sólo landmark 0 hasta `maxHistory` posiciones, sin timestamps. Compara primer/último desplazamiento; no calcula velocidad, aceleración ni forma de trayectoria. Limpia únicamente al detectar o mediante `reset()`. Un desplazamiento lento, deriva o salto puede disparar; movimiento diagonal se asigna al eje dominante. Está sólo en test.

### Acciones y coordenadas

`QCursor::setPos(x,y)` sí cambia el cursor real donde la plataforma Qt lo permita. `performClick()` sólo emite una señal. Cada frame PINCH emitiría otro supuesto click, sin flanco/cooldown. Usa ancho/alto de `primaryScreen()->geometry()` pero omite `geometry().x()/y()`, así que falla con offsets; no usa escritorio virtual, pantalla activa, DPI lógico/físico, active area ni mirror. `DisplayTransform` duplica clamp/map y tampoco offsets; no está conectado.

### Settings y GUI

Todos los campos de `AppSettings` existen sólo en el header y no se instancian: cámara, FPS, resolución, filtro y threshold aparecen hardcodeados en otros módulos; no son editables ni persistentes. `MainWindow` instancia Developer Mode, no los otros dos widgets. Telemetría recibe landmarks mock y gesto derivado, pero FPS literal `30.0`. Cada frame copia dos veces la imagen, pinta todos los puntos, convierte a pixmap y agrega una línea a `QTextEdit`; el widget limita altura visual, no número de bloques.

### Build y tests

CMake compila todos los `.cpp` bajo `src` mediante glob y enlaza Qt6 Core/Gui/Widgets y OpenCV core/imgproc/videoio/video. No hay opciones, tests, install, packaging, warnings ni build type. `BUILD_TESTS` es ignorado. Véase `BUILD_AND_TEST_BASELINE.md`.

## 8. Las 24 hipótesis

| # | Resultado | Evidencia resumida |
|---:|---|---|
| 1 | CONFIRMADA | `VisionWorker.cpp:62,70` llama/define mock |
| 2 | CONFIRMADA | cero APIs o carga de inferencia en `src/` |
| 3 | CONFIRMADA | ONNX mide 0 bytes, SHA-256 de vacío |
| 4 | CONFIRMADA | script líneas 35,55,74 contiene marcadores |
| 5 | CONFIRMADA | `|| true` y `touch` líneas 22–31 |
| 6 | CONFIRMADA | CMake acaba en el único executable; no CTest/add_test |
| 7 | CONFIRMADA | `make test` falla al configurar en baseline; aun con deps, no define tests |
| 8 | CONFIRMADA | sin declaración ni registro de `Landmarks` |
| 9 | CONFIRMADA | `start→processLoop` bloquea event loop del worker |
| 10 | PARCIAL | patrón permite espera indefinida; no se reprodujo por falta de GUI/cámara |
| 11 | CONFIRMADA | una instancia y un par de filtros para las 63 componentes |
| 12 | CONFIRMADA | miembro `m_dCutoff` nunca leído |
| 13 | PARCIAL | usa comparaciones Y relativas, no Y absoluta; aun así depende del eje vertical |
| 14 | CONFIRMADA | threshold fijo `0.05` sin escala de palma |
| 15 | CONFIRMADA | clasificación/acción inmediata por frame |
| 16 | CONFIRMADA | sólo `emit clickPerformed(button)` |
| 17 | CONFIRMADA | referencias runtime inexistentes |
| 18 | CONFIRMADA | `AppSettings` sin consumidores |
| 19 | CONFIRMADA | `updateTelemetry(30.0, ...)` |
| 20 | CONFIRMADA | `QTextEdit::append` sin máximo de bloques |
| 21 | CONFIRMADA | clases compiladas por glob, no instanciadas |
| 22 | CONFIRMADA | duplicación map/clamp con `CursorController` |
| 23 | CONFIRMADA | sólo test; header usa `std::this_thread` sin `<thread>` |
| 24 | CONFIRMADA | documentos afirman completo/inferencia/5 passing contra código/build |

Totales: **22 confirmadas, 0 refutadas, 2 parciales, 0 no verificables**. Las hipótesis parciales son la posibilidad de deadlock no reproducida (10) y la caracterización “Y absoluta” frente a comparaciones Y relativas (13).

## 9. Contradicciones relevantes

Los documentos califican captura asíncrona, inferencia, filtro, gestos dinámicos, clics y cinco pruebas como completos. El código muestra landmarks mock, filtro con estado compartido, módulos sin wiring, señal de clic sin backend y cero tests CMake. “<5 ms” y “baja latencia” no tienen medición.

## 10. Antes de implementar visión real

Debe acordarse y corregirse en fases posteriores: shutdown/event-loop y ownership; metatype; build/dependencias reproducibles; test discovery; contrato de datos multihand con timestamp/confidence/handedness; aislamiento correcto del filtrado; y separación entre detección, estado y acciones reales. Fase 0 no cambia ninguna implementación.
