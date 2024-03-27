#!/bin/bash
DPU_SDK_BIN_DIR="$(dirname "$0")"
"${DPU_SDK_BIN_DIR}/dpu-profiling" memory-transfer "$@"
