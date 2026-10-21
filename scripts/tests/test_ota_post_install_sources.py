"""Source guards for OTA post-install confirm, font unload, and glyph metadata validation."""

from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[2]

CONFIRM_CALL = "otaConfirmPendingRunningImage();"
MAIN_GUARD = "if (fontsReady && !recoveryFirmwareMode)"
PRELOAD_GUARD = "if (!allowAutoPreload_) return"
PRELOAD_CALL = "runPostOtaFontPreloadWithProgress"


def strip_comments(source: str) -> str:
    return re.sub(r"//[^\n]*|/\*.*?\*/", "", source, flags=re.S)


def strip_if_false_blocks(source: str) -> str:
    text = source
    pattern = re.compile(r"if\s*\(\s*false\s*\)\s*\{", re.S)
    while True:
        match = pattern.search(text)
        if not match:
            return text
        brace = text.find("{", match.start())
        depth = 0
        end = None
        for i in range(brace, len(text)):
            if text[i] == "{":
                depth += 1
            elif text[i] == "}":
                depth -= 1
                if depth == 0:
                    end = i + 1
                    break
        if end is None:
            return text
        text = text[: match.start()] + text[end:]


def effective_source(source: str) -> str:
    return strip_if_false_blocks(strip_comments(source))


def extract_braced_block(source: str, open_brace_index: int) -> str:
    depth = 0
    for i in range(open_brace_index, len(source)):
        if source[i] == "{":
            depth += 1
        elif source[i] == "}":
            depth -= 1
            if depth == 0:
                return source[open_brace_index + 1 : i]
    return ""


def function_body(source: str, signature: str) -> str:
    start = source.index(signature)
    brace = source.index("{", start)
    return extract_braced_block(source, brace)


def call_is_effective(source: str, call: str) -> bool:
    return call in effective_source(source)


def main_confirm_in_recovery_guard(source: str) -> bool:
    stripped = strip_comments(source)
    guard = stripped.find(MAIN_GUARD)
    if guard < 0:
        return False
    brace = stripped.find("{", guard)
    if brace < 0:
        return False
    body = extract_braced_block(stripped, brace)
    return call_is_effective(body, CONFIRM_CALL)


def boot_post_ota_preload_guarded(source: str) -> bool:
    body = function_body(strip_comments(source), "void BootActivity::runPostOta(")
    effective = strip_if_false_blocks(body)
    preload = effective.find(PRELOAD_CALL)
    guard = effective.find(PRELOAD_GUARD)
    return guard >= 0 and preload >= 0 and guard < preload


class OtaPostInstallSourceGuards(unittest.TestCase):
    def test_go_home_confirms_running_image(self):
        source = (ROOT / "src/activities/ActivityManager.cpp").read_text()
        body = function_body(source, "void ActivityManager::goHome(")
        self.assertTrue(call_is_effective(body, CONFIRM_CALL))

    def test_main_confirms_when_not_in_recovery(self):
        source = (ROOT / "src/main.cpp").read_text()
        self.assertTrue(main_confirm_in_recovery_guard(source))

    def test_ota_update_unloads_fonts_before_inactive_slot_write(self):
        source = (ROOT / "src/activities/settings/OtaUpdateActivity.cpp").read_text()
        install = effective_source(source).index("updater.installUpdate(")
        prefix = effective_source(source)[:install]
        self.assertIn("sdFontSystem.releaseLoadedFont(renderer);", prefix)

    def test_read_glyph_metadata_validates_payload_size(self):
        source = (ROOT / "lib/EpdFont/SdCardFont.cpp").read_text()
        body = function_body(source, "SdCardFont::readGlyphMetadata(")
        self.assertIn("glyphBitmapPayloadValid(glyph", strip_comments(body))

    def test_boot_post_ota_respects_allow_auto_preload(self):
        source = (ROOT / "src/activities/boot_sleep/BootActivity.cpp").read_text()
        self.assertTrue(boot_post_ota_preload_guarded(source))


class OtaPostInstallSourceGuardNegatives(unittest.TestCase):
    def test_rejects_commented_confirm_call(self):
        snippet = f"void go() {{ // {CONFIRM_CALL}\n}}"
        self.assertFalse(call_is_effective(snippet, CONFIRM_CALL))

    def test_rejects_confirm_inside_if_false(self):
        snippet = f"void go() {{ if (false) {{ {CONFIRM_CALL} }} }}"
        self.assertFalse(call_is_effective(snippet, CONFIRM_CALL))

    def test_rejects_main_confirm_outside_recovery_guard(self):
        snippet = f"{CONFIRM_CALL}\nif (fontsReady && !recoveryFirmwareMode) {{ }}\n"
        self.assertFalse(main_confirm_in_recovery_guard(snippet))
        snippet2 = f"if (fontsReady && !recoveryFirmwareMode) {{ }}\n{CONFIRM_CALL}\n"
        self.assertFalse(main_confirm_in_recovery_guard(snippet2))

    def test_rejects_missing_allow_auto_preload_guard(self):
        snippet = "void BootActivity::runPostOta() { runPostOtaFontPreloadWithProgress(); }"
        self.assertFalse(boot_post_ota_preload_guarded(snippet))

    def test_rejects_allow_auto_preload_guard_after_preload(self):
        snippet = (
            "void BootActivity::runPostOta() { runPostOtaFontPreloadWithProgress(); "
            "if (!allowAutoPreload_) return; }"
        )
        self.assertFalse(boot_post_ota_preload_guarded(snippet))


if __name__ == "__main__":
    unittest.main()
