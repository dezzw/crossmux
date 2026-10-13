#pragma once

#if defined(FREEINK_DEVICE_WAVESHARE_EPAPER_397) && !defined(SIMULATOR) && !defined(CROSSPOINT_HOST_TEST)
#include <sdkconfig.h>
#if !defined(CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE) || !CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
#error "Waveshare OTA requires CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE=y (prebuilt dio_opi sdkconfig)"
#endif
#endif
