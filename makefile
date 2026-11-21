TARGET := bluetooth2.kmod
BINDIR := bin
OBJDIR := obj

ifeq ($(shell uname),AmigaOS)
CC := gcc
AR := ar
else
CC := ppc-amigaos-gcc
AR := ppc-amigaos-ar
endif

CPPFLAGS += -Iinc_Public -Iinc_Private -IThirdParty/libsbc
CFLAGS += -O2 -Wall -Wextra -ffreestanding -MMD -MP
LDFLAGS += -nostdlib -Wl,-e,_start

HCI_SOURCES := $(sort $(wildcard Stack/HCI/*.c))
HCI_OBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(HCI_SOURCES))
BOND_SOURCES := $(sort $(wildcard Stack/Bond/*.c))
BOND_OBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(BOND_SOURCES))
L2CAP_SOURCES := $(sort $(wildcard Stack/L2CAP/*.c))
L2CAP_OBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(L2CAP_SOURCES))
SDP_SOURCES := $(sort $(wildcard Stack/SDP/*.c))
SDP_OBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(SDP_SOURCES))
RFCOMM_SOURCES := $(sort $(wildcard Stack/RFCOMM/*.c))
RFCOMM_OBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(RFCOMM_SOURCES))
OBEX_SOURCES := $(sort $(wildcard Stack/OBEX/*.c))
OBEX_OBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(OBEX_SOURCES))
A2DP_SOURCES := $(sort $(wildcard Stack/A2DP/*.c))
A2DP_OBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(A2DP_SOURCES)) $(OBJDIR)/ThirdParty/libsbc/bits.o $(OBJDIR)/ThirdParty/libsbc/sbc.o
$(OBJDIR)/ThirdParty/libsbc/sbc.o: CPPFLAGS += -Dmemset=BT2_SBCMemset
MASTER_SOURCES := $(sort $(wildcard Stack/Master/*.c))
MASTER_OBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(MASTER_SOURCES))
OBJECTS := $(HCI_OBJECTS) $(BOND_OBJECTS) $(L2CAP_OBJECTS) $(SDP_OBJECTS) $(RFCOMM_OBJECTS) $(OBEX_OBJECTS) $(A2DP_OBJECTS) $(MASTER_OBJECTS) $(OBJDIR)/Stack/Device/Device.o $(OBJDIR)/Stack/Device/Fullboot.o

.PHONY: all archive clean

all: $(BINDIR)/$(TARGET)

archive: $(BINDIR)/libbt2.a

$(BINDIR)/$(TARGET): $(OBJECTS) makefile
	mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

$(BINDIR)/libbt2.a: $(HCI_OBJECTS) $(L2CAP_OBJECTS) $(SDP_OBJECTS) $(RFCOMM_OBJECTS) $(OBEX_OBJECTS) $(A2DP_OBJECTS) makefile
	mkdir -p $(dir $@)
	$(AR) rcs $@ $(HCI_OBJECTS) $(L2CAP_OBJECTS) $(SDP_OBJECTS) $(RFCOMM_OBJECTS) $(OBEX_OBJECTS) $(A2DP_OBJECTS)

$(OBJDIR)/%.o: %.c makefile
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(OBJECTS:.o=.d) $(BINDIR)/$(TARGET) $(BINDIR)/libbt2.a

-include $(OBJECTS:.o=.d)
