# Documentación versus realidad

| Afirmación | Documento | Código real | Resultado |
|---|---|---|---|
| Aplicación C++20/Qt6/OpenCV | README | CMake y fuentes coinciden | CORRECTA |
| Captura asíncrona no congela GUI | README/HAND audit | worker separado, pero loop impide stop y transporte Landmarks no registrado | PARCIAL |
| Captura 30–60 FPS | README | sleep 33 ms más procesamiento; no 60 ni medición | INCORRECTA |
| One Euro “en 3D” elimina temblores | README/HAND audit | una sola instancia mezcla ejes y puntos; dCutoff inactivo | INCORRECTA |
| PINCH, POINTING, PALM_OPEN, FIST, VICTORY | README | enum/reglas existen; PALM/FIST ignoran pulgar y sólo frame actual | PARCIAL |
| PINCH hace click de ratón | README | `performClick` sólo emite señal sin receptor/backend | INCORRECTA |
| DynamicGestureTracker detecta swipes | README/HAND audit | clase y test existen, runtime no la usa | PARCIAL |
| Developer Mode muestra FPS/latencia/log real | README | FPS literal 30; no latencia; landmarks mock; log sí se agrega | INCORRECTA |
| Minimalist y Matrix disponibles como modos | README | clases existen pero MainWindow no las crea/conecta | DESACTUALIZADA |
| Modelos se descargan opcionalmente | README | script tiene conflicto, ignora fallos y crea vacíos | INCORRECTA |
| “pipeline completo” e inferencia | HAND_TRACKING_AUDIT | `extractLandmarksMock`, ninguna API/model load | INCORRECTA |
| Cursor y clicks “completos” | HAND_TRACKING_AUDIT | cursor real por QCursor; click señal בלבד | INCORRECTA |
| filtro correctamente portado/matemáticamente idéntico | HAND_TRACKING_AUDIT | derivada sin dCutoff y estado compartido | INCORRECTA |
| arquitectura/migración completada | HAND_TRACKING_FINAL | numerosos componentes ausentes o desconectados | INCORRECTA |
| 5 suites/ejecutables passing | HAND_TRACKING_FINAL | no hay targets/tests CMake; uno no compila manualmente | INCORRECTA |
| test geometría cubre cinco gestos/distancia | HAND_TRACKING_FINAL | sólo PINCH y POINTING | INCORRECTA |
| test FrameSync valida sincronía frame-landmarks | HAND_TRACKING_FINAL | sólo mide dos sleeps y no compila solo | INCORRECTA |
| menos de 5 ms | informe_arquitectura | objetivo sin instrumentación/benchmark | NO VERIFICABLE |
| cero asignaciones dinámicas por frame | informe_arquitectura | vectors, image copies, QString, text append y pixmap asignan | INCORRECTA |
| diagrama conecta DynamicTracker/Minimalist/Matrix | informe_arquitectura | esos nodos no aparecen en MainWindow/runtime | INCORRECTA |
| ONNX es trabajo futuro | refactor_vision | describe fases de carga/forward aún inexistentes | CORRECTA |
| Python de referencia es completo | analisis referencia | ese repositorio no está incluido ni fue auditado | NO VERIFICABLE |
| CursorController abstraerá X11/Windows/Qt | analisis referencia | formulado en futuro; hoy sólo QCursor | CORRECTA como aspiración |

## Conclusión

`refactor_vision.md` es el documento más alineado porque trata inferencia como plan. README y documentos de cierre mezclan existencia de clases con funcionalidad integrada y resultados no reproducibles. La evidencia canónica es el código del SHA auditado y los comandos de baseline.
