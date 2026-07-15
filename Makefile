# Skeleton Board V3 Makefile
# For ESP32-S3-WROOM-1-N8

PROJECT_NAME = skeleton-board-v3
PLATFORM = espressif32
BOARD = esp32-s3-devkitc-1
ENVIRONMENT = esp32-s3-wroom-1-n8

PIO = pio

.PHONY: all
all: build

.PHONY: build compile
build compile:
	@echo "Building $(PROJECT_NAME)..."
	$(PIO) run -e $(ENVIRONMENT)

.PHONY: clean
clean:
	@echo "Cleaning build files..."
	$(PIO) run -t clean -e $(ENVIRONMENT)

.PHONY: upload flash
upload flash:
	@echo "Uploading firmware to ESP32-S3..."
	$(PIO) run -t upload -e $(ENVIRONMENT)

.PHONY: ota-upload
ota-upload:
	@echo "Uploading firmware via OTA to $(ESP_IP)..."
	$(PIO) run -t upload --upload-port $(ESP_IP) -e $(ENVIRONMENT)

.PHONY: ota-deploy
ota-deploy: build ota-upload

.PHONY: deploy
deploy: build upload

.PHONY: monitor serial
monitor serial:
	@echo "Starting serial monitor..."
	$(PIO) device monitor -e $(ENVIRONMENT)

.PHONY: run
run: deploy monitor

.PHONY: deps dependencies
deps dependencies:
	@echo "Installing/updating dependencies..."
	$(PIO) lib install

.PHONY: devices
devices:
	@echo "Connected devices:"
	$(PIO) device list

.PHONY: check
check:
	@echo "Checking code..."
	$(PIO) check -e $(ENVIRONMENT)

.PHONY: info
info:
	@echo "Project: $(PROJECT_NAME)"
	@echo "Platform: $(PLATFORM)"
	@echo "Board: $(BOARD)"
	@echo "Environment: $(ENVIRONMENT)"
	$(PIO) project config

.PHONY: erase
erase:
	@echo "Erasing flash memory..."
	$(PIO) run -t erase -e $(ENVIRONMENT)

.PHONY: update
update:
	@echo "Updating PlatformIO..."
	$(PIO) upgrade
	$(PIO) platform update

.PHONY: rebuild
rebuild: clean build

.PHONY: init setup
init setup:
	@echo "Initializing project..."
	$(PIO) project init --board $(BOARD)
	$(PIO) lib install

.PHONY: help
help:
	@echo "Available targets:"
	@echo "  build/compile - Build the project"
	@echo "  clean         - Clean build files"
	@echo "  upload/flash  - Upload firmware to device"
	@echo "  ota-upload    - Upload via OTA (set ESP_IP=x.x.x.x)"
	@echo "  ota-deploy    - Build + OTA upload"
	@echo "  deploy        - Build and upload (serial)"
	@echo "  monitor/serial- Start serial monitor"
	@echo "  run           - Build, upload, and monitor"
	@echo "  deps          - Install/update dependencies"
	@echo "  devices       - List connected devices"
	@echo "  check         - Check/lint code"
	@echo "  info          - Show project information"
	@echo "  erase         - Erase flash memory"
	@echo "  update        - Update PlatformIO and platforms"
	@echo "  rebuild       - Clean and rebuild"
	@echo "  init/setup    - Initialize project"
	@echo "  help          - Show this help"
