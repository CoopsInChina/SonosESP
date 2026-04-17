# Release Guide

Releases are handled automatically by GitHub Actions when changes are pushed to the relevant branches.

## Release Channels

- **Stable** — triggered automatically when a clean version tag is pushed to `main` (no RC/beta/nightly suffix). Firmware assets for both screen sizes are attached to the GitHub release.
- **Nightly** — triggered automatically on every push to `dev`. Tagged as `v{VERSION}-nightly.{SHORT_SHA}` and marked as a prerelease.

## Promoting dev to release

When `dev` is stable and ready for public distribution:

```bash
git checkout release
git merge dev
git push origin release
```

The web installer at [CoopsInChina.github.io/SonosESP](https://CoopsInChina.github.io/SonosESP/) will rebuild and deploy automatically.

## Promoting release to stable

Update the firmware version in `include/ui_common.h` to a clean version (e.g. `1.7.0` — no RC/beta suffix), then merge to `main`. The auto-release workflow will build both screen targets and create the GitHub release.

## Firmware assets per release

| File | Use |
|------|-----|
| `firmware-4inch.bin` | OTA update for 4" board |
| `firmware-7inch.bin` | OTA update for 7" board |
| `firmware-4inch-merged.bin` | Full flash via esptool or web installer (4") |
| `firmware-7inch-merged.bin` | Full flash via esptool or web installer (7") |
