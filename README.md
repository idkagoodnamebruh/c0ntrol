# c0ntrol

Aplicación de escritorio en C++20, Qt 6 y OpenCV que convierte landmarks de
mano en acciones de puntero. El runtime mantiene la captura, inferencia,
filtrado, gestos y emisión de input detrás de interfaces independientes y deja
el input nativo desactivado hasta que el usuario lo habilita explícitamente.

## Pipeline actual

```text
AsyncCapture
  -> MediaPipeHandTrackingBackend (o MockHandTrackingBackend)
  -> LandmarkFilterBank
  -> HandFeatureExtractor
  -> GestureEngine
  -> GestureStateMachine
  -> DynamicGestureRecognizer
  -> ActionDispatcher
  -> backend de input de la plataforma
```

Las capacidades implementadas son:

- `POINTING` para movimiento absoluto del puntero;
- `PINCH` con histéresis temporal para botón primario;
- `OPEN_HAND` como pose requerida para swipes;
- eventos semánticos `SWIPE_LEFT` y `SWIPE_RIGHT`;
- `SWIPE_UP` y `SWIPE_DOWN` como scroll vertical configurable.

La ventana activa muestra la cámara, landmarks, telemetría y configuración. Los
widgets experimentales Minimalist y Matrix permanecen en el source, pero no
están instanciados ni son modos accesibles del runtime actual.

## Modelo de mano

El único asset de inferencia requerido es
`models/hand_landmarker.task`. Es el bundle oficial de MediaPipe Hand Landmarker
que contiene el detector de palma y el detector de landmarks. El checkout ya
incluye una copia validada, por lo que normalmente no hay que descargar nada.

Si el archivo falta o no supera la validación de tamaño y SHA-256:

```bash
./scripts/download_hand_model.sh
```

El script obtiene la versión oficial `float16/1`, descarga a un temporal,
valida el resultado y solo entonces instala el modelo mediante un cambio de
nombre atómico. Un error de red termina con código distinto de cero, conserva
cualquier modelo válido anterior y no crea placeholders. La fuente oficial y
la composición del bundle están descritas en la
[guía de Hand Landmarker](https://developers.google.com/edge/mediapipe/solutions/vision/hand_landmarker#models).

## Compilación

Requisitos comunes:

- compilador con C++20;
- CMake 3.16 o posterior;
- Qt 6 (`Core`, `Gui`, `Widgets`) y OpenCV para la aplicación;
- `bash`, `curl` o `wget`, y `sha256sum` o `shasum` para recuperar el modelo.

### Core y backend mock

`ENABLE_MEDIAPIPE` está desactivado por defecto. En ese modo la aplicación usa
`MockHandTrackingBackend`; compilar correctamente no demuestra inferencia real
de MediaPipe.

```bash
cmake -S . -B build-tests \
  -DBUILD_APP=OFF \
  -DBUILD_TESTING=ON \
  -DENABLE_MEDIAPIPE=OFF
cmake --build build-tests --parallel
ctest --test-dir build-tests --output-on-failure
```

Para enlazar la aplicación con el mock:

```bash
cmake -S . -B build \
  -DBUILD_APP=ON \
  -DENABLE_MEDIAPIPE=OFF
cmake --build build --parallel
```

Los targets `make`, `make run` y `make test` son atajos para este flujo y no
habilitan MediaPipe real.

### MediaPipe real

La integración validada usa MediaPipe `v0.10.26` y el bridge del proyecto en
`third_party/mediapipe_bridge`. Primero se construye ese bridge dentro del
checkout fijado de MediaPipe y luego se proporciona su biblioteca absoluta:

```bash
cmake -S . -B build-mediapipe \
  -DBUILD_APP=ON \
  -DBUILD_TESTING=ON \
  -DENABLE_MEDIAPIPE=ON \
  -DMEDIAPIPE_BRIDGE_LIBRARY=/absolute/path/to/libc0ntrol_mediapipe_bridge.so
cmake --build build-mediapipe --parallel
```

`ENABLE_MEDIAPIPE=ON` sin `MEDIAPIPE_BRIDGE_LIBRARY` es un error de
configuración deliberado. Los detalles reproducibles del bridge y la inferencia
real están en `docs/implementation/phase_2/`.

## Backends de input

El input del sistema está desactivado por defecto incluso cuando el backend fue
compilado. Debe habilitarse desde Settings; al deshabilitarlo o cerrar la
aplicación se libera de forma defensiva cualquier botón primario retenido.

- **Windows:** `WindowsSystemInputBackend` usa `SendInput`, coordenadas
  absolutas del escritorio virtual y wheel nativo.
- **Linux Wayland:** con `ENABLE_LINUX_EIS_INPUT=ON` y libei/liboeffis 1.2.1 o
  posterior, `LinuxEisSystemInputBackend` solicita puntero mediante XDG
  RemoteDesktop y emite eventos autorizados por EIS/libei.
- **Fallback:** si el backend nativo no está disponible, la factory selecciona
  `NullSystemInputBackend` y reporta input no soportado.

Una build Linux estricta puede exigir las bibliotecas reales:

```bash
cmake -S . -B build-linux \
  -DENABLE_LINUX_EIS_INPUT=ON \
  -DREQUIRE_LINUX_EIS_INPUT=ON
```

No existe backend X11. La activación física de Windows y Wayland sigue siendo
una validación manual y explícita; los tests automatizados no abren el portal ni
emiten input real.

## Estructura

```text
models/                     modelo task canónico
scripts/                    descarga y checks de hygiene
src/core/                   tracking, filtros, gestos, acciones y configuración
src/platform/windows/       backend SendInput
src/platform/linux/         backend XDG RemoteDesktop + EIS/libei
src/gui/                    ventana, telemetría y Settings
tests/                      pruebas core, plataforma, tooling e integración
third_party/mediapipe_bridge/ bridge C ABI fijado del proyecto
docs/implementation/        evidencia y contratos por fase
```
