# Reporte de tests — Fase 1

## Ambiente

Ubuntu 24.04.4 x86_64; GNU C++ 13.3.0; CMake 3.28.3. Qt6 y OpenCV no disponibles. No se instalaron dependencias.

## Core sin Qt/OpenCV — PASS

```sh
rm -rf build-tests
cmake -S . -B build-tests -DBUILD_APP=OFF -DBUILD_TESTING=ON
cmake --build build-tests --parallel
ctest --test-dir build-tests --output-on-failure
```

| Test | Estado | Duración CTest |
|---|---|---:|
| `test_display_transform` | PASS | 0.01 s |
| `test_dynamic_gestures` | PASS | 0.01 s |
| `test_frame_sync` | PASS | 0.04 s |
| `test_hand_geometry` | PASS | 0.01 s |
| `test_one_euro` | PASS | 0.01 s |

Resultado: **5/5 PASS**, total 0.09 s; exactamente cinco tests descubiertos.

## Makefile — PASS

`make test` reconfiguró `build-tests`, construyó y ejecutó los mismos cinco: **5/5 PASS**, total 0.08 s.

## Aplicación — BLOCKED

```sh
cmake -S . -B /tmp/c0ntrol-phase1-app -DBUILD_APP=ON -DBUILD_TESTING=OFF
```

Configure **BLOCKED BY ENVIRONMENT**: falta `Qt6Config.cmake`; OpenCV tampoco estaba disponible en el baseline. Build y ejecución: **NOT RUN/BLOCKED**. No se atribuye este resultado al código. Lifecycle, metatype y shutdown no tuvieron validación runtime.

No aparecieron warnings en el build core. Los artifacts permanecen en directorios ignorados y no se incluyen en Git.
