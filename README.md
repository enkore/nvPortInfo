# nvPortInfo

A simple command line utility for Windows to dump some low-level information
about connected displays. This includes stuff like DisplayPort speeds,
color spaces, chroma subsampling etc.

Example output:

```
╔══ Display path #0 ════════════════════════════════════════
║ Attached displays: 2
║ Display source format: 2560 x 1440 x 32 bpp
║         source color: 32-bit (RGB888)
║
╟─┐ DisplayID: 0x80061087
║ ├─ Refresh rate: 120.00 Hz
║ ├─ Scaling mode: Preserve aspect ratio (closest resolution)
║ │  (scaling only happens if source and display resolution differ)
║ │
║ ├─┐ Timing (Horz x Vert): progressive scan
║ │ ├─ Name: CTA-861Long: 2560x1440x119.998Hz/P
║ │ ├─ Pixel clock: 497.750 MHz
║ │ ├─ Total:       2720 x 1525
║ │ ├─ Visible:     2560 x 1440
║ │ ├─ Blank:          0 x    0
║ │ ├─ Front porch:   48 x    3
║ │ ├─ Sync width:    32 x    5
║ │ └─ Sync polarity:  +      -
║ │
║ ├─ Connection: DisplayPort
║ ├─┐ DisplayPort info:
║ │ ├─ Current link: DP 5.4 Gbps (HBR2) x 4 lanes
║ │ ├─ Maximum link: DP 8.1 Gbps (HBR3) x 4 lanes
║ │ ├─ Data rate: Calculated 14.93 Gbps / 17.28 Gbps available (86.4%)
║ │ └─ Current color format: RGB (10 bpc, color space sRGB)
║ │
║ ├─┐ HDR capabilities:
║ │ ├─ HDMI 2.0a ST.2084 EOTF: 1
║ │ ├─ HDMI 2.0a traditional HDR gamma: 0
║ │ ├─ EDR on SDR: 0
║ │ ├─ SDR gamma: 1
║ │ ├─ Dolby Vision: 0
║ │ ├─ HDR10+: 0
║ │ ├─ HDR10+ Gaming: 0
║ │ └─ Luminance range: 0.00 - 465 nits
║ └─ HDR status: Off
║
╟─┐ DisplayID: 0x80061081
║ ├─ Refresh rate: 144.00 Hz
║ ├─ Scaling mode: Preserve aspect ratio (closest resolution)
║ │  (scaling only happens if source and display resolution differ)
║ │
║ ├─┐ Timing (Horz x Vert): progressive scan
║ │ ├─ Name: EDID-Detailed:2560x1440x144.000Hz
║ │ ├─ Pixel clock: 568.720 MHz
║ │ ├─ Total:       2640 x 1496
║ │ ├─ Visible:     2560 x 1440
║ │ ├─ Blank:          0 x    0
║ │ ├─ Front porch:    8 x   16
║ │ ├─ Sync width:    48 x    8
║ │ └─ Sync polarity:  +      -
║ │
║ ├─ Connection: DisplayPort
║ ├─┐ DisplayPort info:
║ │ ├─ Current link: DP 5.4 Gbps (HBR2) x 4 lanes
║ │ ├─ Maximum link: DP 5.4 Gbps (HBR2) x 4 lanes
║ │ ├─ Data rate: Calculated 17.06 Gbps / 17.28 Gbps available (98.7%)
║ │ └─ Current color format: RGB (10 bpc, color space sRGB)
║ │
║ ├─┐ HDR capabilities:
║ │ ├─ HDMI 2.0a ST.2084 EOTF: 1
║ │ ├─ HDMI 2.0a traditional HDR gamma: 0
║ │ ├─ EDR on SDR: 0
║ │ ├─ SDR gamma: 1
║ │ ├─ Dolby Vision: 0
║ │ ├─ HDR10+: 0
║ │ ├─ HDR10+ Gaming: 0
║ │ └─ Luminance range: 0.10 - 400 nits
║ └─ HDR status: Off
╚═══════════════════════════════════════════════════════════

╔══ Display path #1 ════════════════════════════════════════
║ Attached displays: 1
║ Display source format: 1600 x 1200 x 32 bpp
║         source color: 32-bit (RGB888)
║
╟─┐ DisplayID: 0x80061082
║ ├─ NT display: \\.\DISPLAY2
║ ├─ Monitor name: LCD2190UXp
║ ├─ Refresh rate: 60.00 Hz
║ ├─ Scaling mode: Preserve aspect ratio (closest resolution)
║ │  (scaling only happens if source and display resolution differ)
║ │
║ ├─┐ Timing (Horz x Vert): progressive scan
║ │ ├─ Name: EDID-Detailed:1600x1200x60.000Hz
║ │ ├─ Pixel clock: 162.000 MHz
║ │ ├─ Total:       2160 x 1250
║ │ ├─ Visible:     1600 x 1200
║ │ ├─ Blank:          0 x    0
║ │ ├─ Front porch:   64 x    1
║ │ ├─ Sync width:   192 x    3
║ │ └─ Sync polarity:  +      +
║ │
║ ├─ Connection: HDMI
║ ├─ No HDR capabilities
║ └─ HDR status: Off
╚═══════════════════════════════════════════════════════════
```