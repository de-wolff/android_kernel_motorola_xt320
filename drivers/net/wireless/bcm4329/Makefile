# bcm4329

DHDCFLAGS = -DLINUX -DBCMDRIVER -DBCMDONGLEHOST -DDHDTHREAD         \
 -DUNRELEASEDCHIP -Dlinux -DDHD_SDALIGN=64 -DMAX_HDR_READ=64           \
 -DDHD_FIRSTREAD=64 -DDHD_GPL -DDHD_SCHED -DBDC -DTOE -DDHD_BCMEVENTS  \
 -DSHOW_EVENTS -DBCMSDIO -DDHD_GPL -DBCMLXSDMMC -DBCMPLATFORM_BUS      \
 -Wall -Wstrict-prototypes -Werror \
 -DARP_OFFLOAD_SUPPORT \
 -DOEM_ANDROID \
 -DWL_CFG80211 -DWLP2P \
 -DSTA -DWLBTAMP  -DSOFTAP -DCSCAN \
 -Idrivers/net/wireless/bcm4329 -Idrivers/net/wireless/bcm4329/include \
 -DWL_COMPAT_WIRELESS -DWIFI_ACT_FRAME -DCUSTOMER_HW  -DDONGLEOVERLAYS \
 -DBCMDMA32 -DDHD_DEBUG -DSDTEST -DKEEP_ALIVE -DBCMWAPI_WPI -DBCMWAPI_WAI \
 -DMMC_SDIO_ABORT -DOOB_INTR_ONLY -DCUSTOM_OOB_GPIO_NUM=27  -DHW_OOB  -DDHD_USE_STATIC_BUF
# -DSDIO_ISR_THREAD 
# -DEMBEDDED_PLATFORM \^M

 #-DMMC_SDIO_ABORT -DOOB_INTR_ONLY -DCUSTOM_OOB_GPIO_NUM=27  -DHW_OOB \


DHDOFILES = \
	dhd_linux.o \
	linux_osl.o \
	bcmutils.o \
	dhd_common.o \
	dhd_custom_gpio.o \
	siutils.o \
	sbutils.o \
	aiutils.o \
	hndpmu.o \
	wl_iw.o \
	wl_cfg80211.o \
	wl_cfgp2p.o \
	bcmwifi.o \
	dhd_cdc.o \
	dhd_sdio.o \
	dhd_linux_sched.o \
	bcmsdh_sdmmc.o \
	bcmsdh.o \
	bcmsdh_linux.o \
	bcmsdh_sdmmc_linux.o \
	bcmevent.o \
	dhd_bta.o \
	wldev_common.o \
	wl_android.o \
	dhd_linux_mon.o

obj-$(CONFIG_BCM4329) += bcm4329.o
bcm4329-objs += $(DHDOFILES)
EXTRA_CFLAGS = $(DHDCFLAGS)
EXTRA_LDFLAGS += --strip-debug
