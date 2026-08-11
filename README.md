# c0ntrol 🖐️🖥️

Aplicación nativa de baja latencia desarrollada en **C++20**, **Qt6** y **OpenCV** para el control del sistema y del cursor mediante gestos manuales en tiempo real.

---

## 🏗️ Estructura del Proyecto

```text
Stark45/ (c0ntrol)
├── CMakeLists.txt              # Configuración de compilación CMake
├── Makefile                    # Target helpers (make, make run, make test)
├── .gitignore                  # Exclusiones de binarios y temporales de build
├── models/                     # Modelos neurales de MediaPipe / ONNX / TFLite
├── scripts/
│   └── download_hand_model.sh  # Script automatizado de descarga de modelos
├── docs/                       # Documentación de arquitectura y auditoría
├── src/                        # Código fuente C++ (Core, Vision, Actions, GUI)
│   ├── core/                   # Visión, Filtros OneEuro, Clasificador y Acciones
│   └── gui/                    # Ventanas y Overlays Qt6 (Developer, Minimalist, Matrix)
└── tests/                      # Suite de pruebas unitarias
```

---

## ⚡ Características Principales

- **Captura Asíncrona Multi-hilo (`VisionWorker`)**: Captura a 30-60 FPS sin congelar la interfaz de usuario Qt6.
- **Suavizado Adaptativo (`OneEuroFilter`)**: Implementación del *One Euro Filter* en 3D para eliminar temblores de la mano.
- **Clasificador Geométrico de Gestos (`GestureClassifier`)**:
  - `POINTING`: Mapeo de posición de cursor.
  - `PINCH`: Clic de ratón.
  - `PALM_OPEN` / `FIST` / `VICTORY`: Control de eventos del sistema.
  - `DynamicGestureTracker`: Detección de desplazamientos veloces (*Swipes*).
- **Modos de Interfaz Qt6**:
  - **Developer Mode**: Telemetría, FPS, latencia y logs en directo.
  - **Minimalist HUD**: Overlay semitransparente flotante.
  - **Matrix Rain**: Efecto visual interactivo.

---

## 🚀 Compilación y Ejecución

### Requisitos Previos
- Compilador con soporte C++20 (`g++` o `clang++`)
- Qt6 (`Core`, `Gui`, `Widgets`)
- OpenCV 4.x
- CMake 3.16+

### Instrucciones

```bash
# 1. Clonar el repositorio
git clone https://github.com/idkagoodnamebruh/c0ntrol.git
cd c0ntrol

# 2. Descargar modelos de MediaPipe (opcional, el script descarga automáticamente si no existen)
./scripts/download_hand_model.sh

# 3. Compilar la aplicación
make

# 4. Ejecutar c0ntrol
make run

# 5. Ejecutar la suite de pruebas unitarias
make test
```
