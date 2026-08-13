# Cambios de Fase 1

## Archivos funcionales

### `CMakeLists.txt`
**Motivo:** R-016 y reproducibilidad. **Cambio:** `BUILD_APP`, CTest, cinco targets/tests. **Antes:** dependencias obligatorias y cero tests. **Después:** tests core independientes. **Riesgo:** R-003/R-016. **Validación:** configure/build y 5/5 CTest.

### `Makefile`
**Motivo:** aislar caches. **Cambio:** `build-tests/`, flags estándar y clean ampliado. **Antes:** tests reutilizaban `build/` y `BUILD_TESTS` inexistente. **Después:** flujo reproducible. **Riesgo:** R-016. **Validación:** `make test` PASS.

### `src/core/vision/FrameSynchronizer.h`
**Motivo:** header no compilaba solo. **Cambio:** incluye `<thread>`. **Antes:** `std::this_thread` no declarado. **Después:** target compila. **Riesgo:** R-024. **Validación:** test PASS.

### `src/core/qt/QtMetaTypes.h`
**Motivo:** tipo custom queued. **Cambio:** bridge Qt con `Q_DECLARE_METATYPE(Landmarks)`. **Antes:** declaración ausente. **Después:** declaración visible sin dependencia Qt en core. **Riesgo:** R-005. **Validación:** revisión estática; app bloqueada por entorno.

### `src/core/vision/VisionWorker.h/.cpp`
**Motivo:** event loop bloqueado. **Cambio:** QTimer hijo, `processFrame()` unitario, stop idempotente, eliminación de `m_running`. **Antes:** loop infinito y sleeps. **Después:** retorno al event loop por frame. **Riesgo:** R-004/R-018/R-025. **Validación:** revisión estática; app no ejecutada.

### `src/gui/MainWindow.cpp`
**Motivo:** transporte/ownership. **Cambio:** registra Landmarks antes de conexiones y conecta `finished→deleteLater`. **Antes:** metatype y cleanup ausentes. **Después:** contrato y cleanup explícitos. **Riesgo:** R-005/R-019. **Validación:** revisión estática.

## Reportes creados

Los cinco archivos de `docs/implementation/phase_1/` documentan alcance, cambios, tests, riesgos y handoff. No cambian runtime.
