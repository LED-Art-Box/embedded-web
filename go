#!/usr/bin/env bash

set -eu -o pipefail

task_init() {
  pipenv install --dev
}

task_build() {
  pipenv run pio run
}

task_flash() {
  pipenv run pio run -t upload
}

task_help() {
  cat <<EOF
Usage $0 COMMAND

Commands are:

  build                 Build the embedded firmware
  flash                 Flash firmware onto ESP32
  init                  Create virtualenv

EOF
}

CMD=${1:-}
shift || true

case "${CMD}" in
  build) task_build "$@" ;;
  flash) task_flash "$@" ;;
  init) task_init "$@" ;;
  *) task_help ;;
esac

