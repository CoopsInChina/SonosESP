# Contributing to SonosESP

First off, thank you for considering contributing to SonosESP! It's people like you that make this project better.
The original project can be found at https://github.com/OpenSurface/SonosESP, and contributions can be made there. 
This fork is really a personal project and is intended to be a mix between the the OpenSurface SONOS ESP, with a bigger screen support and the NFC tap function of this awesome project https://www.hackster.io/mark-hank/sonos-spotify-vinyl-emulator-3be63d and https://github.com/hankhank10/vinylemulator


### Reporting Bugs

If the bug is specific to the 7" screen or NFC Tap, you can report them here. I may or may not get round to fixing them, but feel free to raise the issue. 

**Bug Report Template:**
- **Device**: GUITION JC4880P433C (ESP32-P4 + ESP32-C6) or your specific board
- **Firmware Version**: (e.g., v1.2.1)
- **Description**: Clear description of the issue
- **Steps to Reproduce**: Numbered list of steps
- **Expected Behavior**: What you expected to happen
- **Actual Behavior**: What actually happened
- **Serial Logs**: Include relevant serial output (use code blocks)
- **Screenshots**: If applicable

### Suggesting Enhancements

Enhancement suggestions are tracked as GitHub issues. When creating an enhancement suggestion, include:

- **Use Case**: Why is this enhancement useful?
- **Proposed Solution**: How would you implement it?
- **Alternatives**: What other approaches have you considered?
- **Additional Context**: Screenshots, mockups, etc.

### Pull Requests

1. **Fork** the repository
2. **Create a branch** from `main` (e.g., `feature/add-spotify-connect` or `fix/ota-crash`)
3. **Make your changes** following the coding guidelines below
4. **Test thoroughly** on actual hardware (not just compilation)
5. **Commit** with clear, descriptive messages
6. **Push** to your fork
7. **Open a Pull Request** with:
   - Clear title and description
   - Reference any related issues (e.g., "Fixes #123")
   - List of changes made
   - Test results (serial logs, screenshots)

## Development Guidelines

### Hardware Requirements

- **Board**: GUITION JC4880P433C (ESP32-P4 + ESP32-C6 via SDIO)
- **Display**: ST7701 MIPI DSI (480x800 portrait, rendered 800x480 landscape)
- **PSRAM**: 32MB OPI at 200MHz
- **Sonos System**: For testing


## Questions?

Feel free to:
- Open a **Discussion** for general questions
- Open an **Issue** for bug reports or feature requests
- Check existing issues/discussions before posting

## License

By contributing, you agree that your contributions will be licensed under the same license as the project (see LICENSE file).

---

Thank you for your contributions! 🎵
