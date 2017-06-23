#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

#---------------------------------------------------------------------------------

APP_TITLE	:=	SNES9x for 3DS
APP_DESCRIPTION	:= SNES emulator for 3DS. Based on SNES9x 1.43.
APP_AUTHOR	:=	bubble2k16
VERSION_PARTS := $(subst ., ,$(shell git describe --tags --abbrev=0))
VERSION_MAJOR := $(word 1, $(VERSION_PARTS))
VERSION_MINOR := $(word 2, $(VERSION_PARTS))
VERSION_MICRO := $(word 3, $(VERSION_PARTS))

ifeq ($(strip $(VERSION_MAJOR)),)
    VERSION_MAJOR := 0
endif

ifeq ($(strip $(VERSION_MINOR)),)
    VERSION_MINOR := 0
endif

ifeq ($(strip $(VERSION_MICRO)),)
    VERSION_MICRO := 0
endif

ASSETS		:=	$(CURDIR)/assets
ICON		:=	$(ASSETS)/icon.png
NO_SMDH     :=  1
TARGET		:=	$(notdir $(CURDIR))
BUILD		:=	build
SOURCES		:=	source
ROMFS		:=	$(CURDIR)/romfs

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------


ARCH	:=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS	:=	-g -w -O3 -mword-relocations -finline-limit=20000 \
			-fomit-frame-pointer -ffunction-sections \
			$(ARCH)

CFLAGS	+=	$(INCLUDE) -DARM11 -D_3DS

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS	:= -lcitro3d -lctru -lm

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(CTRULIB)


#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUTDIR	:=	$(CURDIR)/out
export OUTPUT	:=	$(OUTPUTDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

#CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
#CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
CFILES		:=
CPPFILES	:=	3dsmain.cpp 3dsmenu.cpp 3dsopt.cpp \
			3dsgpu.cpp 3dssound.cpp 3dsui.cpp 3dsexit.cpp \
			3dsconfig.cpp 3dsfiles.cpp 3dsinput.cpp 3dsmatrix.cpp \
			3dsimpl.cpp 3dsimpl_tilecache.cpp 3dsimpl_gpu.cpp \
			gpulib.cpp lodepng.cpp \
			Snes9x/bsx.cpp Snes9x/fxinst.cpp Snes9x/fxemu.cpp Snes9x/fxdbg.cpp \
			Snes9x/c4.cpp Snes9x/c4emu.cpp \
			Snes9x/soundux.cpp Snes9x/spc700.cpp Snes9x/apu.cpp \
			Snes9x/cpuexec.cpp Snes9x/sa1cpu.cpp Snes9x/hwregisters.cpp \
			Snes9x/cheats.cpp Snes9x/cheats2.cpp \
			Snes9x/sdd1emu.cpp \
			Snes9x/spc7110.cpp \
			Snes9x/obc1.cpp \
			Snes9x/seta.cpp Snes9x/seta010.cpp Snes9x/seta011.cpp Snes9x/seta018.cpp \
			Snes9x/snapshot.cpp Snes9x/screenshot.cpp \
			Snes9x/cpu.cpp Snes9x/sa1.cpp Snes9x/debug.cpp Snes9x/apudebug.cpp Snes9x/sdd1.cpp Snes9x/tile.cpp Snes9x/srtc.cpp \
			Snes9x/gfx.cpp Snes9x/gfxhw.cpp Snes9x/memmap.cpp Snes9x/clip.cpp Snes9x/cliphw.cpp \
			Snes9x/dsp1.cpp Snes9x/ppu.cpp Snes9x/ppuvsect.cpp Snes9x/dma.cpp Snes9x/snes9x.cpp Snes9x/data.cpp Snes9x/globals.cpp \
			

SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PICAFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.v.pica)))
SHLISTFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.shlist)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
			$(PICAFILES:.v.pica=.shbin.o) \
			$(SHLISTFILES:.shlist=.shbin.o) \
			$(CPPFILES:.cpp=.o) \
			$(CFILES:.c=.o) \
			$(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD) \
			-I$(CURDIR)/$(BUILD)/Snes9x \
			-I$(CURDIR)/$(SOURCES) \
			-I$(CURDIR)/$(SOURCES)/unzip \
			-I$(CURDIR)/$(SOURCES)/Snes9x \

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.png)
	ifneq (,$(findstring $(TARGET).png,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).png
	else
		ifneq (,$(findstring icon.png,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.png
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_SMDH)),)
	export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh
endif

ifneq ($(ROMFS),)
	export _3DSXFLAGS += --romfs=$(CURDIR)/$(ROMFS)
endif

#---------------------------------------------------------------------------------
# OS detection to automatically determine the correct makerom variant to use for
# CIA creation
#---------------------------------------------------------------------------------
UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)
MAKEROM :=
ifeq ($(UNAME_S), Darwin)
	ifeq ($(UNAME_M), x86_64)
		MAKEROM := ./makerom/darwin_x86_64/makerom
	endif
endif
ifeq ($(UNAME_S), Linux)
	ifeq ($(UNAME_M), x86_64)
		MAKEROM := ./makerom/linux_x86_64/makerom
	endif
endif
ifeq ($(findstring CYGWIN_NT, $(UNAME_S)),CYGWIN_NT)
	MAKEROM := ./makerom/windows_x86_64/makerom.exe
endif
ifeq ($(findstring MINGW32_NT, $(UNAME_S)), MINGW32_NT)
	MAKEROM := ./makerom/windows_x86_64/makerom.exe
endif
#---------------------------------------------------------------------------------


.PHONY: $(BUILD) clean all

#---------------------------------------------------------------------------------
all: $(BUILD) $(OUTPUT).cia $(OUTPUT)/$(TARGET).3dsx

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@mkdir -p $(BUILD)/Snes9x
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
$(OUTPUT).cia: $(OUTPUT).elf $(ASSETS)/$(TARGET).bnr $(ASSETS)/$(TARGET).icn
	@mkdir -p "$(@D)"
	@echo building cia $(notdir $(OUTPUT).cia)
	$(MAKEROM) -rsf $(ASSETS)/$(TARGET).rsf -target t -exefslogo -major $(VERSION_MAJOR) -minor $(VERSION_MINOR) -micro $(VERSION_MICRO) -elf $(OUTPUT).elf -icon $(ASSETS)/$(TARGET).icn -banner $(ASSETS)/$(TARGET).bnr -f cia -o $@

$(OUTPUT)/$(TARGET).3dsx: $(OUTPUT).elf $(OUTPUT)/$(TARGET).smdh
	@mkdir -p "$(@D)"
	@echo building $(notdir $@)
	@3dsxtool $< $@ --smdh=$(OUTPUT)/$(TARGET).smdh $(_3DSXFLAGS)

$(ASSETS)/$(TARGET).bnr:
	@mkdir -p "$(@D)"
	@echo building banner $(notdir $@)
	@$(ASSETS)/bannertool32 makebanner -ci $(ASSETS)/$(TARGET).cgfx -a $(ASSETS)/$(TARGET).wav -o $@
	
$(ASSETS)/$(TARGET).icn: $(ICON)
	@mkdir -p "$(@D)"
	@echo building icon $(notdir $@)
	@$(ASSETS)/bannertool32 makesmdh -s "$(APP_TITLE)" -l "$(APP_TITLE) - $(APP_DESCRIPTION)" -p "$(APP_AUTHOR)" -i $(ICON) --flags visible,ratingrequired,recordusage --cero 153 --esrb 153 --usk 153 --pegigen 153 --pegiptr 153 --pegibbfc 153 --cob 153 --grb 153 --cgsrr 153 -o $@ 

$(OUTPUT)/$(TARGET).smdh: $(ICON)
	@mkdir -p "$(@D)"
	@echo building $(notdir $@)
	@smdhtool --create "$(APP_TITLE)" "$(APP_DESCRIPTION)" "$(APP_AUTHOR)" $(ICON) $@
#---------------------------------------------------------------------------------
clean:
	@rm -fr $(BUILD) $(OUTPUTDIR)
	@echo "Cleaned."

#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
# ---------------------------------------------------------------------------------
$(OUTPUT).elf:	$(OFILES)
	@mkdir -p "$(@D)"
	@echo linking $(notdir $@)
	@$(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $@
	@$(NM) -CSn $@ > $(notdir $*.lst)

#---------------------------------------------------------------------------------
# rules for assembling GPU shaders
#---------------------------------------------------------------------------------
define shader-as
	$(eval CURBIN := $(patsubst %.shbin.o,%.shbin,$(notdir $@)))
	picasso -o $(CURBIN) $1
	bin2s $(CURBIN) | $(AS) -o $@
	echo "extern const u8" `(echo $(CURBIN) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" > `(echo $(CURBIN) | tr . _)`.h
	echo "extern const u8" `(echo $(CURBIN) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> `(echo $(CURBIN) | tr . _)`.h
	echo "extern const u32" `(echo $(CURBIN) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> `(echo $(CURBIN) | tr . _)`.h
endef

%.shbin.o : %.v.pica %.g.pica
	@echo $(notdir $^)
	@$(call shader-as,$^)

%.shbin.o : %.v.pica
	@echo $(notdir $<)
	@$(call shader-as,$<)

%.shbin.o : %.shlist
	@echo $(notdir $<)
	@$(call shader-as,$(foreach file,$(shell cat $<),$(dir $<)/$(file)))

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
