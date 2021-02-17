#!/usr/bin/env bash

set -eu -o pipefail

task_init() {
  pipenv install --dev
}

task_build() {
  pipenv run pio run
}

task_capture() {
  pipenv run python scripts/capture.py "$@"
}

task_flash() {
  pipenv run pio run -t upload
}

task_help() {
  cat <<EOF
Usage $0 COMMAND

Commands are:

  build                 Build the embedded firmware
  capture image-file    Store current image to file
  flash                 Flash firmware onto ESP32
  init                  Create virtualenv

EOF
}

CMD=${1:-}
shift || true

case "${CMD}" in
  build) task_build "$@" ;;
  capture) task_capture "$@" ;;
  flash) task_flash "$@" ;;
  init) task_init "$@" ;;
  *) task_help ;;
esac

