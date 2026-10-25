#!/usr/bin/env bash
# Single AZERTY keyboard screenshot for pr-ui-languages docs.
set -euo pipefail

REPO_ROOT="${CROSSMUX_REPO_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)}"
cd "${REPO_ROOT}"

VARIANT="${1:?usage: $0 <before|after> <EN|ZH_CN>}"
LANG_CODE="${2:?usage: $0 <before|after> <EN|ZH_CN>}"
OUT_DIR="${REPO_ROOT}/docs/screenshots/pr-ui-languages/${VARIANT}/${LANG_CODE}"
SIM_BIN="${REPO_ROOT}/.pio/build/simulator/program"

if [[ ! -x "${SIM_BIN}" ]]; then
  echo "Build simulator first: pio run -e simulator" >&2
  exit 1
fi

SIM_SD="$(mktemp -d)"
trap 'rm -rf "${SIM_SD}"' EXIT
mkdir -p "${OUT_DIR}" "${SIM_SD}/Books" "${SIM_SD}/.crosspoint"
cp "${REPO_ROOT}/test/epubs/test_br_section_break.epub" "${SIM_SD}/Books/sample.epub"

if [[ "${LANG_CODE}" == "ZH_CN" ]]; then PROFILE=1; else PROFILE=0; fi
cat >"${SIM_SD}/.crosspoint/settings.json" <<EOF
{"language":"${LANG_CODE}","contentProfile":${PROFILE},"onboardingVersion":1,"keyboardLayouts":2}
EOF

PREFIX="${VARIANT}-${LANG_CODE}"
BMP="${OUT_DIR}/${PREFIX}-keyboard.bmp"

CROSSPOINT_SIM_SD="${SIM_SD}" \
CROSSPOINT_SIM_INPUT_SCRIPT='2000:BOOT;2400:RIGHT;2700:RIGHT;3000:RIGHT;3300:FUNCTION;3700:DOWN;4000:DOWN;4300:DOWN;4600:FUNCTION;4900:DOWN;5200:DOWN;5500:DOWN;5800:DOWN;6100:DOWN;6400:DOWN;6700:DOWN;7000:DOWN;7300:DOWN;7600:DOWN;7900:FUNCTION;8300:ENTER;8800:ENTER;12000:QUIT' \
CROSSPOINT_SIM_SCREENSHOTS="9500:${BMP}" \
  xvfb-run -a "${SIM_BIN}" >/dev/null

convert "${BMP}" "${BMP%.bmp}.png"
rm -f "${BMP}"
echo "Wrote ${OUT_DIR}/${PREFIX}-keyboard.png"
