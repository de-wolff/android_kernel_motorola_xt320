on early-init
    start ueventd

on init

sysclktz 0

loglevel 3

// setup the global environment
    export PATH /sbin:/vendor/bin:/system/sbin:/system/bin:/system/xbin
    // add flex lib +++ virgil
    export LD_LIBRARY_PATH /flex/lib:/vendor/lib:/system/lib
    export ANDROID_BOOTLOGO 1
    export ANDROID_ROOT /system
    export ANDROID_ASSETS /system/app
    export ANDROID_DATA /data
    export EXTERNAL_STORAGE /mnt/sdcard
    export ASEC_MOUNTPOINT /mnt/asec
    export LOOP_MOUNTPOINT /mnt/obb
    export BOOTCLASSPATH /system/framework/core.jar:/system/framework/bouncycastle.jar:/system/framework/ext.jar:/system/framework/framework.jar:/system/framework/android.policy.jar:/system/framework/services.jar:/system/framework/core-junit.jar:/system/framework/qcrilhook.jar:/system/framework/qcnvitems.jar

// Backward compatibility
    symlink /system/etc /etc
    symlink /sys/kernel/debug /d
    mount debugfs debugfs /sys/kernel/debug

// Right now vendor lives on the same filesystem as system,
// but someday that may change.
    symlink /system/vendor /vendor

// create mountpoints
    mkdir /mnt 0775 root system
    mkdir /mnt/sdcard 0000 system system

// create flex partition +++virgil
    mkdir /flex 0775 root system

// Create cgroup mount point for cpu accounting
    mkdir /acct
    mount cgroup none /acct cpuacct
    mkdir /acct/uid

// Backwards Compat - XXX: Going away in G*
    symlink /mnt/sdcard /sdcard

    mkdir /system
    mkdir /data 0771 system system
    mkdir /persist 0771 system system
    mkdir /cache 0770 system cache
    mkdir /config 0500 root root
    mkdir /persist 0771 system system

    // Directory for putting things only root should see.
    mkdir /mnt/secure 0700 root root

    // Directory for staging bindmounts
    mkdir /mnt/secure/staging 0700 root root

    // Directory-target for where the secure container
    // imagefile directory will be bind-mounted
    mkdir /mnt/secure/asec  0700 root root

    // Secure container public mount points.
    mkdir /mnt/asec  0700 root system
    mount tmpfs tmpfs /mnt/asec mode=0755,gid=1000

    // Filesystem image public mount points.
    mkdir /mnt/obb 0700 root system
    mount tmpfs tmpfs /mnt/obb mode=0755,gid=1000

    write /proc/sys/kernel/panic_on_oops 1
    write /proc/sys/kernel/hung_task_timeout_secs 0
    write /proc/cpu/alignment 4
    write /proc/sys/kernel/sched_latency_ns 10000000
    write /proc/sys/kernel/sched_wakeup_granularity_ns 2000000
    write /proc/sys/kernel/sched_compat_yield 1
    write /proc/sys/kernel/sched_child_runs_first 0

// Create cgroup mount points for process groups
    mkdir /dev/cpuctl
    mount cgroup none /dev/cpuctl cpu
    chown system system /dev/cpuctl
    chown system system /dev/cpuctl/tasks
    chmod 0777 /dev/cpuctl/tasks
    write /dev/cpuctl/cpu.shares 1024

    mkdir /dev/cpuctl/fg_boost
    chown system system /dev/cpuctl/fg_boost/tasks
    chmod 0777 /dev/cpuctl/fg_boost/tasks
    write /dev/cpuctl/fg_boost/cpu.shares 1024

    mkdir /dev/cpuctl/bg_non_interactive
    chown system system /dev/cpuctl/bg_non_interactive/tasks
    chmod 0777 /dev/cpuctl/bg_non_interactive/tasks
    // 5.0 %
    write /dev/cpuctl/bg_non_interactive/cpu.shares 52

    chmod 0666 /dev/akm8975_dev
    chmod 0666 /dev/akm8975_aot
    chmod 0666 /dev/bma250
    chmod 0666 /dev/ltr505

on fs
// mount mtd partitions
    // Mount /system rw first to give the filesystem a chance to save a checkpoint
    mount yaffs2 mtd@system /system
    mount yaffs2 mtd@system /system ro remount
    mount yaffs2 mtd@userdata /data nosuid nodev
    mount yaffs2 mtd@persist /persist nosuid nodev
    mount yaffs2 mtd@cache /cache nosuid nodev
    //flex partition +++ virgil
    mount yaffs2 mtd@flex /flex nosuid nodev
    mount yaffs2 mtd@flex /flex nosuid nodev ro remount
    
on ubi-fs
    mount ubifs ubi@system /system
    mount ubifs ubi@system /system ro remount
    mount ubifs ubi@flex /flex nosuid nodev
    mount ubifs ubi@flex /flex nosuid nodev ro remount
    mount ubifs ubi@userdata /data nosuid nodev
    mount ubifs ubi@cache /cache nosuid nodev

on emmc-fs
// mount mmc partitions
    wait /dev/block/mmcblk0p12
    mount ext4 /dev/block/mmcblk0p12 /system ro barrier=1
    wait /dev/block/mmcblk0p13
    exec /system/bin/e2fsck -p /dev/block/mmcblk0p13
    mount ext4 /dev/block/mmcblk0p13 /data nosuid nodev barrier=1 noauto_da_alloc
    mount ext4 /dev/block/mmcblk0p14 /persist nosuid nodev barrier=1
    mount ext4 /dev/block/mmcblk0p15 /cache nosuid nodev barrier=1

on post-fs
    // once everything is setup, no need to modify /
    mount rootfs rootfs / ro remount

    // We chown/chmod /data again so because mount is run as root + defaults
    chown system system /data
    chmod 0771 /data

    // Mounting of persist is moved to 'on emmc-fs' and 'on fs' sections
    // We chown/chmod /persist again so because mount is run as root + defaults
    chown system system /persist
    chmod 0771 /persist
    chmod 0664 /sys/devices/platform/msm_sdcc.1/polling
    chmod 0664 /sys/devices/platform/msm_sdcc.2/polling
    chmod 0664 /sys/devices/platform/msm_sdcc.3/polling
    chmod 0664 /sys/devices/platform/msm_sdcc.4/polling

    // Chown polling nodes as needed from UI running on system server
    chown system system /sys/devices/platform/msm_sdcc.1/polling
    chown system system /sys/devices/platform/msm_sdcc.2/polling
    chown system system /sys/devices/platform/msm_sdcc.3/polling
    chown system system /sys/devices/platform/msm_sdcc.4/polling

    // Create dump dir and collect dumps.
    // Do this before we mount cache so eventually we can use cache for
    // storing dumps on platforms which do not have a dedicated dump partition.
   
    mkdir /data/anr
    chown system system /data/anr
    chmod 0775 /data/anr

    mkdir /data/dontpanic
    chown root log /data/dontpanic
    chmod 0750 /data/dontpanic

    // Collect apanic data, free resources and re-arm trigger
    copy /proc/apanic_console /data/dontpanic/apanic_console
    chown root log /data/dontpanic/apanic_console
    chmod 0640 /data/dontpanic/apanic_console

    copy /proc/apanic_threads /data/dontpanic/apanic_threads
    chown root log /data/dontpanic/apanic_threads
    chmod 0640 /data/dontpanic/apanic_threads

    write /proc/apanic_console 1

    copy /sys/kernel/debug/smd/modem_err_crash /data/dontpanic/modem_err_crash
    chown root log /data/dontpanic/modem_err_crash
    chmod 0640 /data/dontpanic/modem_err_crash

    copy /sys/kernel/debug/smd/modem_err_trace /data/dontpanic/modem_err_trace
    chown root log /data/dontpanic/modem_err_trace
    chmod 0640 /data/dontpanic/modem_err_trace

    mkdir /data/panic
    chown root log /data/panic
    chmod 0750 /data/panic

    mkdir /data/panic/apr
    chown root log /data/panic/apr
    chmod 0750 /data/panic/apr

    exec /system/bin/apr-bppanic-collector

    // Same reason as /data above
    chown system cache /cache
    chmod 0770 /cache

    // This may have been created by the recovery system with odd permissions
    chown system cache /cache/recovery
    chmod 0770 /cache/recovery

    //change permissions on vmallocinfo so we can grab it from bugreports
    chown root log /proc/vmallocinfo
    chmod 0440 /proc/vmallocinfo

    //change permissions on kmsg & sysrq-trigger so bugreports can grab kthread stacks
    chown root system /proc/kmsg
    chmod 0440 /proc/kmsg
    chown root system /proc/sysrq-trigger
    chmod 0220 /proc/sysrq-trigger

    // permissions for NFC
    // setprop ro.nfc.port "I2C"
    // chmod 0600 /dev/pn544
    // chown nfc nfc /dev/pn544

// create basic filesystem structure
    mkdir /data/misc 01771 system misc
    mkdir /data/misc/bluetoothd 0770 bluetooth bluetooth
    mkdir /data/misc/bluetooth 0770 system system
    mkdir /data/misc/keystore 0700 keystore keystore
    mkdir /data/misc/vpn 0770 system system
    mkdir /data/misc/systemkeys 0700 system system
    mkdir /data/misc/vpn/profiles 0770 system system
    // give system access to wpa_supplicant.conf for backup and restore
    mkdir /data/misc/wifi 0770 wifi wifi
    chmod 0770 /data/misc/wifi
    chmod 0660 /data/misc/wifi/wpa_supplicant.conf
    mkdir /data/local 0771 shell shell
    mkdir /data/local/tmp 0771 shell shell
    mkdir /data/data 0771 system system
    mkdir /data/app-private 0771 system system
    mkdir /data/app 0771 system system
    mkdir /data/property 0700 root root
    mkdir /data/radio 0770 radio radio
    mkdir /data/misc/sensors 0775 system system
    write /data/system/sensors/settings 0
    chmod 0664 /data/system/sensors/settings
	
    // create dalvik-cache and double-check the perms
    mkdir /data/dalvik-cache 0771 system system
    chown system system /data/dalvik-cache
    // Discretix change start
    // chmod 0771 /data/dalvik-cache
    chmod 0775 /data/dalvik-cache
    // Discretix change end

    // create the lost+found directories, so as to enforce our permissions
    mkdir /data/lost+found 0770
    mkdir /cache/lost+found 0770

    // double check the perms, in case lost+found already exists, and set owner
    chown root root /data/lost+found
    chmod 0770 /data/lost+found
    chown root root /cache/lost+found
    chmod 0770 /cache/lost+found

    // Discretix DRM change start
    mkdir /data/DxDrm
    mkdir /data/DxDrm/Logs
    mkdir /data/DxDrm/fuse

    chown media system /data/DxDrm
    chown media system /data/DxDrm/Logs
    chown media system /data/DxDrm/fuse

    // We want to enable all the processes to be able to add the trace and log files
    // For release versions you may want to replace 777 by 555, and not to create Logs folder
    chmod 0775 /data/DxDrm
    chmod 0775 /data/DxDrm/Logs

    // We should enable access to IPC service to mounting point when not running as root.
    chmod 0775 /data/DxDrm/fuse

    mkdir /sdcard/download
    // Discretix DRM change end

    insmod /system/lib/modules/test_noncb_oemrapi.ko

    //create the gpsone directory
    mkdir /data/misc/gpsone_d 0775 system system

on boot
// basic network init
    ifup lo
    hostname localhost
    domainname localdomain

// set RLIMIT_NICE to allow priorities from 19 to -20
    setrlimit 13 40 40

// Enable DSDS feature on Android
    setprop persist.dsds.enabled true

// Define the oom_adj values for the classes of processes that can be
// killed by the kernel.  These are used in ActivityManagerService.
    setprop ro.FOREGROUND_APP_ADJ 0
    setprop ro.VISIBLE_APP_ADJ 1
    setprop ro.PERCEPTIBLE_APP_ADJ 2
    setprop ro.HEAVY_WEIGHT_APP_ADJ 3
    setprop ro.SECONDARY_SERVER_ADJ 4
    setprop ro.BACKUP_APP_ADJ 5
    setprop ro.HOME_APP_ADJ 6
    setprop ro.HIDDEN_APP_MIN_ADJ 7
    setprop ro.EMPTY_APP_ADJ 15

// Define the memory thresholds at which the above process classes will
// be killed.  These numbers are in pages (4k).
    setprop ro.FOREGROUND_APP_MEM 2048
    setprop ro.VISIBLE_APP_MEM 3072
    setprop ro.PERCEPTIBLE_APP_MEM 4096
    setprop ro.HEAVY_WEIGHT_APP_MEM 4096
    setprop ro.SECONDARY_SERVER_MEM 6144
    setprop ro.BACKUP_APP_MEM 6144
    setprop ro.HOME_APP_MEM 6144
    setprop ro.HIDDEN_APP_MEM 7168
    setprop ro.EMPTY_APP_MEM 8192

    setprop ro.MAX_HIDDEN_APPS 25

// Write value must be consistent with the above properties.
// Note that the driver only supports 6 slots, so we have combined some of
// the classes into the same memory level; the associated processes of higher
// classes will still be killed first.
    write /sys/module/lowmemorykiller/parameters/adj 0,1,2,4,7,15

    write /proc/sys/vm/overcommit_memory 1
    write /proc/sys/vm/min_free_order_shift 4
    write /sys/module/lowmemorykiller/parameters/minfree 2048,3072,4096,6144,7168,8192

    // Set init its forked children's oom_adj.
    write /proc/1/oom_adj -16

    // Tweak background writeout
    write /proc/sys/vm/dirty_expire_centisecs 200
    write /proc/sys/vm/dirty_background_ratio  5

    // Permissions for System Server and daemons.
    chown radio system /sys/android_power/state
    chown radio system /sys/android_power/request_state
    chown radio system /sys/android_power/acquire_full_wake_lock
    chown radio system /sys/android_power/acquire_partial_wake_lock
    chown radio system /sys/android_power/release_wake_lock
    chown radio system /sys/power/state
    chown radio system /sys/power/wake_lock
    chown radio system /sys/power/wake_unlock
    chmod 0660 /sys/power/state
    chmod 0660 /sys/power/wake_lock
    chmod 0660 /sys/power/wake_unlock
    //baron.hu 20110825
    chmod 0775 /sys/class/usb_composite/diag/enable
    chmod 0775 /sys/devices/virtual/usb_composite/diag/enable
    //end baron.hu
    chmod 0775 /sys/class/usb_composite/usb_mass_storage/enable
    chown system system /sys/class/usb_composite/usb_mass_storage/enable
    chown system system /sys/devices/virtual/usb_composite/usb_mass_storage/enable
    chown system system /sys/class/timed_output/vibrator/enable
    chown system system /sys/class/leds/keyboard-backlight/brightness
    chown system system /sys/class/leds/lcd-backlight/brightness
    chown system system /sys/class/leds/button-backlight/brightness
    chown system system /sys/class/leds/jogball-backlight/brightness
    chown system system /sys/class/leds/red/brightness
    chown system system /sys/class/leds/green/brightness
    chown system system /sys/class/leds/blue/brightness
    chown system system /sys/class/leds/amber/brightness
    chown system system /sys/class/leds/red/blink
    chown system system /sys/class/leds/green/blink
    chown system system /sys/class/leds/blue/blink
    chown system system /sys/class/leds/amber/blink
    chown system system /sys/class/leds/red/device/grpfreq
    chown system system /sys/class/leds/red/device/grppwm
    chown system system /sys/class/leds/red/device/blink
    chown system system /sys/class/timed_output/vibrator/enable
    chown system system /sys/module/sco/parameters/disable_esco
    chown system system /sys/kernel/ipv4/tcp_wmem_min
    chown system system /sys/kernel/ipv4/tcp_wmem_def
    chown system system /sys/kernel/ipv4/tcp_wmem_max
    chown system system /sys/kernel/ipv4/tcp_rmem_min
    chown system system /sys/kernel/ipv4/tcp_rmem_def
    chown system system /sys/kernel/ipv4/tcp_rmem_max
    chown root radio /proc/cmdline

// Define TCP buffer sizes for various networks
//   ReadMin, ReadInitial, ReadMax, WriteMin, WriteInitial, WriteMax,
    setprop net.tcp.buffersize.default 4096,87380,110208,4096,16384,110208
    setprop net.tcp.buffersize.wifi    4095,87380,110208,4096,16384,110208
    setprop net.tcp.buffersize.umts    4094,87380,110208,4096,16384,110208
    setprop net.tcp.buffersize.edge    4093,26280,35040,4096,16384,35040
    setprop net.tcp.buffersize.gprs    4092,8760,11680,4096,8760,11680
    setprop net.tcp.buffersize.lte     4094,87380,1220608,4096,16384,1220608
    setprop net.tcp.buffersize.evdo_b  4094,87380,262144,4096,16384,262144

// Assign TCP buffer thresholds to be ceiling value of technology maximums
// Increased technology maximums should be reflected here.
    write /proc/sys/net/core/rmem_max  1220608
    write /proc/sys/net/core/wmem_max  1220608

// Increase Read Cache for better SD Card Access
    write /sys/devices/virtual/bdi/179:0/read_ahead_kb 2048

    class_start default

//// Daemon processes to be run by init.
////
service ueventd /sbin/ueventd
    critical

service console /system/bin/sh
    console
    disabled
    user shell
    group log

on property:ro.secure=0
    start console

// adbd is controlled by the persist.service.adb.enable system property
service adbd /sbin/adbd
    disabled

// adbd on at boot in emulator
on property:ro.kernel.qemu=1
    start adbd

on property:persist.service.adb.enable=1
    start adbd

on property:persist.service.adb.enable=0
    stop adbd

// This property trigger has added to imitiate the previous behavior of "adb root".
// The adb gadget driver used to reset the USB bus when the adbd daemon exited,
// and the host side adb relied on this behavior to force it to reconnect with the
// new adbd instance after init relaunches it. So now we force the USB bus to reset
// here when adbd sets the service.adb.root property to 1.  We also restart adbd here
// rather than waiting for init to notice its death and restarting it so the timing
// of USB resetting and adb restarting more closely matches the previous behavior.
on property:service.adb.root=1
    write /sys/class/usb_composite/adb/enable 0
    restart adbd
    write /sys/class/usb_composite/adb/enable 1

service servicemanager /system/bin/servicemanager
    user system
    critical
    onrestart restart zygote
    onrestart restart media

service vold /system/bin/vold
    socket vold stream 0660 root mount
    ioprio be 2

service netd /system/bin/netd
    socket netd stream 0660 root system

service debuggerd /system/bin/debuggerd

// service scrstate-scaling /system/bin/sh /system/etc/init.scrstate_scaling.sh

service ril-daemon /system/bin/rild
    socket rild stream 660 root radio
    socket rild-debug stream 660 radio system
    user root
    group radio cache inet misc audio sdcard_rw qcom_oncrpc diag
    disabled

service zygote /system/bin/app_process -Xzygote /system/bin --zygote --start-system-server
    socket zygote stream 666
    onrestart write /sys/android_power/request_state wake
    onrestart write /sys/power/state on
    onrestart restart media
    onrestart restart netd

service media /system/bin/mediaserver
    user media
    group system audio camera graphics inet net_bt net_bt_admin net_raw  qcom_oncrpc
    ioprio rt 4

service bootanim /system/bin/bootanimation
    user graphics
    group graphics
    disabled
    oneshot

service dbus /system/bin/dbus-daemon --system --nofork
    socket dbus stream 660 bluetooth bluetooth
    user bluetooth
    group bluetooth net_bt_admin

service bluetoothd /system/bin/bluetoothd -n
    socket bluetooth stream 660 bluetooth bluetooth
    socket dbus_bluetooth stream 660 bluetooth bluetooth
    // init.rc does not yet support applying capabilities, so run as root and
    // let bluetoothd drop uid to bluetooth with the right linux capabilities
    group bluetooth net_bt_admin misc
    disabled

service hfag /system/bin/sdptool add --channel=10 HFAG
    user bluetooth
    group bluetooth net_bt_admin
    disabled
    oneshot

service hsag /system/bin/sdptool add --channel=11 HSAG
    user bluetooth
    group bluetooth net_bt_admin
    disabled
    oneshot

service opush /system/bin/sdptool add --psm=5255 --channel=12 OPUSH
    user bluetooth
    group bluetooth net_bt_admin
    disabled
    oneshot

service pbap /system/bin/sdptool add --channel=19 PBAP
    user bluetooth
    group bluetooth net_bt_admin
    disabled
    oneshot

service installd /system/bin/installd
    socket installd stream 600 system system

service flash_recovery /system/etc/install-recovery.sh
    oneshot

// Discretix DRM change start
service dx_drm_server /system/bin/DxDrmServerIpc -f -o allow_other /data/DxDrm/fuse
    user media
    group system
    onrestart chown media system /dev/fuse
    onrestart chmod 660 /dev/fuse
// Discretix DRM change end

service racoon /system/bin/racoon
    socket racoon stream 600 system system
    // racoon will setuid to vpn after getting necessary resources.
    group net_admin
    disabled
    oneshot

service mtpd /system/bin/mtpd
    socket mtpd stream 600 system system
    user vpn
    group vpn net_admin net_raw
    disabled
    oneshot

service keystore /system/bin/keystore /data/misc/keystore
    user keystore
    group keystore
    socket keystore stream 666

service dumpstate /system/bin/dumpstate -s
    socket dumpstate stream 0660 shell log
    disabled
    oneshot

service akmd8975 /system/bin/akmd8975
    oneshot

service rwflexnv /system/bin/rwflexnv
    oneshot
    
//the service used to stop receive wifi test
service stop-receive /system/bin/sh /system/etc/wifi/stop-receive.sh
    user root
    group root
    disabled
    oneshot

//the service used to stop send wifi test
service stop-send /system/bin/sh /system/etc/wifi/stop-send.sh
   user root
   group root
   disabled
   oneshot

//the service used to send wifi test
service send /system/bin/sh /data/send.sh
   user root
   group root
   disabled
   oneshot
  
//the service used to receive wifi test
service receive /system/bin/sh /data/receive.sh
   user root
   group root
   disabled
   oneshot

//the service used to insmod bcm4329.ko
service insmod-bcm /system/bin/sh /system/etc/wifi/insmod-bcm.sh
   user root
   group root
   disabled
   oneshot
   
//the service used to BT test
service bt-test /system/bin/sh /system/etc/wifi/enterTestMode.sh
   user root
   group root
   disabled
   oneshot
   
//the service used to counters the packages received
service counters /system/bin/sh /system/etc/wifi/counters.sh
   user root
   group root
   disabled
   oneshot
   
service flexconfigAdd /system/bin/sh /system/etc/flexconfigAdd.sh
   user root
   group root
   disabled
   oneshot
   
service flexconfigDel /system/bin/sh /system/etc/flexconfigDel.sh
   user root
   group root
   disabled
   oneshot  

//service motousbdriver /system/bin/sh /system/etc/motousbdriver.sh
//   user root
//   group root
//   oneshot  
