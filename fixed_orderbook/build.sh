#!/bin/bash
# ═══════════════════════════════════════════════════════════════
#  HFT Order Book – build.sh
#  Run this script to compile and launch the order book.
# ═══════════════════════════════════════════════════════════════

set -e  # exit on any error

COLOR_GREEN="\033[0;32m"
COLOR_CYAN="\033[0;36m"
COLOR_RED="\033[0;31m"
COLOR_YELLOW="\033[1;33m"
COLOR_RESET="\033[0m"

info()    { echo -e "${COLOR_CYAN}[INFO]${COLOR_RESET} $1"; }
success() { echo -e "${COLOR_GREEN}[OK]${COLOR_RESET}   $1"; }
warn()    { echo -e "${COLOR_YELLOW}[WARN]${COLOR_RESET} $1"; }
error()   { echo -e "${COLOR_RED}[ERR]${COLOR_RESET}  $1"; exit 1; }

# ── 1. Check compiler ──────────────────────────────────────────
info "Checking for g++..."
if ! command -v g++ &>/dev/null; then
    error "g++ not found. Install with: sudo apt install g++  (Ubuntu/Debian)"
fi
GCC_VER=$(g++ --version | head -1)
success "Found: $GCC_VER"

# ── 2. Compile tests ──────────────────────────────────────────
info "Building test suite..."
g++ -std=c++17 -O2 -Wall -Iinclude -pthread \
    tests/test_orderbook.cpp -o tests/run_tests
success "Tests built → tests/run_tests"

# ── 3. Run tests ──────────────────────────────────────────────
info "Running tests..."
if ./tests/run_tests; then
    success "All tests passed!"
else
    error "Tests failed. Fix before running the engine."
fi

# ── 4. Compile main binary ────────────────────────────────────
info "Building main binary (Release, -O3)..."
g++ -std=c++17 -O3 -march=native -Wall -Iinclude -pthread \
    src/main.cpp -o orderbook
success "Main binary built → ./orderbook"

echo ""
echo -e "${COLOR_GREEN}═══════════════════════════════════════════════${COLOR_RESET}"
echo -e "${COLOR_GREEN}  Build complete! How to run:${COLOR_RESET}"
echo -e "${COLOR_GREEN}═══════════════════════════════════════════════${COLOR_RESET}"
echo ""
echo "  # Default run (AAPL, 5000 ops/s, 30 seconds)"
echo "  ./orderbook"
echo ""
echo "  # Custom symbol and speed"
echo "  ./orderbook --symbol TSLA --ops 5000 --mid 25000"
echo ""
echo "  # See every trade printed"
echo "  ./orderbook --verbose"
echo ""
echo "  # Run forever (Ctrl+C to stop)"
echo "  ./orderbook --run 0"
echo ""
echo "  # Run tests only"
echo "  ./tests/run_tests"
echo ""
echo "  # Help / all flags"
echo "  ./orderbook --help"
echo ""

# ── 5. Optional: auto-launch ──────────────────────────────────
if [[ "$1" == "--run" ]]; then
    info "Launching order book..."
    ./orderbook "${@:2}"
fi
