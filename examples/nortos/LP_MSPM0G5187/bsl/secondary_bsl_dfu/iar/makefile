MSPM0_SDK_INSTALL_DIR ?= $(abspath ../../../../../..)

include $(MSPM0_SDK_INSTALL_DIR)/imports.mak

ASM = "$(IAR_ARMCOMPILER)/bin/iasmarm"
CC = "$(IAR_ARMCOMPILER)/bin/iccarm"
LNK = "$(IAR_ARMCOMPILER)/bin/ilinkarm"

# Enable verbose output by setting VERBOSE=1
V := @
ifeq ($(VERBOSE), 1)
  V :=
endif

OBJECTS = secondary_bsl.obj dfu_app.obj third_party_tinyusb_src_portable_mentor_musb_dcd_musb.obj third_party_tinyusb_src_portable_mentor_musb_hcd_musb.obj third_party_tinyusb_hw_bsp_board.obj third_party_tinyusb_hw_bsp_mspm0_family.obj third_party_tinyusb_src_class_audio_audio_device.obj third_party_tinyusb_src_class_bth_bth_device.obj third_party_tinyusb_src_class_cdc_cdc_device.obj third_party_tinyusb_src_class_cdc_cdc_host.obj third_party_tinyusb_src_class_cdc_cdc_rndis_host.obj third_party_tinyusb_src_class_dfu_dfu_device.obj third_party_tinyusb_src_class_dfu_dfu_rt_device.obj third_party_tinyusb_src_class_hid_hid_device.obj third_party_tinyusb_src_class_hid_hid_host.obj third_party_tinyusb_src_class_midi_midi_device.obj third_party_tinyusb_src_class_msc_msc_device.obj third_party_tinyusb_src_class_msc_msc_host.obj third_party_tinyusb_src_class_net_ecm_rndis_device.obj third_party_tinyusb_src_class_net_ncm_device.obj third_party_tinyusb_src_class_usbtmc_usbtmc_device.obj third_party_tinyusb_src_class_vendor_vendor_device.obj third_party_tinyusb_src_class_vendor_vendor_host.obj third_party_tinyusb_src_class_video_video_device.obj third_party_tinyusb_src_common_tusb_fifo.obj third_party_tinyusb_src_device_usbd.obj third_party_tinyusb_src_device_usbd_control.obj third_party_tinyusb_src_host_hub.obj third_party_tinyusb_src_host_usbh.obj third_party_tinyusb_src_tusb.obj iar_startup_mspm0g518x_iar.obj

NAME = secondary_bsl_dfu

AFLAGS += -I.. \
    -S \
    --endian little \
    --thumb \
    "-I$(IAR_ARMCOMPILER)/inc/c" \
    --cpu Cortex-M0+

DFLAGS += --debug

CFLAGS += -I.. \
    -D__MSPM0G5187__ \
    -D__USE_SYSCONFIG__ \
    --diag_suppress=Pa039 \
    --diag_suppress=Pe1143 \
    -D__MSPM0G518X__ \
    -DCFG_TUSB_MCU=OPT_MCU_MSPM0G518X \
    "-I$(MSPM0_SDK_INSTALL_DIR)/source/third_party/CMSIS/Core/Include" \
    -I../third_party/tinyusb/src \
    -I../third_party/tinyusb/hw \
    -I../third_party/tinyusb/lib/fatfs/source \
    -I../third_party/tinyusb/lib/embedded-cli \
    -I../third_party/tinyusb/hw/bsp/mspm0 \
    -I$MSPM0_SDK_INSTALL_DIR$$/source \
    -I$MSPM0_SDK_INSTALL_DIR$/source/ti \
    -I$MSPM0_SDK_INSTALL_DIR$/source/ti/devices/msp \
    "-I$(MSPM0_SDK_INSTALL_DIR)/source" \
    --silent \
    -e \
    --aeabi \
    --thumb \
    --diag_suppress=Pa050 \
    --cpu=Cortex-M0+ \
    --vla


ifneq ($(CONFIG),RELEASE)
# Add debug flags by default (excluded only when CONFIG=RELEASE)
CFLAGS += $(DFLAGS)
endif

LFLAGS += -L${CG_TOOL_ROOT}/arm-none-eabi/lib/thumb/v6-m/nofp \
    "$(MSPM0_SDK_INSTALL_DIR)/source/ti/driverlib/lib/iar/m0p/mspm0g518x/driverlib.a" \
    "-L$(MSPM0_SDK_INSTALL_DIR)/source" \
    -L.. \
    --config ../iar/mspm0g5187.icf \
    --map "$(NAME).map" \
    --silent \
    "$(if $(filter RELEASE,$(CONFIG)),,--semihosting=iar_breakpoint)" \
    --cpu=Cortex-M0+

all: $(NAME).out

secondary_bsl.obj: ../secondary_bsl.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

dfu_app.obj: ../dfu_app.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_portable_mentor_musb_dcd_musb.obj: ../third_party/tinyusb/src/portable/mentor/musb/dcd_musb.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_portable_mentor_musb_hcd_musb.obj: ../third_party/tinyusb/src/portable/mentor/musb/hcd_musb.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_hw_bsp_board.obj: ../third_party/tinyusb/hw/bsp/board.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_hw_bsp_mspm0_family.obj: ../third_party/tinyusb/hw/bsp/mspm0/family.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_class_audio_audio_device.obj: ../third_party/tinyusb/src/class/audio/audio_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_class_bth_bth_device.obj: ../third_party/tinyusb/src/class/bth/bth_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_class_cdc_cdc_device.obj: ../third_party/tinyusb/src/class/cdc/cdc_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_class_cdc_cdc_host.obj: ../third_party/tinyusb/src/class/cdc/cdc_host.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_class_cdc_cdc_rndis_host.obj: ../third_party/tinyusb/src/class/cdc/cdc_rndis_host.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_class_dfu_dfu_device.obj: ../third_party/tinyusb/src/class/dfu/dfu_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_class_dfu_dfu_rt_device.obj: ../third_party/tinyusb/src/class/dfu/dfu_rt_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_class_hid_hid_device.obj: ../third_party/tinyusb/src/class/hid/hid_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_class_hid_hid_host.obj: ../third_party/tinyusb/src/class/hid/hid_host.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_class_midi_midi_device.obj: ../third_party/tinyusb/src/class/midi/midi_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_class_msc_msc_device.obj: ../third_party/tinyusb/src/class/msc/msc_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_class_msc_msc_host.obj: ../third_party/tinyusb/src/class/msc/msc_host.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_class_net_ecm_rndis_device.obj: ../third_party/tinyusb/src/class/net/ecm_rndis_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_class_net_ncm_device.obj: ../third_party/tinyusb/src/class/net/ncm_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_class_usbtmc_usbtmc_device.obj: ../third_party/tinyusb/src/class/usbtmc/usbtmc_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_class_vendor_vendor_device.obj: ../third_party/tinyusb/src/class/vendor/vendor_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_class_vendor_vendor_host.obj: ../third_party/tinyusb/src/class/vendor/vendor_host.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_class_video_video_device.obj: ../third_party/tinyusb/src/class/video/video_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_common_tusb_fifo.obj: ../third_party/tinyusb/src/common/tusb_fifo.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_device_usbd.obj: ../third_party/tinyusb/src/device/usbd.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_device_usbd_control.obj: ../third_party/tinyusb/src/device/usbd_control.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_host_hub.obj: ../third_party/tinyusb/src/host/hub.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_host_usbh.obj: ../third_party/tinyusb/src/host/usbh.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

third_party_tinyusb_src_tusb.obj: ../third_party/tinyusb/src/tusb.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

iar_startup_mspm0g518x_iar.obj: ../iar/startup_mspm0g518x_iar.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -o $@

$(NAME).out: $(OBJECTS)
	@ echo linking $@
	$(V) $(LNK)  $(OBJECTS)  $(LFLAGS) -o $(NAME).out

clean:
	@ echo Cleaning...
	$(V) $(RM) $(OBJECTS) > $(DEVNULL) 2>&1
	$(V) $(RM) $(NAME).out > $(DEVNULL) 2>&1
	$(V) $(RM) $(NAME).map > $(DEVNULL) 2>&1
