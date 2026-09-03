#!/usr/bin/env bash
# Build script for devacademyl3e2/nrf54l15/cpuapp/ns

set -euo pipefail

app_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="${app_dir}/build"
board="devacademyl3e2/nrf54l15/cpuapp/ns"

if [[ "${1:-}" == "-p" ]]; then
	pristine_build=true
elif [[ ! -f "${build_dir}/CMakeCache.txt" ]]; then
	pristine_build=true
else
	pristine_build=false
fi

if [[ "${pristine_build}" == true ]]; then
	echo "Performing pristine build for ${board}..."
	west build -p -b "${board}" -d "${build_dir}" "${app_dir}" -- \
		-DBOARD_ROOT="${app_dir}" \
		-DCMAKE_POLICY_DEFAULT_CMP0177=NEW
else
	echo "Performing incremental build..."
	west build -d "${build_dir}"
fi



