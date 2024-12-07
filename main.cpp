#include <cstdarg>
#include <windows.h>
#include <cstdio>
#include <cstring>
#include <conio.h>
#include "nvapi.h"

int main();

bool dump_displays();
bool get_display_paths(NV_DISPLAYCONFIG_PATH_INFO** out_paths, NvU32* out_path_count);

/// given an nv api display id (not the legacy output bit mask),
/// return the NT display name such as \\.\DISPLAY1
char *get_nt_display_name_for_displayid(NvU32 displayId);

/// convert something like \\.\DISPLAY2 into an NT device path such as
/// \\?\DISPLAY#GBT2729#5&2004477b&0&UID155908#{e6f07b5f-ee97-4a90-b076-33f57bf4eaa7}
char *get_display_device_path_for_nt_display(const char *nt_display);

/// given a display device path like \\?\DISPLAY#barf#stuff#UUID,
/// comb through the wingdi display configurations and return the god-given name of the darned thing.
char *get_human_monitor_name(const char *monitor_device_path);

// note: the format_xxx functions mostly return static strings except when they don't
// for simplicity we'll just leak little bits of memory here and there
const char* format_link_rate(NV_DP_LINK_RATE rate);
const char *format_dp_link(NV_DP_LINK_RATE rate, NV_DP_LANE_COUNT lane_count);
double link_net_rate_gbps(NV_DP_LINK_RATE rate);
const char *format_color_format(NV_DP_COLOR_FORMAT format);
const char *format_color_space(NV_DP_COLORIMETRY color);
int bpc_to_bits(NV_DP_BPC bpc);
const char *format_source_format(NV_FORMAT format);
const char *format_connector_type(NV_GPU_CONNECTOR_TYPE type);
const char* format_hdr_mode(NV_HDR_MODE mode);
const char *format_scaling(NV_SCALING mode);

char *aprintf(const char* format, ...);
char* wchar_to_char(const WCHAR* wstr);

int main() {
    bool succ;
    SetConsoleOutputCP(CP_UTF8);
    printf("nvPortInfo 1.0 - https://github.com/enkore/nvPortInfo\n\n");
    if(NvAPI_Initialize() != NVAPI_OK) {
        printf("NvAPI_Initialize() failed, nvidia driver missing or borked");
        succ = false;
    } else {
        succ = dump_displays();
    }

    printf("Press 'any' key.\n");
    getch();
    return succ ? 0 : 1;
}

bool dump_displays() {
    NV_DISPLAYCONFIG_PATH_INFO *paths = NULL;
    NvU32 path_count = 0;
    if(!get_display_paths(&paths, &path_count)) {
        return false;
    }

    for(NvU32 path_index = 0; path_index < path_count; path_index++) {
        auto &path = paths[path_index];

        auto source = path.sourceModeInfo;
        printf("╔══ Display path #%d ════════════════════════════════════════\n", path.sourceId);
        printf("║ Attached displays: %d\n", path.targetInfoCount);
        printf("║ Display source format: %d x %d x %d bpp\n", source->resolution.width, source->resolution.height, source->resolution.colorDepth);
        printf("║         source color: %s\n", format_source_format(source->colorFormat));
        printf("║\n");

        for(NvU32 target_index = 0; target_index < path.targetInfoCount; target_index++) {
            auto &target = path.targetInfo[target_index];

            printf("╟─┐ DisplayID: 0x%x\n", target.displayId);

            if(path.targetInfoCount == 1) {  // see comment in get_nt_display_name_for_displayid
                char *nt_display = get_nt_display_name_for_displayid(target.displayId);
                printf("║ ├─ NT display: %s\n", nt_display);
                if (nt_display) {
                    char *dev_path = get_display_device_path_for_nt_display(nt_display);
                    char *name = get_human_monitor_name(dev_path);
                    if (name)
                        printf("║ ├─ Monitor name: %s\n", name);
                    free(name);
                }
            }

            auto details = target.details;
            auto timing = &details->timing;
            printf("║ ├─ Refresh rate: %.2f Hz\n", details->refreshRate1K / 1000.0);
            printf("║ ├─ Scaling mode: %s\n", format_scaling(details->scaling));
            printf("║ │  (scaling only happens if source and display resolution differ)\n");
            printf("║ │\n");
            printf("║ ├─┐ Timing (Horz x Vert): %s scan\n", timing->interlaced ? "interlaced" : "progressive");
            printf("║ │ ├─ Name: %s\n", timing->etc.name);
            printf("║ │ ├─ Pixel clock: %.3f MHz\n", double(timing->pclk) * 0.01);
            printf("║ │ ├─ Total:       %4d x %4d\n", timing->HTotal, timing->VTotal);
            printf("║ │ ├─ Visible:     %4d x %4d\n", timing->HVisible, timing->VVisible);
            printf("║ │ ├─ Blank:       %4d x %4d\n", timing->HBorder, timing->VBorder);
            printf("║ │ ├─ Front porch: %4d x %4d\n", timing->HFrontPorch, timing->VFrontPorch);
            printf("║ │ ├─ Sync width:  %4d x %4d\n", timing->HSyncWidth, timing->VSyncWidth);
            printf("║ │ └─ Sync polarity:  %s      %s\n", timing->HSyncPol ? "-" : "+", timing->VSyncPol ? "-" : "+");
            printf("║ │\n");

            printf("║ ├─ Connection: %s\n", format_connector_type(details->connector));

            NV_DISPLAY_PORT_INFO port_info;
            port_info.version = NV_DISPLAY_PORT_INFO_VER;
            if(NvAPI_GetDisplayPortInfo(NULL, target.displayId, &port_info) == NVAPI_OK
                    && port_info.isDp) {
                printf("║ ├─┐ DisplayPort info:\n");
                printf("║ │ ├─ Current link: %s\n", format_dp_link(port_info.curLinkRate, port_info.curLaneCount));
                printf("║ │ ├─ Maximum link: %s\n", format_dp_link(port_info.maxLinkRate, port_info.maxLaneCount));

                // calculate data rate based on pixel clock, bits per component and chroma subsampling
                auto datarate = double(timing->pclk) * 10000.0 * bpc_to_bits(port_info.bpc) * 3;
                datarate /= 1e9; // bit/s -> gigabit/s
                if(port_info.colorFormat == NV_DP_COLOR_FORMAT_YCbCr422) {
                    datarate *= 2.0/3.0;
                }
                auto linkrate = link_net_rate_gbps(port_info.curLinkRate) * port_info.curLaneCount;
                auto link_util = datarate / linkrate;
                printf("║ │ ├─ Data rate: Calculated %.2f Gbps / %.2f Gbps available (%.1f%%)\n", datarate, linkrate, link_util * 100.0);
                if(link_util > 0.998) {
                    printf("║ │ │  Display Stream Compression (DSC) is probably active\n");
                } else if(link_util > 0.99) {
                    printf("║ │ │  (Impressive, very nice)\n");
                }

                printf("║ │ └─ Current color format: %s (%d bpc, color space %s)\n",
                       format_color_format(port_info.colorFormat),
                       bpc_to_bits(port_info.bpc),
                       format_color_space(port_info.colorimetry)
                );
                printf("║ │\n");
            }

            NV_HDR_CAPABILITIES hdr;
            hdr.version = NV_HDR_CAPABILITIES_VER;
            hdr.driverExpandDefaultHdrParameters = 0;
            if(NvAPI_Disp_GetHdrCapabilities(target.displayId, &hdr) == NVAPI_OK) {
                bool any_hdr_supported =
                        hdr.isST2084EotfSupported
                        | hdr.isTraditionalHdrGammaSupported
                        | hdr.isEdrSupported
                        | hdr.isTraditionalSdrGammaSupported
                        | hdr.isDolbyVisionSupported
                        | hdr.isHdr10PlusSupported
                        | hdr.isHdr10PlusGamingSupported;

                if(any_hdr_supported) {
                    printf("║ ├─┐ HDR capabilities:\n");
                    printf("║ │ ├─ HDMI 2.0a ST.2084 EOTF: %d\n", hdr.isST2084EotfSupported);
                    printf("║ │ ├─ HDMI 2.0a traditional HDR gamma: %d\n", hdr.isTraditionalHdrGammaSupported);
                    printf("║ │ ├─ EDR on SDR: %d\n", hdr.isEdrSupported);
                    printf("║ │ ├─ SDR gamma: %d\n", hdr.isTraditionalSdrGammaSupported);
                    printf("║ │ ├─ Dolby Vision: %d\n", hdr.isDolbyVisionSupported);
                    printf("║ │ ├─ HDR10+: %d\n", hdr.isHdr10PlusSupported);
                    printf("║ │ ├─ HDR10+ Gaming: %d\n", hdr.isHdr10PlusGamingSupported);

                    auto dv = hdr.isDolbyVisionSupported;
                    printf("║ │ %s─ Luminance range: %.2f - %d nits\n",
                           dv ? "├" : "└",
                           double(hdr.display_data.desired_content_min_luminance) / 10000.0,
                           hdr.display_data.desired_content_max_luminance);

                    if (dv) {
                        printf("║ │ └─┐ DolbyVision:\n");
                        printf("║ │   ├─ Luminance range: %.2f - %d nits\n",
                               double(hdr.dv_static_metadata.target_min_luminance) / 10000.0,
                               hdr.dv_static_metadata.target_max_luminance);
                        printf("║ │   └─ DCI P3: %d\n", hdr.dv_static_metadata.colorimetry);
                    }
                } else {
                    printf("║ ├─ No HDR capabilities\n");
                }
            }

            NV_HDR_COLOR_DATA hdr_color;
            hdr_color.version = NV_HDR_COLOR_DATA_VER;
            hdr_color.cmd = NV_HDR_CMD_GET;
            hdr_color.hdrMode = NV_HDR_MODE_OFF;
            NvAPI_Disp_HdrColorControl(target.displayId, &hdr_color);
            printf("║ └─ HDR status: %s", format_hdr_mode(hdr_color.hdrMode));
            if(hdr_color.hdrMode != NV_HDR_MODE_OFF) {
                printf(" (%d bpc)", bpc_to_bits((NV_DP_BPC) hdr_color.hdrBpc));
            }
            printf("\n");

            if(target_index == path.targetInfoCount - 1) {
                printf("╚═══════════════════════════════════════════════════════════\n");
            } else {
                printf("║\n");
            }
        }
        printf("\n");
    }
    return true;
}

bool get_display_paths(NV_DISPLAYCONFIG_PATH_INFO **out_paths, NvU32 *out_path_count)
{
    NvU32 path_count = 0;
    NV_DISPLAYCONFIG_PATH_INFO *path_info = NULL;
    if(NvAPI_DISP_GetDisplayConfig(&path_count, NULL) != NVAPI_OK)
        return false;

    path_info = (NV_DISPLAYCONFIG_PATH_INFO*)calloc(path_count, sizeof(NV_DISPLAYCONFIG_PATH_INFO));
    for(NvU32 i = 0; i < path_count; i++) {
        path_info[i].version = NV_DISPLAYCONFIG_PATH_INFO_VER;
    }
    if(NvAPI_DISP_GetDisplayConfig(&path_count, path_info) != NVAPI_OK)
        return false;

    for(NvU32 i = 0; i < path_count; i++) {
        path_info[i].sourceModeInfo = (NV_DISPLAYCONFIG_SOURCE_MODE_INFO *)calloc(1,sizeof(NV_DISPLAYCONFIG_SOURCE_MODE_INFO));
        path_info[i].targetInfo = (NV_DISPLAYCONFIG_PATH_TARGET_INFO*)calloc(path_info[i].targetInfoCount, sizeof(NV_DISPLAYCONFIG_PATH_TARGET_INFO));
        for(NvU32 j = 0 ; j < path_info[i].targetInfoCount ; j++) {
            path_info[i].targetInfo[j].details = (NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO*)calloc(1, sizeof(NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO));
            path_info[i].targetInfo[j].details->version = NV_DISPLAYCONFIG_PATH_ADVANCED_TARGET_INFO_VER;
        }
    }
    if (NvAPI_DISP_GetDisplayConfig(&path_count, path_info) != NVAPI_OK)
        return false;

    *out_paths = path_info;
    *out_path_count = path_count;
    return true;
}

char *get_nt_display_name_for_displayid(NvU32 displayId) {
    NvAPI_Status  ret;
    NvDisplayHandle handle;
    char name[NVAPI_SHORT_STRING_MAX] = {0};  // \\.\DISPLAYx

    for(unsigned n = 0;; n++) {
        ret = NvAPI_EnumNvidiaDisplayHandle(n, &handle);
        if (ret == NVAPI_END_ENUMERATION)
            break;
        if (ret == NVAPI_OK) {
            // NvAPI_GetAssociatedDisplayOutputId returns the "legacy display output bit mask", which is
            // not the NVAPI display ID we are handling in targetInfo
            // so first find the NT-display name
            if(NvAPI_GetAssociatedNvidiaDisplayName(handle, name) != NVAPI_OK) {
                continue;
            }
            // then get the display ID for that name
            NvU32 assoc_displayId;
            // TODO: this does not actually work for cloned targets.
            // TODO: for some reason the association made by NvAPI_DISP_GetDisplayIdByDisplayName is consistently
            // TODO: incorrect and it will claim a given displayID is associated with the "other" (non-clone-source,
            // TODO: in my experiments anyway) display.
            // TODO: This leaves one \\.\DISPLAYx un-associated as far as NvAPI is concerned and it will return
            // TODO: NVAPI_NVIDIA_DEVICE_NOT_FOUND.
            if(NvAPI_DISP_GetDisplayIdByDisplayName(name, &assoc_displayId) != NVAPI_OK) {
                continue;
            }
            if(assoc_displayId == displayId) {
                return strdup(name);
            }
        }
    }
    return NULL;
}

char *get_display_device_path_for_nt_display(const char *nt_display) {
    DISPLAY_DEVICE device;
    device.cb = sizeof(DISPLAY_DEVICE);
    if(!EnumDisplayDevices(nt_display, 0, &device, EDD_GET_DEVICE_INTERFACE_NAME))
        return NULL;
    return strdup(device.DeviceID);
}

char *get_human_monitor_name(const char *monitor_device_path) {
    // note how this is very similar to interrogating NVAPI about display paths
    // but since the IDs don't line up we'll just have to walk everything twice
    if(!monitor_device_path)
        return NULL;
    DISPLAYCONFIG_PATH_INFO *paths = NULL;
    DISPLAYCONFIG_MODE_INFO *modes = NULL;
    UINT32 path_count = 0, mode_count = 0;
    UINT32 flags = QDC_ONLY_ACTIVE_PATHS | QDC_VIRTUAL_MODE_AWARE;
    LONG result = ERROR_SUCCESS;
    char *name = NULL;

    result = GetDisplayConfigBufferSizes(flags, &path_count, &mode_count);
    if(result != ERROR_SUCCESS) {
        goto cleanup;
    }

    paths = (DISPLAYCONFIG_PATH_INFO*)calloc(path_count, sizeof(DISPLAYCONFIG_PATH_INFO));
    modes = (DISPLAYCONFIG_MODE_INFO*)calloc(mode_count, sizeof(DISPLAYCONFIG_MODE_INFO));
    result = QueryDisplayConfig(flags, &path_count, paths, &mode_count, modes, NULL);
    if(result != ERROR_SUCCESS) {
        goto cleanup;
    }

    for(UINT32 i = 0; i < path_count; i++) {
        auto &path = paths[i];
        // Find the target (monitor) friendly name
        DISPLAYCONFIG_TARGET_DEVICE_NAME targetName = {};
        targetName.header.adapterId = path.targetInfo.adapterId;
        targetName.header.id = path.targetInfo.id;
        targetName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
        targetName.header.size = sizeof(targetName);
        result = DisplayConfigGetDeviceInfo(&targetName.header);
        if(result != ERROR_SUCCESS) {
            goto cleanup;
        }

        char *mdp = wchar_to_char(targetName.monitorDevicePath);
        if (!strcmp(monitor_device_path, mdp)) {
            const wchar_t *wname = targetName.flags.friendlyNameFromEdid ? targetName.monitorFriendlyDeviceName
                                                                         : L"Generique PnP monitor";
            name = wchar_to_char(wname);
            free(mdp);
            goto cleanup;
        }
        free(mdp);
    }

cleanup:
    free(paths);
    free(modes);
    return name;
}

// stay tuned for DisplayPort 2.0
const char* format_link_rate(NV_DP_LINK_RATE rate) {
    switch (rate) {
        case NV_DP_1_62GBPS: return "DP 1.6 Gbps (RBR)";
        case NV_DP_2_70GBPS: return "DP 2.7 Gbps (HBR)";
        case NV_DP_5_40GBPS: return "DP 5.4 Gbps (HBR2)";
        case NV_DP_8_10GBPS: return "DP 8.1 Gbps (HBR3)";
        case NV_EDP_2_16GBPS: return "eDP 2.16 Gbps";
        case NV_EDP_2_43GBPS: return "eDP 2.43 Gbps";
        case NV_EDP_3_24GBPS: return "eDP 3.24 Gbps";
        case NV_EDP_4_32GBPS: return "eDP 4.32 Gbps";
        default: return aprintf("unknown (%d)", rate);
    }
}

const char *format_dp_link(NV_DP_LINK_RATE rate, NV_DP_LANE_COUNT lane_count) {
    return aprintf("%s x %d lanes", format_link_rate(rate), lane_count);
}

double link_net_rate_gbps(NV_DP_LINK_RATE rate) {
    switch(rate) {
        case NV_DP_1_62GBPS: return 1.62 * 0.8;
        case NV_DP_2_70GBPS: return 2.7 * 0.8;
        case NV_DP_5_40GBPS: return 5.4 * 0.8;
        case NV_DP_8_10GBPS: return 8.1 * 0.8;
        case NV_EDP_2_16GBPS: return 2.16 * 0.8;
        case NV_EDP_2_43GBPS: return 2.43 * 0.8;
        case NV_EDP_3_24GBPS: return 3.24 * 0.8;
        case NV_EDP_4_32GBPS: return 4.32 * 0.8;
        default: return 0;
    }
}

const char *format_color_format(NV_DP_COLOR_FORMAT format) {
    switch(format) {
        case NV_DP_COLOR_FORMAT_RGB: return "RGB";
        case NV_DP_COLOR_FORMAT_YCbCr422: return "YCbCr (YUV) 4:2:2";
        case NV_DP_COLOR_FORMAT_YCbCr444: return "YCbCr (YUV) 4:4:4";
        default: return aprintf("unknown (%d)", format);
    }
}

const char *format_color_space(NV_DP_COLORIMETRY color) {
    switch (color) {
        case NV_DP_COLORIMETRY_RGB:
            return "sRGB";
        case NV_DP_COLORIMETRY_YCbCr_ITU601:
            return "ITU.601";
        case NV_DP_COLORIMETRY_YCbCr_ITU709:
            return "ITU.709";
        case 8:
            return "Rec.2020?"; // YCbCr [informed guess via EDID not advertising DCI P-3]
        case 9:
            return "Rec.2020?"; // RGB [informed guess via EDID not advertising DCI P-3]
        default:
            return aprintf("unknown (%d)", color);
    }
}

int bpc_to_bits(NV_DP_BPC bpc) {
    switch(bpc) {
        case NV_DP_BPC_DEFAULT: return 0;
        case NV_DP_BPC_6: return 6;
        case NV_DP_BPC_8: return 8;
        case NV_DP_BPC_10: return 10;
        case NV_DP_BPC_12: return 12;
        case NV_DP_BPC_16: return 16;
        default: return -1;
    }
}

const char *format_source_format(NV_FORMAT format) {
    switch(format) {
        case NV_FORMAT_P8: return "8-bit (palette)";
        case NV_FORMAT_R5G6B5: return "16-bit (RGB565)";
        case NV_FORMAT_A8R8G8B8: return "32-bit (RGB888)";
        case NV_FORMAT_A16B16G16R16F: return "64-bit fp (RGBA half)";
        default: return aprintf("unknown (%d)", format);
    }
}

const char *format_connector_type(NV_GPU_CONNECTOR_TYPE type) {
    switch (type) {
        case NVAPI_GPU_CONNECTOR_VGA_15_PIN:
            return "VGA";
        case NVAPI_GPU_CONNECTOR_DVI_I_TV_SVIDEO:
        case NVAPI_GPU_CONNECTOR_DVI_I_TV_COMPOSITE:
        case NVAPI_GPU_CONNECTOR_LFH_DVI_I_1:
        case NVAPI_GPU_CONNECTOR_LFH_DVI_I_2:
        case NVAPI_GPU_CONNECTOR_DVI_I:
            return "DVI-I (analog)";
        case NVAPI_GPU_CONNECTOR_DVI_D:
            return "DVI-D";
        case NVAPI_GPU_CONNECTOR_SPWG:
            return "LVDS";
        case NVAPI_GPU_CONNECTOR_OEM:
            return "OEM";
        case NVAPI_GPU_CONNECTOR_DISPLAYPORT_EXTERNAL:
        case NVAPI_GPU_CONNECTOR_DISPLAYPORT_MINI_EXT:
        case NVAPI_GPU_CONNECTOR_LFH_DISPLAYPORT_1:
        case NVAPI_GPU_CONNECTOR_LFH_DISPLAYPORT_2:
            return "DisplayPort";
        case NVAPI_GPU_CONNECTOR_HDMI_A:
        case NVAPI_GPU_CONNECTOR_HDMI_C_MINI:
            return "HDMI";
        case NVAPI_GPU_CONNECTOR_VIRTUAL_WFD:
            return "VirtualLink (USB-C)";
        case NVAPI_GPU_CONNECTOR_USB_C:
            return "USB-C";
        default:
            if (type & NVAPI_GPU_CONNECTOR_TV_COMPOSITE) {
                return "Misc. analog";
            }
            return aprintf("unknown (%d)", type);
    }
}

const char *format_hdr_mode(NV_HDR_MODE mode) {
    switch (mode) {
        case NV_HDR_MODE_OFF: return "Off";
        case NV_HDR_MODE_UHDA: return "UHDA (HDR10 w/ Rec.2020)";
        case NV_HDR_MODE_UHDA_PASSTHROUGH: return "HDR10 pass-thru";
        case NV_HDR_MODE_DOLBY_VISION: return "DolbyVision";
        default: return aprintf("unknown (%d)", mode);
    }
}

const char *format_scaling(NV_SCALING mode) {
    // these are kinda wonky
    switch (mode) {
        case NV_SCALING_DEFAULT: return "uhm...?";
        case NV_SCALING_GPU_SCALING_TO_CLOSEST: return "GPU / Balanced Full Screen";
        case NV_SCALING_GPU_SCALING_TO_NATIVE: return "Force GPU scaling / Full Screen";
        case NV_SCALING_GPU_SCANOUT_TO_NATIVE: return "No scaling (native resolution)";
        case NV_SCALING_GPU_SCANOUT_TO_CLOSEST: return "No scaling (closest resolution)";
        case NV_SCALING_GPU_SCALING_TO_ASPECT_SCANOUT_TO_NATIVE: return "Preserve aspect ratio (native resolution)";
        case NV_SCALING_GPU_SCALING_TO_ASPECT_SCANOUT_TO_CLOSEST: return "Preserve aspect ratio (closest resolution)";
        case NV_SCALING_GPU_INTEGER_ASPECT_SCALING: return "Integer scaling";
        default: return "unknown";
    }
}

char *aprintf(const char* format, ...) {
    va_list args, copy;
    va_start(args, format);
    va_copy(copy, args);
    int size = vsnprintf(NULL, 0, format, copy);
    va_end(copy);
    if(size < 0) {
        va_end(args);
        return NULL;
    }
    char *str = (char*)malloc(size + 1);
    int result = vsnprintf(str, size + 1, format, args);
    va_end(args);
    if(result < 0) {
        free(str);
        return NULL;
    }
    return str;
}

char *wchar_to_char(const WCHAR *wstr) {
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
    char* cstr = (char*)malloc(bufferSize);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, cstr, bufferSize, NULL, NULL);
    return cstr;
}
