CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -Wpedantic -O2 -pthread -I. -I./include
LDFLAGS := -lrt -pthread

BUILD_DIR := build
BIN_DIR := bin
SRC_DIR := src
TESTS_DIR := tests

CORE_SRCS := $(wildcard $(SRC_DIR)/core/*.cpp)
HAL_SRCS  := $(wildcard $(SRC_DIR)/hal/*.cpp)
IPC_SRCS  := $(wildcard $(SRC_DIR)/ipc/*.cpp)
UTILS_SRCS:= $(wildcard $(SRC_DIR)/utils/*.cpp)
UI_SRCS   := $(wildcard $(SRC_DIR)/ui/*.cpp)
APP_SRCS  := $(CORE_SRCS) $(HAL_SRCS) $(IPC_SRCS) $(UTILS_SRCS) $(UI_SRCS) $(SRC_DIR)/main.cpp
TEST_SRCS := $(CORE_SRCS) $(HAL_SRCS) $(IPC_SRCS) $(UTILS_SRCS) $(TESTS_DIR)/test_runner.cpp

APP_TARGET := $(BIN_DIR)/library_system
TEST_TARGET := $(BIN_DIR)/test_runner

.PHONY: all app test driver clean demo help install-driver uninstall-driver

all: app test driver

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

app: $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(APP_SRCS) -o $(APP_TARGET) $(LDFLAGS)
	@echo "Built: $(APP_TARGET)"

test: $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $(TEST_SRCS) -o $(TEST_TARGET) $(LDFLAGS)
	@echo "Running SmartLMS test suite..."
	./$(TEST_TARGET)

driver:
	@echo "Building Linux Kernel Device Driver..."
	$(MAKE) -C driver all

demo: app
	./$(APP_TARGET) --demo

clean:
	rm -rf $(BIN_DIR)/* $(BUILD_DIR)/*
	$(MAKE) -C driver clean 2>/dev/null || true
	@echo "Clean completed."

install-driver:
	$(MAKE) -C driver install

uninstall-driver:
	$(MAKE) -C driver uninstall

help:
	@echo "SmartLMS Build System Targets:"
	@echo "  make all             - Build user-space application, tests, and kernel driver"
	@echo "  make app             - Build main SmartLMS binary ($(APP_TARGET))"
	@echo "  make test            - Build and run comprehensive automated test runner"
	@echo "  make driver          - Build Linux character device driver module (libsys_driver.ko)"
	@echo "  make demo            - Run automated end-to-end demonstration"
	@echo "  make clean           - Remove all build artifacts and test binaries"
	@echo "  make install-driver  - Insert kernel module into running Linux kernel (requires sudo)"
	@echo "  make uninstall-driver- Remove kernel module from Linux kernel (requires sudo)"
