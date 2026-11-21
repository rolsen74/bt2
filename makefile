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

CPPFLAGS += -Iinc_Public -Iinc_Private
CFLAGS += -O2 -Wall -Wextra -ffreestanding -MMD -MP
LDFLAGS += -nostdlib -Wl,-e,_start

HCI_SOURCES := $(sort $(wildcard Stack/HCI/*.c))
HCI_OBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(HCI_SOURCES))
BOND_SOURCES := $(sort $(wildcard Stack/Bond/*.c))
BOND_OBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(BOND_SOURCES))
MASTER_SOURCES := $(sort $(wildcard Stack/Master/*.c))
MASTER_OBJECTS := $(patsubst %.c,$(OBJDIR)/%.o,$(MASTER_SOURCES))
OBJECTS := $(HCI_OBJECTS) $(BOND_OBJECTS) $(MASTER_OBJECTS) $(OBJDIR)/Stack/Device/Device.o $(OBJDIR)/Stack/Device/Fullboot.o

.PHONY: all archive clean

all: $(BINDIR)/$(TARGET)

archive: $(BINDIR)/libbt2.a

$(BINDIR)/$(TARGET): $(OBJECTS) makefile
	mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $(OBJECTS)

$(BINDIR)/libbt2.a: $(HCI_OBJECTS) makefile
	mkdir -p $(dir $@)
	$(AR) rcs $@ $(HCI_OBJECTS)

$(OBJDIR)/%.o: %.c makefile
	mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(OBJECTS:.o=.d) $(BINDIR)/$(TARGET) $(BINDIR)/libbt2.a

-include $(OBJECTS:.o=.d)
