#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR=""
SKILL_DIR=""
PLATFORM=""
JOBS="${JOBS:-12}"

usage() {
  cat <<'EOF'
Usage: build-release.sh [-p platform] [options]

Options:
  -p, --platform   all, linux, linux-x86_64, linux-aarch64, macos, macos-x86_64, macos-aarch64
  -b, --build      Build root directory
  -s, --src        SweetShot source directory
  --skill-dir      Skill directory to update
  -h, --help       Show this help message
EOF
}

parse_args() {
  while [ $# -gt 0 ]; do
    case "$1" in
      -p|--platform)
        PLATFORM="$2"
        shift 2
        ;;
      -b|--build)
        BUILD_DIR="$2"
        shift 2
        ;;
      -s|--src)
        PROJECT_DIR="$2"
        shift 2
        ;;
      --skill-dir)
        SKILL_DIR="$2"
        shift 2
        ;;
      -h|--help)
        usage
        exit 0
        ;;
      *)
        echo "Unknown option: $1" >&2
        usage >&2
        exit 1
        ;;
    esac
  done
}

resolve_dirs() {
  PROJECT_DIR="$(cd "$PROJECT_DIR" && pwd)"
  if [ -z "$BUILD_DIR" ]; then
    BUILD_DIR="$PROJECT_DIR/build/release"
  fi
  if [ -z "$SKILL_DIR" ]; then
    SKILL_DIR="$PROJECT_DIR/skill/sweetshot-code-screenshot"
  fi
  mkdir -p "$BUILD_DIR"
  BUILD_DIR="$(cd "$BUILD_DIR" && pwd)"
  mkdir -p "$SKILL_DIR"
  SKILL_DIR="$(cd "$SKILL_DIR" && pwd)"
}

host_platform() {
  case "$(uname -s)" in
    Linux) echo "linux" ;;
    Darwin) echo "macos" ;;
    *) echo "unsupported" ;;
  esac
}

host_arch() {
  case "$(uname -m)" in
    x86_64|amd64) echo "x86_64" ;;
    arm64|aarch64) echo "aarch64" ;;
    *) uname -m ;;
  esac
}

section() {
  printf '\n============================= %s =============================\n' "$1"
}

ensure_command() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Required command not found: $1" >&2
    exit 1
  fi
}

ensure_rust_target() {
  local target="$1"
  if command -v rustup >/dev/null 2>&1 && ! rustup target list --installed | grep -qx "$target"; then
    echo "Rust target is not installed: $target" >&2
    echo "Run: rustup target add $target" >&2
    exit 1
  fi
}

find_sweetshot_binary() {
  local build_dir="$1"
  local name="$2"
  local candidates=(
    "$build_dir/bin/$name"
    "$build_dir/bin/Release/$name"
    "$build_dir/$name"
    "$build_dir/Release/$name"
  )

  local candidate
  for candidate in "${candidates[@]}"; do
    if [ -f "$candidate" ]; then
      printf '%s\n' "$candidate"
      return 0
    fi
  done

  echo "Built sweetshot binary was not found under $build_dir" >&2
  return 1
}

configure_and_build() {
  local build_dir="$1"
  shift

  cmake -S "$PROJECT_DIR" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DSWEETSHOT_BUILD_CLI=ON \
    -DSWEETSHOT_BUILD_TESTS=OFF \
    -DSWEETSHOT_PNG_BACKEND=resvg \
    "$@"

  cmake --build "$build_dir" --target sweetshot --config Release --parallel "$JOBS"
}

copy_binary() {
  local source="$1"
  local destination="$2"

  mkdir -p "$(dirname "$destination")"
  cp -f "$source" "$destination"
  chmod +x "$destination"
  echo "Updated $destination"
}

build_linux_arch() {
  local arch="$1"
  local build_dir="$BUILD_DIR/linux/$arch"
  local output="$SKILL_DIR/bin/linux/$arch/sweetshot"
  local cmake_args=()

  section "Linux $arch"
  if [ "$(host_platform)" != "linux" ]; then
    echo "Linux builds require Linux." >&2
    exit 1
  fi

  if [ "$arch" = "aarch64" ]; then
    ensure_rust_target "aarch64-unknown-linux-gnu"
    if [ "$(host_arch)" != "aarch64" ]; then
      ensure_command "aarch64-linux-gnu-gcc"
      ensure_command "aarch64-linux-gnu-g++"
      cmake_args+=("-DCMAKE_TOOLCHAIN_FILE=$PROJECT_DIR/cmake/linux-aarch64-toolchain.cmake")
    fi
  elif [ "$arch" = "x86_64" ]; then
    ensure_rust_target "x86_64-unknown-linux-gnu"
  else
    echo "Unsupported Linux architecture: $arch" >&2
    exit 1
  fi

  configure_and_build "$build_dir" "${cmake_args[@]}"
  copy_binary "$(find_sweetshot_binary "$build_dir" sweetshot)" "$output"
}

build_macos_arch() {
  local arch="$1"
  local cmake_arch="$arch"
  local rust_target=""
  local build_dir="$BUILD_DIR/macos/$arch"
  local output="$SKILL_DIR/bin/macos/$arch/sweetshot"

  section "macOS $arch"
  if [ "$(host_platform)" != "macos" ]; then
    echo "macOS builds require macOS." >&2
    exit 1
  fi

  if [ "$arch" = "aarch64" ]; then
    cmake_arch="arm64"
    rust_target="aarch64-apple-darwin"
  elif [ "$arch" = "x86_64" ]; then
    rust_target="x86_64-apple-darwin"
  else
    echo "Unsupported macOS architecture: $arch" >&2
    exit 1
  fi
  ensure_rust_target "$rust_target"

  configure_and_build "$build_dir" "-DCMAKE_OSX_ARCHITECTURES=$cmake_arch"
  copy_binary "$(find_sweetshot_binary "$build_dir" sweetshot)" "$output"
}

run_platform() {
  local platform="$1"
  if [ -z "$platform" ]; then
    platform="$(host_platform)"
  fi

  case "$platform" in
    all)
      case "$(host_platform)" in
        linux) run_platform linux ;;
        macos) run_platform macos ;;
        *) echo "Unsupported host platform for all: $(uname -s)" >&2; exit 1 ;;
      esac
      ;;
    linux)
      build_linux_arch x86_64
      build_linux_arch aarch64
      ;;
    linux-x86_64)
      build_linux_arch x86_64
      ;;
    linux-aarch64)
      build_linux_arch aarch64
      ;;
    macos)
      build_macos_arch x86_64
      build_macos_arch aarch64
      ;;
    macos-x86_64)
      build_macos_arch x86_64
      ;;
    macos-aarch64)
      build_macos_arch aarch64
      ;;
    *)
      echo "Unsupported platform: $platform" >&2
      usage >&2
      exit 1
      ;;
  esac
}

parse_args "$@"
resolve_dirs
ensure_command cmake
ensure_command cargo
run_platform "$PLATFORM"
