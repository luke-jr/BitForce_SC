AS = avr32-as
CC = avr32-gcc

OUTDIR = Release
# TODO: UseSectionRoDataForReadOnlyData
CFLAGS = -O3 -ffast-math -mforce-double-align -Wall
# TODO: PutReadOnlyDataInWritableDataSection
LDFLAGS = -lm -Wl,--gc-sections -Wl,--direct-data

PART=uc3a1128

PROGNAME = BitForce_SC
xCFLAGS = -std=gnu99 -mpart=$(PART)
xASPPFLAGS = -x assembler-with-cpp -mpart=$(PART)
xASFLAGS = -mpart=$(PART)
xLDFLAGS = -mpart=$(PART)

DEBUG=1
ifeq ($(DEBUG),1)
	OUTDIR = Debug
	# TODO: GenerateGprofInformation(-pg) GenerateProfInformation(-p)
	CFLAGS = -O2 -funsafe-math-optimizations -ffast-math -fPIC -masm-addr-pseudos -mforce-double-align -g3 -Wall
	LDFLAGS = -lm -Wl,--gc-sections -Wl,--direct-data
	ASFLAGS = -g
endif

SOURCES = \
	AVR32X/AVR32_Module.c \
	AVR32X/exception.S \
	AVR32X/intc.c \
	A2D_Module.c \
	ASIC_Engine.c \
	AVR32_OptimizedTemplates.c \
	ChainProtocol_Module.c \
	FAN_Subsystem.c \
	Generic_Module.c \
	HighLevel_Operations.c \
	HostInteractionProtocols.c \
	JobPipe_Module.c \
	JohnChengASIC.c \
	MCU_Initialization.c \
	Main_BitforceSC.c \
	PipeProcessingKernel.c \
	SHA256_Engine.c \
	USBProtocol_Module.c \
	std_defs.c

OBJECTS = $(addprefix $(OUTDIR)/,$(patsubst %.S,%.o,$(patsubst %.c,%.o,$(SOURCES))))

all: $(OUTDIR)/$(PROGNAME).hex

$(OUTDIR)/$(PROGNAME).hex: $(OUTDIR)/$(PROGNAME).elf

$(OUTDIR)/$(PROGNAME).elf: $(OBJECTS)
	$(LINK.o) $^ $(xLDFLAGS) $(LDFLAGS) -o $@

$(OUTDIR)/%.o: %.c
	mkdir -p "$(dir $@)"
	$(CC) -c $(xCFLAGS) $(CFLAGS) $< -o $@

%.o: %.s
	$(AS) $(xASFLAGS) $(ASFLAGS) $< -o $@

$(OUTDIR)/%.s: %.S
	$(CPP) $(xASPPFLAGS) $(CPPFLAGS) $(ASPPFLAGS) $< > $@

clean:
	rm -f $(OBJECTS) $(OUTDIR)/$(PROGNAME).elf $(OUTDIR)/$(PROGNAME).hex

.PHONY: clean
