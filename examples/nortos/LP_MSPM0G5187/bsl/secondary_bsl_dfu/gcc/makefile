MSPM0_SDK_INSTALL_DIR ?= $(abspath ../../../../../..)

include $(MSPM0_SDK_INSTALL_DIR)/imports.mak

CC = "$(GCC_ARMCOMPILER)/bin/arm-none-eabi-gcc"
LNK = "$(GCC_ARMCOMPILER)/bin/arm-none-eabi-gcc"

# Enable verbose output by setting VERBOSE=1
V := @
ifeq ($(VERBOSE), 1)
  V :=
endif

OBJECTS = secondary_bsl.obj dfu_app.obj third_party_tinyusb_src_portable_mentor_musb_dcd_musb.obj third_party_tinyusb_src_portable_mentor_musb_hcd_musb.obj third_party_tinyusb_hw_bsp_board.obj third_party_tinyusb_hw_bsp_mspm0_family.obj third_party_tinyusb_src_class_audio_audio_device.obj third_party_tinyusb_src_class_bth_bth_device.obj third_party_tinyusb_src_class_cdc_cdc_device.obj third_party_tinyusb_src_class_cdc_cdc_host.obj third_party_tinyusb_src_class_cdc_cdc_rndis_host.obj third_party_tinyusb_src_class_dfu_dfu_device.obj third_party_tinyusb_src_class_dfu_dfu_rt_device.obj third_party_tinyusb_src_class_hid_hid_device.obj third_party_tinyusb_src_class_hid_hid_host.obj third_party_tinyusb_src_class_midi_midi_device.obj third_party_tinyusb_src_class_msc_msc_device.obj third_party_tinyusb_src_class_msc_msc_host.obj third_party_tinyusb_src_class_net_ecm_rndis_device.obj third_party_tinyusb_src_class_net_ncm_device.obj third_party_tinyusb_src_class_usbtmc_usbtmc_device.obj third_party_tinyusb_src_class_vendor_vendor_device.obj third_party_tinyusb_src_class_vendor_vendor_host.obj third_party_tinyusb_src_class_video_video_device.obj third_party_tinyusb_src_common_tusb_fifo.obj third_party_tinyusb_src_device_usbd.obj third_party_tinyusb_src_device_usbd_control.obj third_party_tinyusb_src_host_hub.obj third_party_tinyusb_src_host_usbh.obj third_party_tinyusb_src_tusb.obj gcc_startup_mspm0g518x_gcc.obj

NAME = secondary_bsl_dfu


DFLAGS += -gstrict-dwarf

CFLAGS += -I.. \
    -D__MSPM0G5187__ \
    -g \
    -mthumb \
    -std=c99 \
    -mcpu=cortex-m0plus \
    -mfloat-abi=soft \
    -fsingle-precision-constant \
    -fno-strict-aliasing \
    -MD \
    -ggdb \
    -fdata-sections \
    -ffunction-sections \
    -Wextra \
    -Wfatal-errors \
    -Wdouble-promotion \
    -Wstrict-prototypes \
    -Wstrict-overflow \
    -Werror-implicit-function-declaration \
    -Wfloat-equal \
    -Wshadow \
    -Wwrite-strings \
    -Wsign-compare \
    -Wmissing-format-attribute \
    -Wunreachable-code \
    -Wcast-function-type \
    -Wnull-dereference \
    -Wuninitialized \
    -Wreturn-type \
    -Wredundant-decls \
    -O0 \
    -Wno-error=format= \
    "-I$(MSPM0_SDK_INSTALL_DIR)/source/third_party/CMSIS/Core/Include" \
    -I../third_party/tinyusb/src \
    -I../third_party/tinyusb/hw \
    -I../third_party/tinyusb/lib/fatfs/source \
    -I../third_party/tinyusb/lib/embedded-cli \
    -I../third_party/tinyusb/hw/bsp/mspm0 \
    -I${COM_TI_MSPM0_SDK_INSTALL_DIR}/source \
    -I${COM_TI_MSPM0_SDK_INSTALL_DIR}/source/ti \
    -I${COM_TI_MSPM0_SDK_INSTALL_DIR}/source/ti/devices/msp \
    "-I$(MSPM0_SDK_INSTALL_DIR)/source" \
    -march=armv6-m \
    -Wall \
    "-I$(GCC_ARMCOMPILER)/arm-none-eabi/include/newlib-nano" \
    "-I$(GCC_ARMCOMPILER)/arm-none-eabi/include"


ifneq ($(CONFIG),RELEASE)
# Add debug flags by default (excluded only when CONFIG=RELEASE)
CFLAGS += $(DFLAGS)
endif

LFLAGS += "-L$(MSPM0_SDK_INSTALL_DIR)/source/ti/driverlib/lib/gcc/m0p/mspm0g518x" \
    -Wl,-Map=.map \
    -Wl,--cref \
    -Wl,-gc-sections \
    -Wl,--print-memory-usage \
    -g \
    -mthumb \
    -mcpu=cortex-m0plus \
    -mfloat-abi=soft \
    -fsingle-precision-constant \
    -MD \
    -ggdb \
    -fdata-sections \
    -ffunction-sections \
    -fno-strict-aliasing \
    -Wextra \
    -Wfatal-errors \
    -Wdouble-promotion \
    -Wstrict-prototypes \
    -Wstrict-overflow \
    -Werror-implicit-function-declaration \
    -Wfloat-equal \
    -Wshadow \
    -Wwrite-strings \
    -Wsign-compare \
    -Wmissing-format-attribute \
    -Wunreachable-code \
    -Wcast-function-type \
    -Wnull-dereference \
    -Wuninitialized \
    -Wreturn-type \
    -Wredundant-decls \
    -O0 \
    -Wno-error=format= \
    --specs=nosys.specs \
    --specs=nano.specs \
    -nostartfiles \
    -Wl,--start-group \
    -l:driverlib.a \
    -lgcc \
    -lm \
    -lnosys \
    -lc \
    -Wl,--end-group \
    "-L$(GCC_ARMCOMPILER)/arm-none-eabi/lib/thumb/v6-m/nofp" \
    -Wl,-T,../gcc/mspm0g5187.lds \
    "-Wl,-Map,$(NAME).map" \
    "-L$(MSPM0_SDK_INSTALL_DIR)/source" \
    -L.. \
    -march=armv6-m \
    -static \
    -Wl,--gc-sections

all: $(NAME).out

secondary_bsl.obj: ../secondary_bsl.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

dfu_app.obj: ../dfu_app.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_portable_mentor_musb_dcd_musb.obj: ../third_party/tinyusb/src/portable/mentor/musb/dcd_musb.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_portable_mentor_musb_hcd_musb.obj: ../third_party/tinyusb/src/portable/mentor/musb/hcd_musb.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_hw_bsp_board.obj: ../third_party/tinyusb/hw/bsp/board.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_hw_bsp_mspm0_family.obj: ../third_party/tinyusb/hw/bsp/mspm0/family.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_class_audio_audio_device.obj: ../third_party/tinyusb/src/class/audio/audio_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_class_bth_bth_device.obj: ../third_party/tinyusb/src/class/bth/bth_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_class_cdc_cdc_device.obj: ../third_party/tinyusb/src/class/cdc/cdc_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_class_cdc_cdc_host.obj: ../third_party/tinyusb/src/class/cdc/cdc_host.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_class_cdc_cdc_rndis_host.obj: ../third_party/tinyusb/src/class/cdc/cdc_rndis_host.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_class_dfu_dfu_device.obj: ../third_party/tinyusb/src/class/dfu/dfu_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_class_dfu_dfu_rt_device.obj: ../third_party/tinyusb/src/class/dfu/dfu_rt_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_class_hid_hid_device.obj: ../third_party/tinyusb/src/class/hid/hid_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_class_hid_hid_host.obj: ../third_party/tinyusb/src/class/hid/hid_host.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_class_midi_midi_device.obj: ../third_party/tinyusb/src/class/midi/midi_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_class_msc_msc_device.obj: ../third_party/tinyusb/src/class/msc/msc_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_class_msc_msc_host.obj: ../third_party/tinyusb/src/class/msc/msc_host.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_class_net_ecm_rndis_device.obj: ../third_party/tinyusb/src/class/net/ecm_rndis_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_class_net_ncm_device.obj: ../third_party/tinyusb/src/class/net/ncm_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_class_usbtmc_usbtmc_device.obj: ../third_party/tinyusb/src/class/usbtmc/usbtmc_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_class_vendor_vendor_device.obj: ../third_party/tinyusb/src/class/vendor/vendor_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_class_vendor_vendor_host.obj: ../third_party/tinyusb/src/class/vendor/vendor_host.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_class_video_video_device.obj: ../third_party/tinyusb/src/class/video/video_device.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_common_tusb_fifo.obj: ../third_party/tinyusb/src/common/tusb_fifo.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_device_usbd.obj: ../third_party/tinyusb/src/device/usbd.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_device_usbd_control.obj: ../third_party/tinyusb/src/device/usbd_control.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_host_hub.obj: ../third_party/tinyusb/src/host/hub.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_host_usbh.obj: ../third_party/tinyusb/src/host/usbh.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

third_party_tinyusb_src_tusb.obj: ../third_party/tinyusb/src/tusb.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

gcc_startup_mspm0g518x_gcc.obj: ../gcc/startup_mspm0g518x_gcc.c
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) $< -c -o $@

$(NAME).out: $(OBJECTS)
	@ echo linking $@
	$(V) $(LNK)  $(OBJECTS)  $(LFLAGS) -o $(NAME).out

clean:
	@ echo Cleaning...
	$(V) $(RM) $(OBJECTS) > $(DEVNULL) 2>&1
	$(V) $(RM) $(NAME).out > $(DEVNULL) 2>&1
	$(V) $(RM) $(NAME).map > $(DEVNULL) 2>&1
