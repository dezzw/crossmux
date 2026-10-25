#!/usr/bin/env bash
# Capture simulator PNGs for docs/screenshots/pr-ui-languages/
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
{"language":"${LANG_CODE}","contentProfile":${PROFILE},"onboardingVersion":1}
EOF

PREFIX="${VARIANT}-${LANG_CODE}"

run_one() {
  local script="$1"
  local shots="$2"
  CROSSPOINT_SIM_SD="${SIM_SD}" \
  CROSSPOINT_SIM_INPUT_SCRIPT="${script}" \
  CROSSPOINT_SIM_SCREENSHOTS="${shots}" \
    xvfb-run -a "${SIM_BIN}" >/dev/null
}

# Home (Recent tab)
run_one '3000:QUIT' "2500:${OUT_DIR}/${PREFIX}-home.bmp"

# Settings category list
run_one \
  '2000:BOOT;2400:RIGHT;2700:RIGHT;3000:RIGHT;3300:FUNCTION;4500:QUIT' \
  "4000:${OUT_DIR}/${PREFIX}-settings.bmp"

# Language picker (System → Language; 16 rows after System expand)
run_one \
  '2000:BOOT;2400:RIGHT;2700:RIGHT;3000:RIGHT;3300:FUNCTION;3700:DOWN;4000:DOWN;4300:DOWN;4600:FUNCTION;4900:DOWN;5200:DOWN;5500:DOWN;5800:DOWN;6100:DOWN;6400:DOWN;6700:DOWN;7000:DOWN;7300:DOWN;7600:DOWN;7900:DOWN;8200:DOWN;8500:DOWN;8800:DOWN;9100:DOWN;9400:DOWN;9700:FUNCTION;11000:QUIT' \
  "10500:${OUT_DIR}/${PREFIX}-language.bmp"

# Reader (Library → sample.epub)
run_one \
  '2000:BOOT;2300:RIGHT;2600:FUNCTION;3200:DOWN;3500:DOWN;3800:FUNCTION;7000:QUIT' \
  "6500:${OUT_DIR}/${PREFIX}-reader.bmp"

# Text entry (OPDS → Add Server form; 10 rows to OPDS in System)
run_one \
  '2000:BOOT;2400:RIGHT;2700:RIGHT;3000:RIGHT;3300:FUNCTION;3700:DOWN;4000:DOWN;4300:DOWN;4600:FUNCTION;4900:DOWN;5200:DOWN;5500:DOWN;5800:DOWN;6100:DOWN;6400:DOWN;6700:DOWN;7000:DOWN;7300:DOWN;7600:DOWN;7900:FUNCTION;8300:FUNCTION;10000:QUIT' \
  "9100:${OUT_DIR}/${PREFIX}-keyboard.bmp"

for bmp in "${OUT_DIR}/${PREFIX}"-*.bmp; do
  [[ -f "${bmp}" ]] || continue
  convert "${bmp}" "${bmp%.bmp}.png"
done

echo "Wrote screenshots under ${OUT_DIR}"
