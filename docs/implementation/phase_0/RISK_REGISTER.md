# Registro de riesgos

| ID | Severidad | Área | Riesgo | Evidencia | Impacto | Fase futura |
|---|---|---|---|---|---|---|
| R-001 | P0 | Visión | landmarks totalmente mock | `VisionWorker::extractLandmarksMock` | producto no detecta manos | visión real |
| R-002 | P0 | Inferencia | ningún backend/modelo se carga | cero referencias desde `src` | objetivo básico imposible | visión real |
| R-003 | P0 | Build | Qt6/OpenCV ausentes en baseline | configure falla | no se valida integración | reproducibilidad |
| R-004 | P0 | Threading | stop queued no se despacha | loop dentro de slot + blocking invoke | cierre colgado | lifecycle |
| R-005 | P0 | Qt types | Landmarks sin metatype | header/arranque sin declare/register | frames pueden no llegar | threading |
| R-006 | P1 | Modelos | ONNX vacío/artefactos no validados | tamaño 0; sin loaders | inferencia fallará | assets |
| R-007 | P1 | Script | conflicto, fallos ocultos/placeholders | marcadores, `|| true`, `touch` | builds falsamente preparados | tooling |
| R-008 | P1 | Filtrado | estado cruza ejes/puntos | un filtro para 63 llamadas | deformación/jitter impredecible | filtrado |
| R-009 | P1 | Filtrado | dCutoff sin uso | miembro nunca leído | derivada ruidosa | filtrado |
| R-010 | P1 | Gestos | reglas dependientes de eje Y | comparaciones tips/PIP | errores por orientación | gestos |
| R-011 | P1 | Gestos | pinch sin normalizar | umbral 0.05 | errores por escala/distancia | gestos |
| R-012 | P1 | Estado | sin debounce/histéresis/FSM | acción por frame | disparos repetidos | state machine |
| R-013 | P1 | Input | clicks no llegan al SO | señal sin receptor | función principal ausente | backends OS |
| R-014 | P1 | Dinámicos | tracker desconectado | sólo test lo referencia | swipes ausentes | gestos dinámicos |
| R-015 | P1 | Display | sólo primary y sin offset | usa width/height desde cero | multi-monitor incorrecto | coordenadas |
| R-016 | P1 | Tests | CTest vacío | no `add_test` | regresiones sin detectar | tests |
| R-017 | P1 | Tracking | sin multihand/metadata/tiempo | `vector<Point3D>` único | contrato insuficiente | arquitectura visión |
| R-018 | P2 | Concurrencia | flag no atómico | `bool m_running` | race si se cambia estrategia stop | lifecycle |
| R-019 | P2 | Ownership | worker nunca eliminado | sin `deleteLater` | fuga/cleanup inválido | lifecycle |
| R-020 | P2 | GUI | FPS falso y log ilimitado | 30.0 literal; append | telemetría engañosa/memoria | dev mode |
| R-021 | P2 | Rendimiento | copias/paint/pixmap cada frame | MainWindow frame slot | latencia/carga | profiling |
| R-022 | P2 | Config | settings sin consumidores | sólo definición | divergencia y no persistencia | configuración |
| R-023 | P2 | Arquitectura | transformación duplicada | dos map/clamp | comportamiento divergente | display |
| R-024 | P2 | Timing | FrameSynchronizer roto/desconectado | sólo test, falta thread | timing no reutilizable | captura |
| R-025 | P2 | Cámara | read bloqueante/sin drops | `m_cap >> frame` | parada/latencia desconocida | captura |
| R-026 | P3 | GUI | widgets muertos | no instanciados | coste/mantenimiento | UX |
| R-027 | P3 | Distribución | sin install/package/warnings | CMake mínimo | despliegue débil | release |

Conteo: **P0 5 · P1 11 · P2 9 · P3 2**.
