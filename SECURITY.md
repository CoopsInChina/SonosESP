# Security Policy

## Supported Versions

Only the latest release is supported.

## Reporting a Vulnerability

**Please do not report security vulnerabilities through public GitHub issues.**

If you discover a security vulnerability, create a **private security advisory** on GitHub:

1. Go to the **Security** tab
2. Click **Report a vulnerability**
3. Fill in the details

There are no guaranteed response times. This is a personal project maintained on a best-effort basis.

## Security Considerations for Deployment

### Network Security

**WiFi Credentials:**
- WiFi credentials are stored in NVS (Non-Volatile Storage)
- Credentials are NOT encrypted in NVS
- ⚠️ **Do not share firmware dumps** as they contain your WiFi password
- ⚠️ **Factory reset** before disposing of hardware

**Network Traffic:**
- Album art from public CDNs uses HTTP (not HTTPS) for performance
- Sonos SOAP API uses HTTP over local network (Sonos limitation)
- Lyrics API uses HTTPS with certificate validation disabled (performance trade-off)
- OTA updates use HTTPS with certificate validation disabled

**Recommendations:**
- Use a secure, private WiFi network
- Do not expose the device to untrusted networks
- Consider network segmentation (IoT VLAN)

### Physical Security

**Flash Memory:**
- Firmware contains WiFi credentials in plaintext
- Enable flash encryption in production deployments (performance impact)

**Serial Access:**
- Serial port provides full system access
- Physical access = full compromise

### OTA Updates

**Update Security:**
- OTA downloads use HTTPS but skip certificate validation
- Updates are **not cryptographically signed**
- Only update from official GitHub releases
- Use a trusted network for updates

### Known Limitations

- No authentication on device (physical access = control)
- No encryption of stored data (NVS)
- Sonos API does not use authentication (local network trust model)
