PAC-MAN — Raspberry Pi Zero 2W + GMT130 v1.0 (ST7789 240x240)
==============================================================

CONEXIÓN DE PINES
-----------------
GMT130  →  RPi Zero 2W (header 40 pines)

VCC     →  Pin 17  (3.3V)
GND     →  Pin 20  (GND)
SCK     →  Pin 23  (GPIO 11, SPI0 SCLK)
SDATA   →  Pin 19  (GPIO 10, SPI0 MOSI)
DC      →  Pin 22  (GPIO 25)
RST     →  Pin 18  (GPIO 24)
BL      →  Pin 16  (GPIO 23)

BUZZER (opcional):
+  →  Pin 32  (GPIO 12)
-  →  GND

NOTA: El módulo GMT130 v1.0 no expone CS. El CS está conectado
internamente a GND (siempre activo). El driver spidev maneja
esto correctamente.


PREPARACIÓN DEL SISTEMA (una sola vez)
---------------------------------------

1) Habilitar SPI en el sistema:

   sudo nano /boot/firmware/config.txt
   # (en RPi OS anterior a Bookworm: /boot/config.txt)

   Agregar o descomentar:
     dtparam=spi=on

   Guardar y reiniciar:
     sudo reboot

2) Verificar que SPI está disponible:

   ls /dev/spidev0.0
   # Debe aparecer /dev/spidev0.0

3) (Opcional) Agregar usuario al grupo spi y gpio para no usar sudo:

   sudo usermod -aG spi,gpio $USER
   # Luego cerrar sesión y volver a entrar


COMPILAR Y EJECUTAR
--------------------

   make
   sudo ./pacman

   # Si hay error de SPI:
   make check


DIAGNÓSTICO DE PROBLEMAS
-------------------------

"open /dev/spidev0.0: No such file or directory"
  → SPI no está habilitado. Ver paso 1 de preparación.

"open /dev/gpiochip0: Permission denied"
  → Ejecutar con sudo, o agregar usuario al grupo gpio.

Pantalla en blanco / no enciende:
  → Verificar conexión física (VCC=3.3V, no 5V)
  → Verificar que BL esté conectado (backlight)
  → El script enciende BL después de init_display()

No hay sonido:
  → El buzzer debe ser PASIVO (no activo)
  → Conectar entre GPIO 12 (Pin 32) y GND
  → Agregar resistencia de 100Ω en serie para limitar corriente


ESTRUCTURA DE ARCHIVOS
-----------------------

main.cpp         — Hardware: GPIO (gpiochip), SPI (spidev), init display
HardwareProfile.h — Configuración de pines y constantes
Graphics.cpp/h   — Primitivas gráficas, sprites Pac-Man y fantasmas
GameEngine.cpp/h — Lógica del juego: laberinto, IA, niveles, power pellets
Sound.cpp/h      — Tonos por bit-bang en GPIO
fonts.h          — Fuente 5x7 bitmap
Makefile         — Build


DIFERENCIAS CON LA VERSIÓN PIC32MX795
---------------------------------------

PIC32                    →  Raspberry Pi Zero 2W
-----------                 ------------------
SPI4 registros directos  →  /dev/spidev0.0 (ioctl)
LATBbits / TRISBbits     →  /dev/gpiochip0 (ioctl GPIO_V1)
SPI4BRG=0 → 40 MHz       →  SPI_SPEED_HZ=40000000 (mismo)
CKP=1 CKE=0 (Mode 3)     →  SPI_MODE_3 (mismo)
_CP0_GET_COUNT()         →  clock_gettime(CLOCK_MONOTONIC)
delay_cycles(40000)      →  nanosleep()
#pragma config FPLL...   →  (no aplica, kernel gestiona CPU)
