//  Copyright (c) 2009-2011, Code Aurora Forum. All rights reserved.
// 
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions are met:
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//     * Neither the name of Code Aurora nor
//       the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

on boot-pause-one
    exec /system/bin/battery_charging 1

on boot-pause-two
    exec /system/bin/battery_charging 2

on boot
    mkdir /data/misc/bluetooth 0770 bluetooth bluetooth
    chown bluetooth bluetooth /sys/module/bluetooth_power/parameters/power
    chown bluetooth bluetooth /sys/class/rfkill/rfkill0/type
    chown bluetooth bluetooth /sys/class/rfkill/rfkill0/state
    chown bluetooth bluetooth /proc/bluetooth/sleep/proto
    chown system system /sys/module/sco/parameters/disable_esco
    chmod 0660 /sys/module/bluetooth_power/parameters/power
    chmod 0660 /sys/class/rfkill/rfkill0/state
    chmod 0660 /proc/bluetooth/sleep/proto
    chown bluetooth bluetooth /dev/ttyHS0
    chmod 0660 /dev/ttyHS0
    chown bluetooth bluetooth /sys/devices/platform/msm_serial_hs.0/clock
    chmod 0660 /sys/devices/platform/msm_serial_hs.0/clock

    // Create the directories used by the Wireless subsystem
    mkdir /data/misc/wifi 0770 wifi wifi
    mkdir /data/misc/wifi/sockets 0770 wifi wifi
    mkdir /data/misc/wifi/wpa_supplicant 0770 wifi wifi
    mkdir /data/misc/dhcp 0770 dhcp dhcp
    chown dhcp dhcp /data/misc/dhcp

    setprop wifi.interface wlan0

    chmod 0777 /system
    chmod 0777 /system/etc
    chmod 0777 /system/etc/wifi
    chmod 0777 /system/etc/wifi/wpa_supplicant.conf
    chown wifi wifi /system/etc/wifi/wpa_supplicant.conf
    // add by barry 
    mkdir /data/system 0775  system system
    mkdir /data/system/wpa_supplicant 0770 wifi wifi

    chmod 0775 /data/system/wpa_supplicant
    chmod 0771 /data
    chmod 0775 /data/misc
    chmod 0775 /data/misc/wifi
    chmod 0775 /data/misc/wifi/wpa_supplicant.conf

    // Create the symlink to qcn wpa_supplicant folder for ar6000 wpa_supplicant
    mkdir /data/system 0775 system system
    symlink /data/misc/wifi/wpa_supplicant /data/system/wpa_supplicant

    // Enable Bluetooth 802.11 PAL when Bluetooth is turned on
    setprop ro.config.bt.amp no

     // Create directories for wiper services
    mkdir /data/wpstiles/ 0775 shell
    mkdir /data/wiper 0775 location qcom_oncrpc

     // Create directories for gpsone_daemon services
    mkdir /data/misc/gpsone_d 0770 system system

     // Create directory from IMS services
    mkdir /data/shared 0775

    // Create directory for FOTA
    mkdir /data/fota 0771
    chown system system /data/fota

    //Set SUID bit for usbhub
    chmod 4755 /system/bin/usbhub
    chmod 755 /system/bin/usbhub_init

    //Set SUID bit for diag_mdlog
    chmod 4755 /system/bin/diag_mdlog

    //Set SUID bit for btwlancoex
    chmod 4755 /system/bin/btwlancoex

    //Remove SUID bit for iproute2 ip tool
    chmod 0755 /system/bin/ip

    //Provide the access to hostapd.conf only to root and group
    chmod 0660 /data/hostapd/hostapd.conf

    //port-bridge
    chmod 0660 /dev/smd0
    chown system system /dev/smd0

    chmod 0660 /dev/ttyGS0
    chown system system /dev/ttyGS0

    chmod 0444 /sys/devices/platform/msm_hsusb/gadget/usb_state

    // Remove write permissions to video related nodes
    chmod 0664 /sys/devices/virtual/graphics/fb1/hpd
    chmod 0664 /sys/devices/virtual/graphics/fb1/video_mode
    chmod 0664 /sys/devices/virtual/graphics/fb1/format_3d

    // Change owner and group for media server and surface flinger
    chown media system /sys/devices/virtual/graphics/fb1/format_3d

    //For netmgr daemon to inform the USB driver of the correct transport
    chown radio radio /sys/class/usb_composite/rmnet_smd_sdio/transport

    //To allow interfaces to get v6 address when tethering is enabled
    write /proc/sys/net/ipv6/conf/rmnet0/accept_ra 2
    write /proc/sys/net/ipv6/conf/rmnet1/accept_ra 2
    write /proc/sys/net/ipv6/conf/rmnet2/accept_ra 2
    write /proc/sys/net/ipv6/conf/rmnet3/accept_ra 2
    write /proc/sys/net/ipv6/conf/rmnet4/accept_ra 2
    write /proc/sys/net/ipv6/conf/rmnet5/accept_ra 2
    write /proc/sys/net/ipv6/conf/rmnet6/accept_ra 2
    write /proc/sys/net/ipv6/conf/rmnet7/accept_ra 2
    write /proc/sys/net/ipv6/conf/rmnet_sdio0/accept_ra 2
    write /proc/sys/net/ipv6/conf/rmnet_sdio1/accept_ra 2
    write /proc/sys/net/ipv6/conf/rmnet_sdio2/accept_ra 2
    write /proc/sys/net/ipv6/conf/rmnet_sdio3/accept_ra 2
    write /proc/sys/net/ipv6/conf/rmnet_sdio4/accept_ra 2
    write /proc/sys/net/ipv6/conf/rmnet_sdio5/accept_ra 2
    write /proc/sys/net/ipv6/conf/rmnet_sdio6/accept_ra 2
    write /proc/sys/net/ipv6/conf/rmnet_sdio7/accept_ra 2

// Export GPIO56 for fusion targets to enable/disable hub
service usbhub_init /system/bin/usbhub_init
   user root
   disabled
   oneshot

on property:ro.product.device=msm7630_fusion
    start usbhub_init

on property:init.svc.wpa_supplicant=stopped
    stop dhcpcd

on property:init.svc.bluetoothd=running
    start btwlancoex
    start amp_load
    write /sys/class/bluetooth/hci0/idle_timeout 7000

on property:init.svc.bluetoothd=stopped
    start amp_unload
    stop btwlancoex

on property:persist.cne.UseCne=none
    stop cnd

service cnd /system/bin/cnd
    socket cnd stream 660 root radio

service rmt_storage /system/bin/rmt_storage /dev/block/mmcblk0p10 /dev/block/mmcblk0p11 /dev/block/mmcblk0p17
    user root
    disabled

on property:ro.emmc=1
    start rmt_storage

service hciattach /system/bin/sh /system/etc/init.brcm.bt.sh
    user bluetooth
    group qcom_oncrpc bluetooth net_bt_admin
    disabled
    oneshot

service btld  /system/bin/btld 
    user root
    group bluetooth net_bt_admin
    disabled
    oneshot

on property:bt.hci_smd.driver.load=1
    insmod /system/lib/modules/hci_smd.ko

on property:bt.hci_smd.driver.load=0
    exec /system/bin/rmmod hci_smd.ko

service bt-dun /system/bin/dun-server /dev/smd7 /dev/rfcomm0
    disabled
    oneshot

service bt-sap /system/bin/sapd 15
    disabled
    oneshot
service sapd /system/bin/sdptool add --channel=15 SAP
    user bluetooth
    group bluetooth net_bt_admin
    disabled
    oneshot
service dund /system/bin/sdptool add --channel=1 DUN
    user bluetooth
    group bluetooth net_bt_admin
    disabled
    oneshot

service bridgemgrd /system/bin/bridgemgrd
    user radio
    group radio
    disabled

service port-bridge /system/bin/port-bridge /dev/smd0 /dev/ttyGS0
    socket cellonmattest stream 0660 radio system
    user system
    group system inet
    disabled

on property:ro.baseband="msm"
    start port-bridge
    // Enable BT-DUN only for all msms
    setprop ro.qualcomm.bluetooth.dun true

on property:ro.baseband="unknown"
    start port-bridge

service qmiproxy /system/bin/qmiproxy
    user radio
    group radio
    disabled

service qmuxd /system/bin/qmuxd
    user radio
    group radio
    disabled

service netmgrd /system/bin/netmgrd
    disabled

service sensors /system/bin/sensors.qcom
    user root
    group root
    disabled

on property:ro.use_data_netmgrd=false
    // netmgr not supported on specific target
    stop netmgrd

// Adjust socket buffer to enlarge TCP receive window for high bandwidth
// but only if ro.data.large_tcp_window_size property is set.
on property:ro.data.large_tcp_window_size=true
    write /proc/sys/net/ipv4/tcp_adv_win_scale  1

service btwlancoex /system/bin/sh /system/etc/init.qcom.coex.sh
    user shell
    group bluetooth net_bt_admin
    disabled
    oneshot

service amp_init /system/bin/amploader -i
    user root
    disabled
    oneshot

service amp_load /system/bin/amploader -l 7000
    user root
    disabled
    oneshot

service amp_unload /system/bin/amploader -u
    user root
    disabled
    oneshot

service wpa_supplicant /system/bin/wpa_supplicant -Dnl80211 -iwlan0 -c/data/misc/wifi/wpa_supplicant.conf -puse_p2p_group_interface=1 -ddd 
    user root
    group wifi inet
    disabled
    oneshot

service p2p_supplicant /system/bin/wpa_supplicant -Dnl80211 -iwlan0 -c/data/misc/wifi/p2p_supplicant.conf -puse_p2p_group_interface=1 -ddd
    user root
    group wifi inet
    disabled
    oneshot

service dhcpcd_wlan0 /system/bin/dhcpcd -ABKL 
    disabled
    oneshot

service dhcpcd_p2p /system/bin/dhcpcd -ABKLG
    disabled
    oneshot

service wiperiface /system/bin/wiperiface
    user location
    group qcom_oncrpc
    oneshot

service gpsone_daemon /system/bin/gpsone_daemon
    user system
    group system qcom_oncrpc inet
    disabled

service fm_dl /system/bin/sh /system/etc/init.qcom.fm.sh
    user root
    group system qcom_oncrpc
    disabled
    oneshot

on property:crypto.driver.load=1
     insmod /system/lib/modules/qce.ko
     insmod /system/lib/modules/qcedev.ko

on property:crypto.driver.load=0
     exec /system/bin/rmmod qcedev.ko
     exec /system/bin/rmmod qce.ko

service drmdiag /system/bin/drmdiagapp
     user root
     disabled
     oneshot

on property:drmdiag.load=1
    start drmdiag

on property:drmdiag.load=0
    stop drmdiag

service qcom-sh /system/bin/sh /init.qcom.sh
    user root
    oneshot

// service qcom-post-boot /system/bin/sh /system/etc/init.qcom.post_boot.sh
//    user root
//    disabled
//    oneshot

// on property:init.svc.bootanim=stopped
//    start qcom-post-boot

service atfwd /system/bin/ATFWD-daemon
    user system
    group system radio
    onrestart /system/bin/log -t RIL-ATFWD -p w "ATFWD daemon restarted"

service hdmid /system/bin/hdmid
    socket hdmid stream 0660 root system graphics
    disabled

on property:ro.hdmi.enable=true
    start hdmid

service abld /system/bin/mm-abl-daemon
    disabled

service hostapd /system/bin/hostapd -dddd /data/hostapd/hostapd.conf
    user root
    group root
    oneshot
    disabled

service ds_fmc_appd /system/bin/ds_fmc_appd -p "rmnet0" -D
    group radio
    disabled
    oneshot

on property:persist.data.ds_fmc_app.mode=1
    start ds_fmc_appd

service ims_regmanager /system/bin/exe-ims-regmanagerprocessnative
    user system
    group qcom_oncrpc net_bt_admin inet radio wifi
    disabled

on property:persist.ims.regmanager.mode=1
    start ims_regmanager

on property:ro.data.large_tcp_window_size=true
    // Adjust socket buffer to enlarge TCP receive window for high bandwidth (e.g. DO-RevB)
    write /proc/sys/net/ipv4/tcp_adv_win_scale  1

service thermald /system/bin/thermald
    user root
    group root
    disabled

on property:persist.thermal.monitor=true
    start thermald

service time_daemon /system/bin/time_daemon
    user root
    group root
    oneshot
    disabled

on property:persist.timed.enable=true
    mkdir /data/time/ 0700
    start time_daemon

service ftp /system/bin/sdptool add --psm=5257 --channel=20 FTP
    user bluetooth
    group bluetooth net_bt_admin
    disabled
    oneshot

service map /system/bin/sdptool add --channel=16 MAS
    user bluetooth
    group bluetooth net_bt_admin
    disabled
    oneshot

service ril-daemon1 /system/bin/rild -c 1
    socket rild1 stream 660 root radio
    socket rild-debug1 stream 660 radio system
    user root
    disabled
    group radio cache inet misc audio sdcard_rw qcom_oncrpc diag

service profiler_daemon /system/bin/profiler_daemon
    user root
    group root
    disabled

// bugreport is triggered by the KEY_CAMERA and KEY_VOLUMEUP keycodes
// service bugreport /system/bin/dumpstate -d -v -g -o /sdcard/bugreports/bugreport
//     disabled
//     oneshot
//     keycodes 68 115
    
service dhcpcd_eth0 /system/bin/dhcpcd -ABKL
    disabled
    oneshot


service iprenew_wlan0 /system/bin/dhcpcd -n
    disabled
    oneshot


service iprenew_p2p /system/bin/dhcpcd -n
    disabled
    oneshot


service dlna_p2p /system/bin/dms -i
    disabled
    oneshot
