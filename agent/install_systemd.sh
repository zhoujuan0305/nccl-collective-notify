#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

SERVER_HOST="192.168.3.3"
SERVER_PORT="19090"
BUILD_DIR="${REPO_ROOT}/build-systemd-agent"
INSTALL_BIN="/usr/local/bin/nccl-agent"
SERVICE_NAME="nccl-agent"
SERVICE_FILE="/etc/systemd/system/${SERVICE_NAME}.service"
SERVICE_USER="root"
SERVICE_GROUP="root"

usage() {
  cat <<EOF
Usage:
  sudo $(basename "$0") [options]

Options:
  --server-host HOST   Remote UDP server host, default: ${SERVER_HOST}
  --server-port PORT   Remote UDP server port, default: ${SERVER_PORT}
  --build-dir DIR      Temporary build directory, default: ${BUILD_DIR}
  --install-bin PATH   Installed binary path, default: ${INSTALL_BIN}
  --service-name NAME  systemd service name, default: ${SERVICE_NAME}
  --service-user USER  systemd service user, default: ${SERVICE_USER}
  --service-group GRP  systemd service group, default: ${SERVICE_GROUP}
  -h, --help           Show this help

Example:
  sudo $(basename "$0") --server-host 10.0.0.8 --server-port 19090 --service-user zhiyuanzhou --service-group zhiyuanzhou
EOF
}

require_root() {
  if [[ "${EUID}" -ne 0 ]]; then
    echo "this script must be run as root, for example: sudo $0 $*" >&2
    exit 1
  fi
}

parse_args() {
  while [[ $# -gt 0 ]]; do
    case "$1" in
      --server-host)
        SERVER_HOST="$2"
        shift 2
        ;;
      --server-port)
        SERVER_PORT="$2"
        shift 2
        ;;
      --build-dir)
        BUILD_DIR="$2"
        shift 2
        ;;
      --install-bin)
        INSTALL_BIN="$2"
        shift 2
        ;;
      --service-name)
        SERVICE_NAME="$2"
        SERVICE_FILE="/etc/systemd/system/${SERVICE_NAME}.service"
        shift 2
        ;;
      --service-user)
        SERVICE_USER="$2"
        shift 2
        ;;
      --service-group)
        SERVICE_GROUP="$2"
        shift 2
        ;;
      -h|--help)
        usage
        exit 0
        ;;
      *)
        echo "unknown argument: $1" >&2
        usage
        exit 1
        ;;
    esac
  done
}

build_agent() {
  echo "building nccl-agent for ${SERVER_HOST}:${SERVER_PORT}"
  make -C "${REPO_ROOT}/agent" build \
    BUILDDIR="${BUILD_DIR}" \
    AGENT_REMOTE_HOST="${SERVER_HOST}" \
    AGENT_REMOTE_PORT="${SERVER_PORT}"
}

install_binary() {
  echo "installing binary to ${INSTALL_BIN}"
  install -D -m 0755 "${BUILD_DIR}/bin/nccl-agent" "${INSTALL_BIN}"
}

install_service() {
  local escaped_bin escaped_user escaped_group
  escaped_bin="$(printf '%s\n' "${INSTALL_BIN}" | sed 's/[\/&]/\\&/g')"
  escaped_user="$(printf '%s\n' "${SERVICE_USER}" | sed 's/[\/&]/\\&/g')"
  escaped_group="$(printf '%s\n' "${SERVICE_GROUP}" | sed 's/[\/&]/\\&/g')"
  echo "installing systemd unit to ${SERVICE_FILE}"
  sed -e "s/__AGENT_BIN__/${escaped_bin}/g" \
      -e "s/__SERVICE_USER__/${escaped_user}/g" \
      -e "s/__SERVICE_GROUP__/${escaped_group}/g" \
    "${SCRIPT_DIR}/nccl-agent.service.template" > "${SERVICE_FILE}"
}

reload_and_enable() {
  echo "reloading systemd daemon"
  systemctl daemon-reload
  echo "enabling ${SERVICE_NAME}"
  systemctl enable "${SERVICE_NAME}"
  echo "restarting ${SERVICE_NAME}"
  systemctl restart "${SERVICE_NAME}"
}

show_status() {
  echo
  echo "deployment complete"
  echo "service: ${SERVICE_NAME}"
  echo "binary : ${INSTALL_BIN}"
  echo "target : ${SERVER_HOST}:${SERVER_PORT}"
  echo "user   : ${SERVICE_USER}:${SERVICE_GROUP}"
  echo
  echo "use these commands to inspect it:"
  echo "  systemctl status ${SERVICE_NAME}"
  echo "  journalctl -u ${SERVICE_NAME} -f"
}

main() {
  parse_args "$@"
  require_root "$@"
  build_agent
  install_binary
  install_service
  reload_and_enable
  show_status
}

main "$@"
