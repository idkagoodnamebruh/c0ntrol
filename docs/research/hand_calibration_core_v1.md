# Hand calibration core v1

**STATUS: RESEARCH / NOT RUNTIME CONNECTED**

Rama: `research/hand-calibration-core-v1`. Base de trabajo existente:
`00f0be3` (`fix(release): enable Windows C11 atomics`).

## Objetivo y aislamiento

Construir un perfil geométrico personalizado, pequeño y determinista a partir de
`HandFeatures` ya extraídos y poses etiquetadas por el caller. Esta investigación
no detecta poses ni ejecuta acciones. No vuelve a calcular landmarks.

El diff añade archivos; no modifica archivos existentes. En particular, no cambia
el CMake raíz ni los targets, dependencias o fuentes de producción. El target
`c0ntrol` utiliza una lista explícita de fuentes; ninguno de los archivos nuevos
se añade a ella. No hay llamadas al módulo desde el código existente.
`HandCalibrationSession.cpp` no forma parte del ejecutable principal.

El único CMake nuevo está en `research/hand_calibration/`, tiene su propio
`project()` y se configura explícitamente con `cmake -S research/hand_calibration`.
No está incluido desde el proyecto principal. Es seguro registrar allí el test
porque ese árbol de build sólo produce `test_hand_calibration`.

No cambia MediaPipe, VisionWorker, GestureEngine, GesturePipeline,
GestureStateMachine, ActionDispatcher, NativeInputRuntime, los backends de input,
PointerCalibration, RuntimeConfig, SettingsCodec, QSettings, GUI, filtros, modelos
ni empaquetado. `kRuntimeConfigVersion` permanece intacto.

## Archivos

| Archivo nuevo | Responsabilidad |
| --- | --- |
| `src/core/calibration/HandCalibrationTypes.h` | Poses, observación, configuración, estados y perfil versionado |
| `src/core/calibration/HandCalibrationStatistics.h/.cpp` | Resúmenes robustos deterministas |
| `src/core/calibration/HandCalibrationSession.h/.cpp` | Validación, adquisición acotada y construcción del perfil |
| `tests/test_hand_calibration.cpp` | 28 casos autocontenidos, sin frameworks externos |
| `research/hand_calibration/CMakeLists.txt` | Harness CMake independiente |
| `research/hand_calibration/run_tests.sh` | Compilación GCC/Clang sin CMake; sanitizers opcionales |
| `docs/research/hand_calibration_core_v1.md` | Contrato, decisiones y reporte de esta fase |

Archivos existentes modificados: **ninguno**.

## Contrato de datos y API

Flujo: `HandFeatures` → `HandCalibrationObservation` →
`HandCalibrationSession` → estadísticas → `HandCalibrationProfile`.

```cpp
#include "src/core/calibration/HandCalibrationSession.h"

HandCalibrationSession session; // 10 muestras mínimas por pose
HandCalibrationObservation observation;
observation.pose = HandCalibrationPose::OPEN_HAND;
observation.features = alreadyExtractedFeatures;
observation.normalizedLuma = 0.55; // opcional; omitir si no está disponible
const auto result = session.addObservation(observation);
// Repetir con las cuatro poses etiquetadas externamente.
auto profile = session.buildProfile();
// Consultar profile.valid / profile.status. No aplicar al runtime.
```

`features.handedness` es la única fuente de handedness en la observación. No se
duplica una etiqueta que pudiera contradecirla. Se acepta LEFT o RIGHT; la primera
observación completamente válida fija la mano. No se permite mezclar manos.

Las poses tienen IDs explícitos `uint16_t`: OPEN_HAND=1, POINTING=2,
PINCH_OPEN=3, PINCH_CLOSED=4. El orden de `kHandCalibrationPoses` define el índice
de los resúmenes por pose. Nunca se utiliza un cast del enum como índice.
Futuros ángulos/poses deben añadir IDs sin renumerar los existentes y definir su
papel estadístico/requisitos explícitamente. Los IDs desconocidos se rechazan.
No se promete estabilidad binaria del layout de C++; no existe serialización.

La versión del perfil es `kHandCalibrationProfileVersion = 1`, independiente de
RuntimeConfig. El perfil contiene handedness, conteos aceptados/rechazados y por
pose, estadísticas de escala, pinch cerrado/abierto, curl extendido/flexionado,
luminancia y confidence opcional. El pulgar conserva dos distribuciones por pose:
`thumbCurl` y `thumbSpreadRatio`. No mezcla roles diferentes del pulgar.

Cada resumen contiene sampleCount, P10, mediana y P90. sampleCount=0 significa
no disponible; sus ceros no deben interpretarse como mediciones. El intervalo
P10–P90 describe variabilidad empírica; no es un intervalo de confianza ni mide
por sí solo calidad del tracker. `trackingConfidence`, si existe, procede del
caller y se resume sin inferir una nueva confianza.

`acceptedSamples()`, `rejectedSamples()`, `sampleCount(pose)`, `handedness()` y
`ready()` permiten inspeccionar progreso. `ready()` significa que se completaron
las cuotas; **no** garantiza separabilidad. `buildProfile()` siempre devuelve
un perfil con estado, incluso incompleto; no consume ni cambia la sesión.
Las cuatro recomendaciones `optional<double>` sólo existen si todo el perfil es
válido. Los estados indican configuración inválida, muestras insuficientes,
pinch inseparable o curl inseparable (en ese orden de prioridad).

## Validación de observaciones y configuración

Se valida antes de almacenar y antes de fijar handedness:

- `HandFeatures.valid` debe ser true.
- Todos los valores numéricos de HandFeatures deben ser finitos, incluidas las
  coordenadas de palmCenter/pointerPoint, aunque estas no se retienen.
- `0 < handScale <= 4`; los cinco curls deben pertenecer a `[0,1]`.
- `pinchRatio` y `thumbSpreadRatio` deben pertenecer a `[0,10]`.
  Se exige pinch finito incluso fuera de las poses pinch: el extractor existente
  siempre entrega ese campo finito cuando marca `valid=true`.
- Coordenadas en `[-16,16]`. Son límites amplios para detectar corrupción, no una
  exigencia de que la mano esté dentro del encuadre. No se rechaza z negativo.
- UNKNOWN, valores inválidos del enum y la mano contraria se rechazan.
- normalizedLuma y trackingConfidence son opcionales; cuando existen deben ser
  finitos y estar en `[0,1]`. Un metadato inválido rechaza **toda** la observación;
  no se recorta ni se convierte silenciosamente en un valor ausente.
- Una pose llena rechaza muestras adicionales sin reemplazar las aceptadas.

Los límites geométricos son guardas de investigación para features calculados a
partir de coordenadas de imagen normalizadas; no son límites anatómicos medidos
ni umbrales de GestureConfig. No admiten features en píxeles/metrías arbitrarias.
`handScale` conserva la unidad del extractor: tamaño de palma en coordenadas
normalizadas de imagen. Varía con la distancia a la cámara; no es una medida
física de la mano. Los ratios pinch/spread sí están divididos por esa escala.

`addObservation()` devuelve una razón explícita. Un rechazo incrementa solamente
el contador de rechazos, saturado al máximo de size_t. No cambia distribuciones
ni fija la mano. Errores de asignación de memoria conservan el comportamiento
normal de C++ (`std::bad_alloc`); no se presentan como rechazos de datos.

Configuración predeterminada:

| Parámetro | Valor |
| --- | ---: |
| minimumSamplesPerPose | 10 |
| maximumSamplesPerPose | 256 |
| minimumPinchMedianSeparation | 0.10 |
| minimumPinchQuantileGap | 0.04 |
| minimumCurlMedianSeparation | 0.15 |
| minimumCurlQuantileGap | 0.05 |

Debe cumplirse `1 <= minimumSamplesPerPose <= maximumSamplesPerPose <= 4096`.
Las separaciones deben ser finitas, al menos `1e-6`, y como máximo 10 para pinch
o 1 para curl. Una configuración inválida no se corrige silenciosamente:
`configValid()` es false, las observaciones se rechazan y el perfil es inválido.
Permitir mínimos pequeños ayuda a experimentos/tests; no implica robustez con
una o dos muestras. El default de 10 es el punto inicial de esta investigación.

## Estadística robusta y fórmulas

Se ordena cada distribución y se utilizan percentiles tipo 7
(Hyndman–Fan, interpolación lineal):

```text
h = (n - 1) * p
i = floor(h), j = min(i + 1, n - 1), a = h - i
Q(p) = lerp(x[i], x[j], a)
p = 0.10, 0.50, 0.90
```

Para un dato, los tres percentiles coinciden. Para cero datos, sampleCount=0.
El helper rechaza cualquier NaN/Inf antes de ordenar y normaliza `-0.0` a `+0.0`.
`std::lerp` evita overflow al interpolar extremos finitos de signos opuestos.
No hay sumas dependientes del orden, aleatoriedad o promedios aritméticos como
estimador central. No se añaden MAD/IQR: los cuantiles ya proporcionan un control
de solapamiento sencillo y auditable.

Para pinch, L es PINCH_CLOSED y H es PINCH_OPEN. Se exige:

```text
median(H) - median(L) >= minimumPinchMedianSeparation
g = P10(H) - P90(L) >= minimumPinchQuantileGap
recommendedPinchEnterRatio = P90(L) + g/3
recommendedPinchExitRatio  = P90(L) + 2*g/3
```

Los thresholds quedan estrictamente dentro del espacio entre los rangos centrales
de ambas distribuciones. La histéresis es un tercio de ese espacio, con otro
tercio de margen a cada lado. Se verifica explícitamente:
`P90(L) < enter < exit < P10(H)`. Nunca se copian 0.25/0.35 del runtime.
Las colas extremas pueden solaparse; no se promete separación de cada muestra.

Para curl, L representa dedos extendidos y H dedos flexionados:

- OPEN_HAND aporta index/middle/ring/pinky como cuatro mediciones de extensión.
- POINTING aporta index a extensión y middle/ring/pinky a flexión.
- El índice de POINTING **jamás** se usa como ejemplo flexionado.
- PINCH_OPEN/CLOSED no aportan curl de dedos a estas clases: el nombre de la
  pose no garantiza su estado de extensión.

Se reúnen mediciones individuales semánticamente compatibles para recomendar un
threshold común. Por ello, 10 OPEN_HAND + 10 POINTING producen 50 valores
extendidos y 30 flexionados. Son mediciones agrupadas, **no** muestras
estadísticamente independientes ni 80 frames distintos. Las cuotas se comprueban
por observación/pose, no con los conteos de dedos.

Las fórmulas son las mismas con las separaciones de curl:

```text
g = P10(curled) - P90(extended)
recommendedFingerExtendedMaxCurl = P90(extended) + g/3
recommendedFingerCurledMinCurl   = P90(extended) + 2*g/3
```

Se exige la separación de medianas y el gap configurados, y se verifica la
desigualdad estricta entre recomendaciones y cuantiles. Si falla pinch o curl,
el perfil completo es inválido y no expone recomendaciones parciales.

## Determinismo y límites de validez

El mismo multiconjunto **aceptado**, configuración y mano produce exactamente el
mismo perfil en el mismo entorno de coma flotante. Se compara el perfil completo
en tests con inversión y 20 permutaciones de semilla fija. Construirlo repetidas
veces no altera la sesión. No se promete identidad bit a bit entre todas las
bibliotecas/arquitecturas; los thresholds tienen guardas de separación numérica.

La política de adquisición sí importa: en un stream de ambas manos gana la
primera mano válida, como requiere el contrato. Si se excede la capacidad,
se conservan las primeras muestras aceptadas por pose. Permutar un stream que
cambia ese subconjunto no puede garantizar el mismo perfil. No hay muestreo
aleatorio ni reemplazo implícito al llegar al límite.

P10/P90 toleran una pequeña minoría de extremos; no garantizan inmunidad cuando
la contaminación alcanza sus colas de aproximadamente 10%, especialmente con
pocos datos y percentiles interpolados. El test de outlier acepta un pinch de
10.0 frente a valores habituales de 0.12–0.14, y una observación OPEN_HAND con
todos los curls en 1.0, sin perder una calibración con 20 muestras por pose.
Los valores fuera de las guardas se rechazan antes de la estadística.

No se verifica automáticamente que una etiqueta de pose sea correcta, que frames
sean independientes ni que las muestras cubran suficientes ángulos/iluminaciones.
La separación de clases detecta algunos fallos de etiquetado, no todos.
No existe seguimiento temporal, medición de jitter temporal o detección de una
muestra duplicada. Agrupar dedos puede ocultar diferencias por dedo; una fase
posterior deberá evaluar si hacen falta resúmenes/requisitos individuales.

`thumbCurl` del extractor actual incluye un término dependiente de
`GestureConfig::thumbMinSpreadRatio`. Se conserva como diagnóstico por pose,
sin presentarlo como medida anatómica independiente ni producir thresholds de
pulgar. Los flags `*Extended` ya clasificados por el runtime se ignoran.
La luminancia describe exclusivamente las observaciones que la proporcionan;
no clasifica LOW_LIGHT/NORMAL/BRIGHT ni evalúa condiciones de exposición.

## Memoria, coste y privacidad

La sesión conserva un registro compacto por muestra: ocho doubles geométricos y
dos `optional<double>`. No conserva el HandFeatures completo, coordenadas, flags,
landmarks, imágenes, timestamps ni identificadores de personas.

En el entorno Linux x86-64/GCC 13 usado aquí:

| Elemento | Memoria |
| --- | ---: |
| Perfil autónomo (`sizeof`, medido) | 608 bytes |
| Objeto sesión sin almacenamiento dinámico (`sizeof`, medido) | 168 bytes |
| Observación de entrada (`sizeof`, medido; no se conserva entera) | 176 bytes |
| Registro retenido (estimación por layout ABI) | 96 bytes |
| 40 muestras, capacidad inicial 10 por pose | ~3.75 KiB |
| Capacidad default máxima, 4 × 256 registros | ~96 KiB |
| Scratch máximo default de buildProfile, 5 × 256 doubles | ~10 KiB |

Estos valores excluyen overhead del allocator. El crecimiento geométrico puede
mantener temporalmente el bloque anterior y el nuevo durante una reasignación.
El tope absoluto configurable de 4096 por pose supone ~1.5 MiB retenidos y
~160 KiB de scratch. Normalmente sólo se necesitan decenas/cientos de muestras.
La sesión crece por bloques; no reserva el máximo por adelantado ni realiza una
asignación por cada observación. El scratch se reutiliza entre distribuciones.

Inserción: O(1) amortizado, con validación constante; crecimiento ocasional O(n).
Perfil: O(N log N) en total para un número fijo de distribuciones y O(N) de scratch.
Cada distribución se ordena separadamente; algunos recorridos se repiten para
mantener el código pequeño. No hay threads, locks, Qt, OpenCV, MediaPipe ni otras
dependencias enlazadas. El código requiere C++20, como el proyecto actual.

El perfil contiene valores agregados por valor, sin referencias a la sesión:
se puede destruir la sesión inmediatamente después. No hay persistencia, red,
fotografías, reconocimiento de identidad ni construcción de plantillas biométricas
identificativas. Sólo medidas geométricas normalizadas y metadatos opcionales
necesarios para estudiar la personalización de gestos.

## Compilación y pruebas

Desde la raíz del repositorio, sin CMake:

```sh
bash research/hand_calibration/run_tests.sh
ASAN_OPTIONS=detect_leaks=0 UBSAN_OPTIONS=halt_on_error=1 \
  SANITIZE=1 bash research/hand_calibration/run_tests.sh
```

La desactivación de LeakSanitizer en ese segundo comando es específica del entorno
de esta validación, que usa ptrace. En una máquina compatible puede omitirse
`ASAN_OPTIONS=detect_leaks=0` para habilitar también la detección de fugas.

Con CMake (Linux o Windows con compilador C++20):

```sh
cmake -S research/hand_calibration -B build-hand-calibration
cmake --build build-hand-calibration --config Release
ctest --test-dir build-hand-calibration -C Release --output-on-failure
```

Los tests usan comprobaciones explícitas que funcionan con `NDEBUG`. No enlazan
HandFeatureExtractor, GestureEngine, Qt, OpenCV, Windows SDK o libei. El harness
no agrega tests al CMake de Phase 11. GCC/Clang usan
`-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror`; MSVC usa
`/W4 /WX /permissive-`. El script Bash usa GCC/Clang; para MSVC se usa CMake.

Los 28 casos cubren sesión válida, cuotas incompletas por cada pose, NaN/Inf en
todos los campos numéricos, handedness, percentiles pares/impares y extremos,
outliers aceptados, ambos pares de thresholds, medianas y colas solapadas,
etiquetas pinch invertidas, determinismo, luminancia ausente/inválida y cuantiles,
valores imposibles, configuración inválida/separaciones configurables, capacidad,
poses desconocidas, metadatos antes del bloqueo de mano, pulgar/confidence,
inmutabilidad de build/lifetime, flags preclasificados y sesión vacía.

## Resultados ejecutados en esta entrega

Entorno: Linux x86-64, GCC 13.3.0, libstdc++, C++20. CMake y Clang no están
instalados en el entorno disponible. No se instalaron dependencias adicionales.

| Validación | Resultado exacto |
| --- | --- |
| Harness Bash, `-O2 -DNDEBUG`, warnings estrictos como errores | 28/28 casos PASS; exit 0; cero warnings |
| Mismo test, `-O1 -g`, AddressSanitizer + UndefinedBehaviorSanitizer | 28/28 casos PASS; exit 0; sin diagnósticos; LeakSanitizer desactivado |
| Primer intento con LeakSanitizer habilitado | exit 1: `LeakSanitizer does not work under ptrace`; no se declara PASS |
| Tests core existentes, compilación directa sin alterar CMake | 25/25 ejecutables compilaron y terminaron con exit 0 |
| `bash tests/tooling/test_model_tooling.sh` | PASS; exit 0; modelo válido e inalterado y fallo de descarga controlado |
| Sintaxis del harness Bash y `git diff --check` | exit 0 |
| Diff frente a `00f0be3` | Sólo archivos nuevos; ningún archivo existente modificado |

Los 25 tests core se compilaron con `c++ -std=c++20 -O0 -g -Wall -Wextra
-Wpedantic -pthread -I <raíz>`, sin NDEBUG, utilizando exactamente los archivos
de cada llamada `add_core_test` del CMake existente. Para `test_runtime_paths` se
conservó la definición `C0NTROL_MODEL_RELATIVE_DIR="models"`; para
`test_build_metadata` se conservaron las seis definiciones de su target original.
Se ejecutó cada binario, no sólo su compilación:

```text
test_display_transform       test_dynamic_gestures       test_frame_sync
test_hand_geometry           test_hand_features          test_gesture_state_machine
test_gesture_pipeline        test_pointer_mapper         test_action_dispatcher
test_runtime_config          test_settings_serialization test_pointer_calibration
test_latest_frame_slot       test_async_capture          test_pipeline_metrics
test_native_input_lifecycle  test_async_input_runtime    test_windows_pointer_math
test_eis_regions             test_linux_eis_backend      test_one_euro
test_landmark_filter_bank    test_hand_tracking          test_runtime_paths
test_build_metadata
```

Se observaron tres warnings en código existente e inalterado: dos
`-Wmissing-field-initializers` para `FakeCameraConfig::fatalAfterFrames` en
`test_async_capture.cpp`, y un `-Wunused-function` para `enabledInput()` en
`test_native_input_lifecycle.cpp`. No se corrigieron dentro de esta investigación.

**No ejecutado / no disponible:** configuración y CTest de ambos proyectos por
ausencia de CMake, `test_production_build_guard` por la misma razón, compilación
con Clang o MSVC/Windows SDK, `test_windows_input_backend_compile`,
`test_linux_eis_backend_compile` con libei/liboeffis reales, ambos tests de
MediaPipe con el bridge real, GUI, cámara, acciones nativas, empaquetado y
regresión real de Windows. `test_windows_pointer_math` es portable y sí se
ejecutó en Linux; eso no equivale a validar el backend Windows.
`test_linux_eis_backend` usa el soporte fake existente, no una sesión Wayland real.

El código nuevo usa únicamente C++20 estándar y no contiene ramas de plataforma,
pero la compilación MSVC y los flags del harness CMake quedan por comprobar en
Windows. No se promete ausencia absoluta de regresiones no observadas: la
evidencia de aislamiento es que ningún archivo existente cambia y no existe
integración del módulo en el runtime o en su grafo de build.

## Próxima fase propuesta (no implementada)

Fase 2: crear un evaluador offline, todavía fuera de los targets de producción,
que reciba lotes de HandFeatures etiquetados y evalúe las recomendaciones sobre
muestras distintas de las usadas para calibrar. Comparar errores de pinch/curl,
cobertura por dedo, sensibilidad a outliers, tamaño muestral, ángulos y luminancia.
Definir un protocolo de adquisición y criterios de aceptación antes de conectar
frames reales. Mantener separados entrenamiento de parámetros y evaluación;
esto sigue siendo estadística convencional, no machine learning.

Una integración posterior, autorizada por separado, podría suministrar features
desde un adaptador de captura y presentar el perfil al usuario. Requerirá diseñar
cancelación, invalidación/versionado, política de aplicación y regresiones de
input antes de tocar GestureConfig, configuración persistente o el runtime.
Esta fase no implementa GUI, fotos, captura, persistencia, calibración continua,
clasificación automática de luz/poses, acciones nativas ni entrenamiento de modelos.
