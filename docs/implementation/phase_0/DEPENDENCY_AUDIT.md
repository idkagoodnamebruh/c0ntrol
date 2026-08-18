# Auditoría de dependencias

## 1. Presentes en el ambiente

GNU C++ 13.3.0 y CMake 3.28.3. Las cabeceras estándar usadas incluyen vector, memory, cmath, algorithm, chrono y string. Qt6/OpenCV no se encontraron en el ambiente auditado.

## 2. Declaradas por build

| Dependencia | Declaración | Uso real |
|---|---|---|
| C++20 | `CMAKE_CXX_STANDARD 20` | sí, aunque el código visible usa mayormente capacidades anteriores |
| Qt6 Core | REQUIRED/link | QObject, QThread, señales/slots, timers, strings/debug |
| Qt6 Gui | REQUIRED/link | QImage/QPainter/QPixmap/QCursor/QScreen |
| Qt6 Widgets | REQUIRED/link | QApplication, QMainWindow, QWidget, layouts/labels/text edit |
| OpenCV core | REQUIRED/link | `cv::Mat`, tipos base |
| OpenCV imgproc | REQUIRED/link | `cvtColor` |
| OpenCV videoio | REQUIRED/link | `VideoCapture` y propiedades |
| OpenCV video | REQUIRED/link | **sin uso visible** (declarada pero no usada) |

No se declaran threads del sistema explícitamente; Qt normalmente aporta threading transitivamente. No se declara ningún framework de tests.

## 3. Realmente utilizadas

Qt6 Core/Gui/Widgets y OpenCV core/imgproc/videoio son dependencias de compilación/runtime del target actual. Los cuatro artefactos de `models/` están presentes como archivos pero no son dependencias del target ni se leen. curl/wget son alternativas sólo del script, no de la aplicación.

## 4. Faltantes para compilar en este baseline

- Paquetes/desarrollo Qt6 que proporcionen `Qt6Config.cmake` para Core, Gui y Widgets.
- Paquetes/desarrollo OpenCV 4 que proporcionen su config CMake y core/imgproc/videoio/video.
- Para el test manual de FrameSynchronizer no falta un paquete: falta una inclusión fuente `<thread>`; no se corrigió.

No se instalaron dependencias y no se verificaron versiones Qt/OpenCV por estar ausentes.

## 5. Usadas pero no declaradas / declaradas no usadas

- **Usada pero no declarada adecuadamente:** infraestructura de tests/CTest pretendida por Makefile; no existe en CMake.
- **Header no autocontenido:** `std::this_thread` se usa sin incluir `<thread>`.
- **Declarada no usada:** componente OpenCV `video`.
- **Datos declarados no usados:** modelos TFLite/Task/ONNX y `AppSettings::modelPath`.

## 6. Futuro propuesto — no presente

Estos elementos se separan deliberadamente del baseline y no implican elección de implementación:

| Capacidad objetivo | Estado | Familia de dependencia futura posible |
|---|---|---|
| Hand Landmarker real | AUSENTE | MediaPipe Tasks C++/backend oficialmente soportado, o backend decidido posteriormente |
| Inferencia por ONNX | AUSENTE | ONNX Runtime u OpenCV DNN; OpenCV DNN no está solicitado en CMake actual |
| Tests registrados | AUSENTE | CTest y ejecutables; framework como GoogleTest es opcional, no presente |
| Input Windows | AUSENTE | APIs nativas Win32 (`SendInput`) |
| Input Linux X11 | AUSENTE | API X11/XTest u otra decisión de plataforma |
| Input Linux Wayland | AUSENTE | protocolo/compositor/portal apropiado; no existe solución en repo |
| Packaging/CI | AUSENTE | CPack/install/CI según plataformas |

La fase no consultó APIs externas, no descargó modelos y no agregó dependencias.
