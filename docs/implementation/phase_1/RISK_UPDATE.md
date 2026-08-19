# Actualización de riesgos

| ID | Estado | Evidencia de Fase 1 | Nota restante |
|---|---|---|---|
| R-003 | PARTIALLY_RESOLVED | tests configuran sin Qt/OpenCV | app continúa ENVIRONMENT_BLOCKED por Qt6/OpenCV |
| R-004 | RESOLVED | no hay loop; timer invoca un frame y retorna | read individual aún puede retrasar stop (R-025) |
| R-005 | RESOLVED | declare bridge + registro antes de conexión | compilación app no validada en ambiente |
| R-016 | RESOLVED | cinco `add_test`; 5/5 por CTest y make | tests siguen basados en assert por alcance |
| R-018 | RESOLVED | `m_running` eliminado; estado confinado al worker | — |
| R-019 | RESOLVED | `finished→deleteLater`; stop/quit/wait | runtime no ejecutado |
| R-024 | PARTIALLY_RESOLVED | `<thread>` y test compila/pasa | componente sigue desconectado |
| R-025 | PARTIALLY_RESOLVED | se eliminó loop continuo | una lectura OpenCV todavía puede bloquear temporalmente |

## Fuera de alcance, permanecen OPEN

R-001, R-002, R-008, R-009, R-010, R-011, R-012 y R-013. Esta fase no cambió landmarks mock, inferencia, filtro, clasificador, estado ni input.
