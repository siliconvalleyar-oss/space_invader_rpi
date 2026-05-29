# ============================================================
# Makefile para Space Shooter en Raspberry Pi Zero 2W + ST7789
# ============================================================

TARGET = pacman
CC = g++
CFLAGS = -O2 -std=c++11 -Wall -Iinclude
LDFLAGS = -lpthread

# Directorios
SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

# Archivos fuente
SOURCES = $(wildcard $(SRCDIR)/*.cpp)
OBJECTS = $(SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)

# Ejecutable final
TARGET_BIN = $(BINDIR)/$(TARGET)

# Colores para output
GREEN = \033[0;32m
RED = \033[0;31m
YELLOW = \033[1;33m
NC = \033[0m

.PHONY: all clean run debug help

all: $(TARGET_BIN)

$(TARGET_BIN): $(OBJECTS)
	@echo -e "$(GREEN)Linkeando...$(NC)"
	@mkdir -p $(BINDIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo -e "$(GREEN)✓ Compilación completada: $(TARGET_BIN)$(NC)"

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@echo -e "$(YELLOW)Compilando $<...$(NC)"
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo -e "$(RED)Limpiando...$(NC)"
	@rm -rf $(OBJDIR)/*.o $(BINDIR)/$(TARGET)
	@echo -e "$(GREEN)✓ Limpieza completada$(NC)"

run: $(TARGET_BIN)
	@echo -e "$(GREEN)Ejecutando Space Shooter...$(NC)"
	@echo -e "$(YELLOW)Nota: Ejecutar con sudo para GPIO/SPI$(NC)"
	sudo $(TARGET_BIN)

debug: CFLAGS += -g -DDEBUG
debug: $(TARGET_BIN)
	@echo -e "$(GREEN)Ejecutando en modo debug...$(NC)"
	sudo $(TARGET_BIN)

help:
	@echo -e "$(GREEN)Space Shooter Makefile$(NC)"
	@echo "  make      - Compilar el juego"
	@echo "  make run  - Compilar y ejecutar (requiere sudo)"
	@echo "  make debug- Compilar con símbolos de depuración"
	@echo "  make clean- Limpiar archivos objeto y ejecutable"
	@echo "  make help - Mostrar esta ayuda"

# Verificar que existe el SPI
check:
	@echo -e "$(YELLOW)Verificando configuración del sistema...$(NC)"
	@if [ -e /dev/spidev0.0 ]; then \
		echo -e "$(GREEN)✓ SPI disponible$(NC)"; \
	else \
		echo -e "$(RED)✗ SPI no disponible. Añadir dtparam=spi=on a /boot/config.txt$(NC)"; \
	fi
	@if [ -e /sys/class/pwm/pwmchip0 ]; then \
		echo -e "$(GREEN)✓ PWM disponible$(NC)"; \
	else \
		echo -e "$(YELLOW)! PWM no disponible. Sonido por GPIO bit-bang$(NC)"; \
	fi
