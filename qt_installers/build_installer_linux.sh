#!/usr/bin/env bash
#
# Build the QtJsonViewer offline installer with the Qt Installer Framework.
#
# Steps:
#   1. Configure + build the app (src/) in Release mode.
#   2. Install it, including the Qt runtime (qt_generate_deploy_app_script),
#      into the component's data/ directory.
#   3. Bundle non-Qt shared libraries that may be missing on target machines (TBB).
#   4. Fill version/date placeholders in config.xml and package.xml.
#   5. Run binarycreator --offline-only.
#
# Usage:
#   ./build_installer.sh [--version X.Y.Z] [--clean]
#
# Environment overrides:
#   QT_DIR      Qt kit prefix         (default: /home/programs/DEV/QtSDK/6.12.1/gcc_64)
#   IFW_DIR     Installer Framework   (default: /home/programs/DEV/QtSDK/Tools/QtInstallerFramework/4.11)
#   APP_VERSION Installer version     (default: 1.0.0, or --version)
#   JOBS        Parallel build jobs   (default: nproc)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
SRC_DIR="${REPO_DIR}/src"

QT_DIR="${QT_DIR:-/home/programs/DEV/QtSDK/6.12.1/gcc_64}"
IFW_DIR="${IFW_DIR:-/home/programs/DEV/QtSDK/Tools/QtInstallerFramework/4.11}"
APP_VERSION="${APP_VERSION:-1.0.0}"
JOBS="${JOBS:-$(nproc)}"
CLEAN=0

COMPONENT="com.rbt.qtjsonviewer"
APP_NAME="QtJsonViewer"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --version) APP_VERSION="$2"; shift 2 ;;
        --clean)   CLEAN=1; shift ;;
        -h|--help) sed -n '2,22p' "$0"; exit 0 ;;
        *) echo "Unknown argument: $1" >&2; exit 1 ;;
    esac
done

WORK_DIR="${SCRIPT_DIR}/_work"
BUILD_DIR="${WORK_DIR}/build"
STAGE_DIR="${WORK_DIR}/stage"
DATA_DIR="${STAGE_DIR}/packages/${COMPONENT}/data"
OUTPUT_DIR="${SCRIPT_DIR}/output"
INSTALLER="${OUTPUT_DIR}/${APP_NAME}-${APP_VERSION}-linux-x64-installer.run"

BINARYCREATOR="${IFW_DIR}/bin/binarycreator"
INSTALLERBASE="${IFW_DIR}/bin/installerbase"

log() { printf '\n\033[1;34m==> %s\033[0m\n' "$*"; }

# --- sanity checks -----------------------------------------------------------
[[ -x "${BINARYCREATOR}" ]] || { echo "binarycreator not found: ${BINARYCREATOR}" >&2; exit 1; }
[[ -x "${INSTALLERBASE}" ]] || { echo "installerbase not found: ${INSTALLERBASE}" >&2; exit 1; }
[[ -d "${QT_DIR}/lib/cmake/Qt6" ]] || { echo "Qt6 kit not found: ${QT_DIR}" >&2; exit 1; }
[[ "${APP_VERSION}" =~ ^[0-9]+(\.[0-9]+)*(-[A-Za-z0-9.]+)?$ ]] || { echo "Invalid version: ${APP_VERSION}" >&2; exit 1; }

if [[ ${CLEAN} -eq 1 ]]; then
    log "Cleaning ${WORK_DIR}"
    rm -rf "${WORK_DIR}"
fi

GENERATOR=()
command -v ninja >/dev/null 2>&1 && GENERATOR=(-G Ninja)

# --- 1. build ----------------------------------------------------------------
log "Configuring ${APP_NAME} (Qt: ${QT_DIR})"
cmake -S "${SRC_DIR}" -B "${BUILD_DIR}" "${GENERATOR[@]}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH="${QT_DIR}"

log "Building"
cmake --build "${BUILD_DIR}" --parallel "${JOBS}"

# --- 2. stage meta + install app with the Qt runtime --------------------------
log "Staging installer tree in ${STAGE_DIR}"
rm -rf "${STAGE_DIR}"
mkdir -p "${STAGE_DIR}"
cp -r "${SCRIPT_DIR}/config"   "${STAGE_DIR}/config"
cp -r "${SCRIPT_DIR}/packages" "${STAGE_DIR}/packages"
mkdir -p "${DATA_DIR}"
rm -f "${DATA_DIR}/.gitkeep"

cmake --install "${BUILD_DIR}" --prefix "${DATA_DIR}" --strip

EXE="${DATA_DIR}/bin/${APP_NAME}"
[[ -x "${EXE}" ]] || { echo "Installed executable not found: ${EXE}" >&2; exit 1; }

# --- 3. bundle non-Qt runtime libraries --------------------------------------
# Qt's deploy script copies the Qt libs/plugins. TBB is a system library that
# end users usually don't have, so ship it too (binary RPATH is $ORIGIN/../lib).
log "Bundling extra runtime libraries"
mkdir -p "${DATA_DIR}/lib"
while read -r lib; do
    [[ -n "${lib}" && -f "${lib}" ]] || continue
    echo "  + $(basename "${lib}")"
    cp -L "${lib}" "${DATA_DIR}/lib/"
done < <(LD_LIBRARY_PATH="${DATA_DIR}/lib" ldd "${EXE}" | awk '/libtbb/ { print $3 }')

# Icon used by the desktop entry created in installscript.qs
mkdir -p "${DATA_DIR}/share/icons"
cp "${SRC_DIR}/resources/${APP_NAME}.png" "${DATA_DIR}/share/icons/${APP_NAME}.png"

# Verify that every dependency resolves inside the bundle or on the base system.
if LD_LIBRARY_PATH="" ldd "${EXE}" | grep -q "not found"; then
    echo "Unresolved libraries:" >&2
    ldd "${EXE}" | grep "not found" >&2
    exit 1
fi

# --- 4. version placeholders --------------------------------------------------
RELEASE_DATE="$(date +%Y-%m-%d)"
sed -i "s/__APP_VERSION__/${APP_VERSION}/g" "${STAGE_DIR}/config/config.xml"
sed -i -e "s/__APP_VERSION__/${APP_VERSION}/g" -e "s/__RELEASE_DATE__/${RELEASE_DATE}/g" \
    "${STAGE_DIR}/packages/${COMPONENT}/meta/package.xml"

# --- 5. create the offline installer ----------------------------------------
log "Creating offline installer"
mkdir -p "${OUTPUT_DIR}"
rm -f "${INSTALLER}"
"${BINARYCREATOR}" --offline-only \
    -t "${INSTALLERBASE}" \
    -c "${STAGE_DIR}/config/config.xml" \
    -p "${STAGE_DIR}/packages" \
    "${INSTALLER}"

log "Done: ${INSTALLER} ($(du -h "${INSTALLER}" | cut -f1))"
