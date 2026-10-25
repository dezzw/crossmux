# PR hyphenation (English-only trie) visual regression

Desktop simulator (`pio run -e simulator`) captures using `CROSSPOINT_SIM_SCREENSHOTS`
and `CROSSPOINT_SIM_INPUT_SCRIPT` (Waveshare face-button chords: `5000:FUNCTION;6000:FUNCTION`
opens the sole recent-book fixture). English UI uses settings language `EN`; Chinese UI uses `ZH_CN`.

| UI language | Screen | Before (main, all tries) | After (English-only registry) |
|-------------|--------|--------------------------|-------------------------------|
| English | Home | `before-en-ui/before-en-ui-home.png` | `after-en-ui/after-en-ui-home.png` |
| English | Settings | `before-en-ui/before-en-ui-settings.png` | `after-en-ui/after-en-ui-settings.png` |
| English | Reader (en EPUB) | `before-en-ui/before-en-ui-reader-en.png` | `after-en-ui/after-en-ui-reader-en.png` |
| English | Reader (zh EPUB) | `before-en-ui/before-en-ui-reader-zh.png` | `after-en-ui/after-en-ui-reader-zh.png` |
| English | Reader (de EPUB) | `before-en-ui/before-en-ui-reader-de.png` | `after-en-ui/after-en-ui-reader-de.png` |
| Chinese UI | Home | `before-zh-ui/before-zh-ui-home.png` | `after-zh-ui/after-zh-ui-home.png` |
| Chinese UI | Settings | `before-zh-ui/before-zh-ui-settings.png` | `after-zh-ui/after-zh-ui-settings.png` |
| Chinese UI | Reader (en/zh/de) | `before-zh-ui/before-zh-ui-reader-*.png` | `after-zh-ui/after-zh-ui-reader-*.png` |

English and Chinese reader/home/settings pairs match before vs after (no layout regression).

The German reader fixture uses a full-width paragraph: at this width it does not show visible
inserted hyphens even when German patterns are linked, so before/after German reader PNGs are
byte-identical. German hyphenation removal is covered by unit tests, not this screenshot pair.
