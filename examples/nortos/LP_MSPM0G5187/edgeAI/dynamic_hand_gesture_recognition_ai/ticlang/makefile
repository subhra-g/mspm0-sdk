MSPM0_SDK_INSTALL_DIR ?= $(abspath ../../../../../..)

include $(MSPM0_SDK_INSTALL_DIR)/imports.mak

CC = "$(TICLANG_ARMCOMPILER)/bin/tiarmclang"
LNK = "$(TICLANG_ARMCOMPILER)/bin/tiarmclang"

SYSCONFIG_GUI_TOOL = $(dir $(SYSCONFIG_TOOL))sysconfig_gui$(suffix $(SYSCONFIG_TOOL))
SYSCFG_CMD_STUB = $(SYSCONFIG_TOOL) --compiler ticlang --product $(MSPM0_SDK_INSTALL_DIR)/.metadata/product.json
SYSCFG_GUI_CMD_STUB = $(SYSCONFIG_GUI_TOOL) --compiler ticlang --product $(MSPM0_SDK_INSTALL_DIR)/.metadata/product.json
SYSCFG_FILES := $(shell $(SYSCFG_CMD_STUB) --listGeneratedFiles --listReferencedFiles --output . ../dynamic_hand_gesture_recognition_ai.syscfg)

SYSCFG_C_FILES = $(filter %.c,$(SYSCFG_FILES))
SYSCFG_H_FILES = $(filter %.h,$(SYSCFG_FILES))
SYSCFG_OPT_FILES = $(filter %.opt,$(SYSCFG_FILES))

# Enable verbose output by setting VERBOSE=1
V := @
ifeq ($(VERBOSE), 1)
  V :=
endif

OBJECTS = dynamic_hand_gesture_recognition_ai.obj comm_spi_comm.obj sensors_sensor.obj sensors_imu_sensor_BMI270.obj $(patsubst %.c,%.obj,$(notdir $(SYSCFG_C_FILES)))

NAME = dynamic_hand_gesture_recognition_ai


DFLAGS += -gdwarf-3

CFLAGS += -I.. \
    -I. \
    $(addprefix @,$(SYSCFG_OPT_FILES)) \
    -DDEVICE_NAME=MSPM0G5187 \
    -DARM_MATH_CM0 \
    @device.opt \
    "-I$(MSPM0_SDK_INSTALL_DIR)/source/third_party/CMSIS/Core/Include" \
    "-I$(MSPM0_SDK_INSTALL_DIR)/source/third_party/CMSIS/DSP/Include" \
    -Iexamples/edgeAI/dynamic_hand_gesture_recognition_ai/comm \
    -Iexamples/edgeAI/dynamic_hand_gesture_recognition_ai/sensors \
    -Iexamples/edgeAI/dynamic_hand_gesture_recognition_ai/imu \
    -I../comm \
    -I../sensors \
    -I../sensors/imu \
    "-I$(MSPM0_SDK_INSTALL_DIR)/source/ti/edgeAI/feature_extract" \
    "-I$(MSPM0_SDK_INSTALL_DIR)/source" \
    -mcpu=cortex-m0plus \
    -march=thumbv6m \
    -mfloat-abi=soft \
    -mthumb \
    -Wall


ifneq ($(CONFIG),RELEASE)
# Add debug flags by default (excluded only when CONFIG=RELEASE)
CFLAGS += $(DFLAGS)
endif

LFLAGS += -lmodel/model.a \
    -ldevice.cmd.genlibs \
    "-L$(MSPM0_SDK_INSTALL_DIR)/source" \
    -L.. \
    ../ticlang/device_linker.cmd \
    "-Wl,-m,$(NAME).map" \
    -Wl,--rom_model \
    -Wl,--warn_sections \
    "-L$(TICLANG_ARMCOMPILER)/lib" \
    -llibc.a

all: $(NAME).out

.INTERMEDIATE: syscfg
$(SYSCFG_FILES): syscfg
	@ echo generation complete

syscfg: ../dynamic_hand_gesture_recognition_ai.syscfg
	@ echo Generating configuration files...
	$(V) $(SYSCFG_CMD_STUB) --output $(@D) $<


# Helpful hint that the user needs to use a standalone SysConfig installation
$(SYSCONFIG_GUI_TOOL):
	$(error $(dir $(SYSCONFIG_TOOL)) does not contain the GUI framework \
        necessary to launch the SysConfig GUI.  Please set SYSCONFIG_TOOL \
        (in your SDK's imports.mak) to a standalone SysConfig installation \
        rather than one inside CCS)

syscfg-gui: ../dynamic_hand_gesture_recognition_ai.syscfg $(SYSCONFIG_GUI_TOOL)
	@ echo Opening SysConfig GUI
	$(V) $(SYSCFG_GUI_CMD_STUB) $<


define C_RULE
$(basename $(notdir $(1))).obj: $(1) $(SYSCFG_H_FILES)
	@ echo Building $$@
	$(V) $(CC) $(CFLAGS) -c $$< -o $$@
endef

$(foreach c_file,$(SYSCFG_C_FILES),$(eval $(call C_RULE,$(c_file))))

dynamic_hand_gesture_recognition_ai.obj: ../dynamic_hand_gesture_recognition_ai.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

comm_spi_comm.obj: ../comm/spi_comm.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

sensors_sensor.obj: ../sensors/sensor.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

sensors_imu_sensor_BMI270.obj: ../sensors/imu/sensor_BMI270.c $(SYSCFG_H_FILES)
	@ echo Building $@
	$(V) $(CC) $(CFLAGS) -c $< -o $@

$(NAME).out: $(OBJECTS)
	@ echo linking $@
	$(V) $(LNK) -Wl,-u,_c_int00 $(OBJECTS)  $(LFLAGS) -o $(NAME).out

clean:
	@ echo Cleaning...
	$(V) $(RM) $(OBJECTS) > $(DEVNULL) 2>&1
	$(V) $(RM) $(NAME).out > $(DEVNULL) 2>&1
	$(V) $(RM) $(NAME).map > $(DEVNULL) 2>&1
	$(V) $(RM) $(call SLASH_FIXUP,$(SYSCFG_FILES)) > $(DEVNULL) 2>&1
