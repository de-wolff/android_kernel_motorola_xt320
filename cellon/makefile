# constant variable
build_in_android := no
KERNEL_TARGET_OUT := $(objtree)/cellon

ifeq ("$(origin O)", "command line")
ifneq (, $(findstring target/product,$(O)))
  build_in_android := yes
  KERNEL_TARGET_OUT := $(objtree)/../..
endif
endif

KERNEL_TARGET_ROOT_OUT := $(KERNEL_TARGET_OUT)/root
LOCAL_PATH := $(srctree)/cellon
KERNEL_ALL_PREBUILT := 

# tools 
MV = /bin/mv
CP = /bin/cp
RM = /bin/rm
MKDIR = /bin/mkdir
MINIGZIP = $(LOCAL_PATH)/minigzip
MKBOOTFS = $(LOCAL_PATH)/mkbootfs
MKBOOTIMG = $(LOCAL_PATH)/mkbootimg

# generate init.rc and init.qcom.rc
$(KERNEL_TARGET_ROOT_OUT)/%.rc: $(LOCAL_PATH)/%.rc.c
	@$(kecho) "  Compiling $<"
	@$(MKDIR) -p $(dir $@)
	@$(CPP) -I $(objtree)/include $< | \
		sed -e 's/^[ \t]*//' -e '/^#/d' -e '/^$$/d' -e '/^\(on\|service\) /i\ ' \
                    -e '/^\(on\|service\) /!s/^/    /' > $@

file := $(KERNEL_TARGET_ROOT_OUT)/init.rc \
	$(KERNEL_TARGET_ROOT_OUT)/init.qcom.rc
KERNEL_ALL_PREBUILT += $(file)

# copy default file
copy_from := btftm

copy_to := $(addprefix $(KERNEL_TARGET_ROOT_OUT)/sbin/,$(copy_from))
$(copy_to): $(KERNEL_TARGET_ROOT_OUT)/sbin/%: $(LOCAL_PATH)/%
	@$(kecho) "  Copy: $@"
	@$(MKDIR) -p $(dir $@)
	@$(CP) $< $@
KERNEL_ALL_PREBUILT += $(copy_to)


ifeq ($(build_in_android),yes)

all: $(KERNEL_ALL_PREBUILT)

else

# copy default file
copy_from := init.goldfish.rc \
             init.target.rc \
             default.prop \
             ueventd.goldfish.rc \
             ueventd.rc \
             init.qcom.sh \
             init

copy_to := $(addprefix $(KERNEL_TARGET_ROOT_OUT)/,$(copy_from))
$(copy_to): $(KERNEL_TARGET_ROOT_OUT)/%: $(LOCAL_PATH)/%
	@$(kecho) "  Copy: $@"
	@$(CP) $< $@
KERNEL_ALL_PREBUILT += $(copy_to)


copy_from := adbd

copy_to := $(addprefix $(KERNEL_TARGET_ROOT_OUT)/sbin/,$(copy_from))
$(copy_to): $(KERNEL_TARGET_ROOT_OUT)/sbin/%: $(LOCAL_PATH)/%
	@$(kecho) "  Copy: $@"
	@$(MKDIR) -p $(dir $@)
	@$(CP) $< $@
KERNEL_ALL_PREBUILT += $(copy_to)

# make a symlink from /sbin/ueventd to /init
ueventd_symlink := $(KERNEL_TARGET_ROOT_OUT)/sbin/ueventd

$(ueventd_symlink): $(KERNEL_TARGET_ROOT_OUT)/init 
	@$(kecho) "  Symlink: $@ -> ../init"
	@$(MKDIR) -p $(dir $@)
	@$(RM) -rf $@
	@ln -sf ../init $@
KERNEL_ALL_PREBUILT += $(ueventd_symlink)

# create some directories(some are mount points)
DIRS := $(addprefix $(KERNEL_TARGET_ROOT_OUT)/, \
		sbin \
		dev \
		proc \
		sys \
		system \
		data \
	)

$(DIRS):
	@$(kecho) "  Directory: $@"
	@$(MKDIR) -p $@
KERNEL_ALL_PREBUILT += $(DIRS)

 
INSTALL_MOD_PATH := $(KERNEL_TARGET_OUT)/system
KERNEL_MODULES_OUT := $(KERNEL_TARGET_OUT)/system/lib/modules

define mv-modules
mdpath=`find $(KERNEL_MODULES_OUT) -type f -name modules.dep`;\
if [ "$$mdpath" != "" ];then\
mpath=`dirname $$mdpath`;\
ko=`find $$mpath/kernel -type f -name *.ko`;\
for i in $$ko; do mv $$i $(KERNEL_MODULES_OUT)/; done;\
fi
endef

define clean-module-folder
mdpath=`find $(KERNEL_MODULES_OUT) -type f -name modules.dep`;\
if [ "$$mdpath" != "" ];then\
mpath=`dirname $$mdpath`; rm -rf $$mpath;\
fi
endef

BOOTIMG = $(KERNEL_TARGET_OUT)/boot.img
RAMDISKIMG = $(KERNEL_TARGET_OUT)/ramdisk.img

all: $(KERNEL_ALL_PREBUILT) modules_install
	@$(kecho) "  Copying $(KBUILD_IMAGE)"
	@$(CP) $(boot)/$(KBUILD_IMAGE) $(KERNEL_TARGET_OUT)/$(KBUILD_IMAGE)

	@$(kecho) "  Building ramdisk.img"
	$(Q)$(MKBOOTFS) $(KERNEL_TARGET_ROOT_OUT) | $(MINIGZIP) > $(RAMDISKIMG)

	@$(kecho) "  Building boot.img"
	$(Q)$(MKBOOTIMG) --kernel $(objtree)/cellon/$(KBUILD_IMAGE) \
	                 --ramdisk $(RAMDISKIMG) \
	                 --cmdline "androidboot.hardware=qcom loglevel=1" \
	                 --base 0x00200000 --pagesize 2048 \
	                 --output $(BOOTIMG)

	$(mv-modules)
	$(clean-module-folder)

endif
