# Baseline de build y tests

## Ambiente (2026-08-13 UTC)

| Elemento | Resultado |
|---|---|
| OS/kernel | Ubuntu 24.04.4 LTS; Linux 6.18.35 |
| Arquitectura | x86_64 |
| Compilador | GNU C++ 13.3.0 |
| CMake | 3.28.3 |
| Qt6 | **no encontrado**; `qmake6` ausente y pkg-config sin Qt6Core/Gui/Widgets |
| OpenCV | **no encontrado** por pkg-config (`opencv4`) |

## Camino limpio separado

Comando: `cmake -S . -B /tmp/c0ntrol-phase0-build -DBUILD_TESTS=ON`.

**Configure: FAIL/BLOCKED.** CMake llega a `CMakeLists.txt:11` y no encuentra `Qt6Config.cmake`/`qt6-config.cmake`. Al ser REQUIRED, no alcanza la búsqueda OpenCV; la comprobación independiente de pkg-config tampoco encontró OpenCV.

Comando: `cmake --build /tmp/c0ntrol-phase0-build --parallel 2`.

**Build: BLOCKED.** No hay Makefile generado (`gmake: No rule to make target 'Makefile'`). Por tanto no existen warnings de compilador/linker evaluables ni binario.

## Mecanismo oficial

Comando: `make test`.

**FAIL/BLOCKED** con código 2: crea el directorio ignorado `build/`, ejecuta `cmake -DBUILD_TESTS=ON ..` y falla por Qt6. No se ocultó ni corrigió el fallo.

## CTest y descubrimiento

Comando: `ctest --test-dir /tmp/c0ntrol-phase0-build --output-on-failure`.

Salida: `No tests were found!!!`; CTest devuelve 0 aunque no prueba nada. CMake no contiene `include(CTest)`, `enable_testing()`, `add_executable` de tests ni `add_test`; tampoco consulta `BUILD_TESTS` o `BUILD_TESTING`. En consecuencia los **cinco fuentes están NOT DISCOVERED** incluso en un host con dependencias.

## Comprobación diagnóstica manual (no mecanismo oficial)

Con `c++ -std=c++20 -I.` (y `GestureClassifier.cpp` para geometría):

| Fuente | Compila | Ejecuta | Resultado/límite |
|---|---:|---:|---|
| display transform | sí | sí | PASS de 2 casos |
| dynamic gestures | sí | sí | PASS de swipe right |
| frame sync | **no** | — | `std::this_thread has not been declared`, falta `<thread>` en header |
| hand geometry | sí | sí | PASS PINCH/POINTING |
| one euro | sí | sí | PASS de tres muestras |

Estos cuatro PASS manuales no convierten la suite oficial en PASS y no prueban integración. `assert` además desaparece con `NDEBUG`.

## Problemas reproducibles y warnings

1. Dependencias dev Qt6 y OpenCV faltantes en el entorno.
2. `BUILD_TESTS=ON` no cambia el grafo CMake.
3. CTest devuelve éxito vacío, posible falso verde.
4. `FrameSynchronizer.h` no es autocontenido.
5. No se pudo evaluar build completo, AUTOMOC, linker, ejecución GUI/cámara ni shutdown. No se instalaron paquetes.
