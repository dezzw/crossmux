# PR hyphenation (English-only trie) visual regression

Captured with `scripts/capture_pr_hyphenation_screenshots.py` against the desktop
simulator (`pio run -e simulator`). Each run uses a fresh SD tree with three
fixture EPUBs (English long words, Chinese sample, German compounds).

| UI language | Screen | Before (main, all tries) | After (en-only trie) |
|-------------|--------|--------------------------|----------------------|
| English | Home | `before-en-ui/before-en-ui-home.png` | `after-en-ui/after-en-ui-home.png` |
| English | Settings | `before-en-ui/before-en-ui-settings.png` | `after-en-ui/after-en-ui-settings.png` |
| English | Reader (en EPUB) | `before-en-ui/before-en-ui-reader-en.png` | `after-en-ui/after-en-ui-reader-en.png` |
| English | Reader (zh EPUB) | `before-en-ui/before-en-ui-reader-zh.png` | `after-en-ui/after-en-ui-reader-zh.png` |
| English | Reader (de EPUB) | `before-en-ui/before-en-ui-reader-de.png` | `after-en-ui/after-en-ui-reader-de.png` |
| Chinese UI | Home | `before-zh-ui/before-zh-ui-home.png` | `after-zh-ui/after-zh-ui-home.png` |
| Chinese UI | Settings | `before-zh-ui/before-zh-ui-settings.png` | `after-zh-ui/after-zh-ui-settings.png` |
| Chinese UI | Reader (en EPUB) | `before-zh-ui/before-zh-ui-reader-en.png` | `after-zh-ui/after-zh-ui-reader-en.png` |
| Chinese UI | Reader (zh EPUB) | `before-zh-ui/before-zh-ui-reader-zh.png` | `after-zh-ui/after-zh-ui-reader-zh.png` |
| Chinese UI | Reader (de EPUB) | `before-zh-ui/before-zh-ui-reader-de.png` | `after-zh-ui/after-zh-ui-reader-de.png` |

Expected visual delta: German reader pages lose inserted hyphens but keep
clean wraps; English hyphenation and Chinese layout should match the before
captures.
