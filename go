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

task_clock() {
  pipenv run python scripts/clock.py --full-image "$@"
}

task_upload() {
  pipenv run python scripts/upload-image.py "$@"
}

task_flash() {
  pipenv run pio run -t upload
}

task_lint() {
  pipenv run flake8 ./animations/ ./scripts/ --count --show-source --ignore=E501
}

task_yellow_screamy() {
  pipenv run python animations/yellow-screamy.py "$@"
}

task_help() {
  cat <<EOF
Usage $0 COMMAND

Commands are:

  build                 Build the embedded firmware
  capture image-file    Store current image to file
  clock                 Current time as analog clock
  upload image-file     Upload image file
  flash                 Flash firmware onto ESP32
  init                  Create virtualenv
  lint                  Lint Python scripts
  yellow-screamy        Run yellow screamy animation

EOF
}

CMD=${1:-}
shift || true

case "${CMD}" in
  build) task_build "$@" ;;
  capture) task_capture "$@" ;;
  clock) task_clock "$@" ;;
  upload) task_upload "$@" ;;
  flash) task_flash "$@" ;;
  init) task_init "$@" ;;
  lint) task_lint "$@" ;;
  yellow-screamy) task_yellow_screamy "$@" ;;
  *) task_help ;;
esac

