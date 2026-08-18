# Handoff — Fase 0

## 1. Qué se hizo

Auditoría estática completa del árbol, wiring runtime, visión/modelos, Qt threading/metatypes, filtro, gestos, acciones/display, settings/GUI, build/tests, dependencias y documentación. Se hizo configure/build limpio separado y diagnóstico manual no invasivo de tests.

## 2. Qué NO se hizo

No se implementó, corrigió, refactorizó, descargó ni reemplazó nada funcional. No se ejecutó GUI/cámara por dependencias ausentes. No se inició MediaPipe/ONNX ni Fase 1.

## 3. Archivos creados

Los ocho reportes bajo `docs/implementation/phase_0/`: audit, inventory, architecture, build/tests, dependencies, reality check, risk register y este handoff.

## 4. Archivos existentes modificados

**NINGUNO.** El `build/` creado por `make test` está ignorado; no es fuente ni cambio Git.

## 5. Estado Git antes y después

Inicio: rama `work`, HEAD `554c3afffaa7d2a1287bd675afb56daa40d0d086`, árbol limpio, sin conflictos ni remote configurado (`git remote -v` vacío). Historial reciente: `554c3af Subir proyecto c0ntrol`, `0b42bee Primer commit`, `2c1e8ff Update README...`, y commits previos de modelos/script/fuentes.

Final: rama `work`, reportes guardados en un commit exclusivamente documental y working tree limpio (`## work`); `build/` permanece ignorado. Ningún archivo funcional fue modificado.

## 6. Comandos relevantes

`git branch --show-current`; `git rev-parse HEAD`; `git remote -v`; `git status --short --branch`; `git diff`; `git log --oneline -10`; `rg --files -uu`; `nl -ba`; `rg` de símbolos/conflictos; `stat`; `sha256sum`; `uname`; `/etc/os-release`; versiones de c++/cmake/qmake/pkg-config; configure/build/ctest en `/tmp/c0ntrol-phase0-build`; `make test`; compilación/ejecución manual de fuentes de test en `/tmp`.

## 7. Resultado del build

**BLOCKED:** configure limpio falla al no hallar Qt6. OpenCV tampoco aparece por pkg-config. Build posterior no tiene Makefile generado.

## 8. Resultado de tests

**NOT DISCOVERED:** CMake no registra ninguno. `make test` falla durante configure por Qt6. Cuatro fuentes pasan compilación/ejecución manual; FrameSynchronizer falla por `std::this_thread` no declarado.

## 9. P0 confirmados

Landmarks mock; ausencia de inferencia; baseline sin dependencias; shutdown/event loop bloqueable; Landmarks sin metatype.

## 10. P1 confirmados

Modelos/script inválidos, filtro compartido/dCutoff, gestos frágiles y sin FSM, click/acciones ausentes, dynamic tracker desconectado, coordenadas monomonitor, tests no registrados y contrato de tracking insuficiente.

## 11. Decisiones tomadas

Sólo decisiones de auditoría: código como verdad; no considerar existencia de archivo como wiring; separar fallo ambiental de defecto CMake; no ejecutar script destructivo; no corregir hallazgos.

## 12. Preguntas abiertas

Backend y formato de modelos definitivo; política Windows/X11/Wayland; contrato multihand; estrategia de shutdown/captura; soporte de monitores/DPI; umbrales/calibración; alcance exacto de Fase 1; versiones soportadas de Qt/OpenCV.

## 13. Dependencias faltantes

Actualmente para compilar: Qt6 Core/Gui/Widgets dev y OpenCV core/imgproc/videoio/video dev. Futuras, no seleccionadas: backend Hand Landmarker/inferencia, backends input OS y testing formal.

## 14. Recomendación de alcance para Fase 1

Sin implementarla: primero hacer reproducible build/tests y asegurar lifecycle/metatypes/contratos; luego establecer una interfaz de tracking real y tests de integración antes de acciones avanzadas.

## 15. SHA/commit auditado

**`554c3afffaa7d2a1287bd675afb56daa40d0d086`**. Los reportes describen exactamente ese código preexistente; el commit documental posterior no cambia el baseline auditado.
