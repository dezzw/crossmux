# Apps (`src/activities/apps/`)

The Apps menu lists lightweight activities beside the reader. Each app is an
`Activity` (or small Activity tree) launched from `AppsMenuActivity` via
`ActivityManager::goTo*`.

## Catalog (PR-1 slim fork)

| App | Path | Entry |
|-----|------|-------|
| File transfer | `network/CrossPointWebServerActivity` | `goToFileTransfer` |
| OPDS browser | `browser/OpdsBookBrowserActivity` | `goToBrowser` |
| WeRead (China profile) | `weread/` | `goToWeRead` |
| Reading stats | `reading-stats/` | `goToReadingStatsMenu` |
| Standby faces | `standby/` | `goToStandby` |

Register new apps in `AppsMenuActivity.cpp` (`kAppEntries`), add a stable
`appVisibility::AppId` bit in `AppVisibility.h` (never renumber existing IDs),
and wire `ActivityManager::goTo*`.

## Shared helpers

- **`GameUi.{h,cpp}`** — touch/grid geometry shared with reading-stats layouts
  (not limited to games despite the name).
- **`AppVisibility.h`** — persisted hide/show bits for the apps catalog.

## Conventions

- User-facing strings use `tr()` / `StrId`.
- Back from an app returns via `activityManager.goToApps()`.
- Free heap allocations in `onExit()` that were taken in `onEnter()`.

See [reading-stats/README.md](./reading-stats/README.md) and
[weread/README.md](./weread/README.md) for subsystem docs.
