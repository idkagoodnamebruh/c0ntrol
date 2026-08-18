# Makefile para compilación rápida con CMake y Qt6/OpenCV

BUILD_DIR = build
TEST_BUILD_DIR = build-tests
TARGET = $(BUILD_DIR)/c0ntrol

.PHONY: all clean rebuild run test help

all: $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): | $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. && $(MAKE) -j$$(nproc)

clean:
	rm -rf $(BUILD_DIR) $(TEST_BUILD_DIR)

rebuild: clean all

run: $(TARGET)
	./$(TARGET)

test:
	cmake -S . -B $(TEST_BUILD_DIR) -DBUILD_APP=OFF -DBUILD_TESTING=ON
	cmake --build $(TEST_BUILD_DIR) --parallel $$(nproc)
	ctest --test-dir $(TEST_BUILD_DIR) --output-on-failure

help:
	@echo "Opciones disponibles:"
	@echo "  make all       - Compila el proyecto en ./build"
	@echo "  make run       - Compila y ejecuta el ejecutable c0ntrol"
	@echo "  make clean     - Elimina el directorio ./build"
	@echo "  make rebuild   - Limpia y vuelve a compilar"
	@echo "  make test      - Compila y ejecuta las pruebas unitarias"
