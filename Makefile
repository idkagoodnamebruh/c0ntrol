# Makefile para compilación rápida con CMake y Qt6/OpenCV

BUILD_DIR = build
TARGET = $(BUILD_DIR)/c0ntrol

.PHONY: all clean rebuild run test help

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): | $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. && $(MAKE) -j$$(nproc)

clean:
	rm -rf $(BUILD_DIR)

rebuild: clean all

run: $(TARGET)
	./$(TARGET)

test: | $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake -DBUILD_TESTS=ON .. && $(MAKE) -j$$(nproc) && ctest --output-on-failure

help:
	@echo "Opciones disponibles:"
	@echo "  make all       - Compila el proyecto en ./build"
	@echo "  make run       - Compila y ejecuta el ejecutable c0ntrol"
	@echo "  make clean     - Elimina el directorio ./build"
	@echo "  make rebuild   - Limpia y vuelve a compilar"
	@echo "  make test      - Compila y ejecuta las pruebas unitarias"
