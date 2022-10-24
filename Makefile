BUILD_DIR=build
include $(N64_INST)/include/n64.mk

src = $(wildcard src/*.c)
assets_xm = $(wildcard assets/*.xm)
assets_wav = $(wildcard assets/*.wav)
assets_png = $(wildcard assets/*.png)

assets_conv = $(addprefix filesystem/,$(notdir $(assets_xm:%.xm=%.xm64))) \
              $(addprefix filesystem/,$(notdir $(assets_wav:%.wav=%.wav64))) \
              $(addprefix filesystem/,$(notdir $(assets_png:%.png=%.sprite)))

AUDIOCONV_FLAGS ?=
MKSPRITE_FLAGS ?=

all: spook64.z64

filesystem/%.xm64: assets/%.xm
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	@$(N64_AUDIOCONV) $(AUDIOCONV_FLAGS) -o filesystem $<

filesystem/%.wav64: assets/%.wav
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	@$(N64_AUDIOCONV) -o filesystem $<

filesystem/%.sprite: assets/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	@$(N64_MKSPRITE) $(MKSPRITE_FLAGS) -o filesystem "$<"

filesystem/ground.sprite: MKSPRITE_FLAGS=--format RGBA16 --tiles 64,32
filesystem/wall.sprite: MKSPRITE_FLAGS=--format RGBA16 --tiles 64,32
filesystem/roof.sprite: MKSPRITE_FLAGS=--format RGBA16 --tiles 64,32
filesystem/snooper.sprite: MKSPRITE_FLAGS=--format RGBA16 --tiles 32,32
filesystem/spooker1.sprite: MKSPRITE_FLAGS=--format RGBA16 --tiles 32,32
filesystem/numbers.sprite: MKSPRITE_FLAGS=--format RGBA16 --tiles 64,32
filesystem/win.sprite: MKSPRITE_FLAGS=--format RGBA16 --tiles 32,32
filesystem/lose.sprite: MKSPRITE_FLAGS=--format RGBA16 --tiles 32,32

filesystem/light.sprite: MKSPRITE_FLAGS=--format IA8 --tiles 32,64
filesystem/level_light.sprite: MKSPRITE_FLAGS=--format IA8 --tiles 64,64


$(BUILD_DIR)/spook64.dfs: $(assets_conv)
$(BUILD_DIR)/spook64.elf: $(src:%.c=$(BUILD_DIR)/%.o)

spook64.z64: N64_ROM_TITLE="SuperSnooperSpookers"
spook64.z64: $(BUILD_DIR)/spook64.dfs 

clean:
	rm -rf $(BUILD_DIR) spook64.z64

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean
