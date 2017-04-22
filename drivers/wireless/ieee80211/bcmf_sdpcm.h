#ifndef __DRIVERS_WIRELESS_IEEE80211_BCMF_SDPCM_H
#define __DRIVERS_WIRELESS_IEEE80211_BCMF_SDPCM_H

#include "bcmf_driver.h"

int bcmf_sdpcm_readframe(FAR struct bcmf_dev_s *priv);

int bcmf_sdpcm_sendframe(FAR struct bcmf_dev_s *priv);

int bcmf_sdpcm_iovar_request(FAR struct bcmf_dev_s *priv,
                             uint32_t ifidx, bool set, char *name,
                             uint8_t *data, uint32_t *len);

int bcmf_sdpcm_ioctl(FAR struct bcmf_dev_s *priv,
                     uint32_t ifidx, bool set, uint32_t cmd,
                     uint8_t *data, uint32_t *len);

#endif /* __DRIVERS_WIRELESS_IEEE80211_BCMF_SDPCM_H */