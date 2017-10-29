#pragma once

#ifdef SCANBD_CFG_DIR
#define SCANBD_CONF  SCANBD_CFG_DIR "/scanbd.conf"
#else
#error SCANBD_CFG_DIR is not set!
#endif


