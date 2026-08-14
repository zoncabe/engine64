BUILD_DIR=build

include $(N64_INST)/include/n64.mk
include $(T3D_INST)/t3d.mk

N64_CFLAGS   += -std=gnu2x -Iinclude
GLTF_FLAGS = '--base-scale=1'

PROJECT_NAME = game

src =   $(wildcard *.c) \
		$(wildcard src/game/*.c) \
		$(wildcard src/screen/*.c) \
		$(wildcard src/cutscene/*.c) \
		$(wildcard src/time/*.c) \
		$(wildcard src/physics/math/*.c) \
		$(wildcard src/physics/geometry/*.c) \
		$(wildcard src/physics/memory/*.c) \
		$(wildcard src/physics/shapes/*.c) \
		$(wildcard src/physics/body/*.c) \
		$(wildcard src/physics/cloth/*.c) \
		$(wildcard src/physics/broadphase/*.c) \
		$(wildcard src/physics/collision/*.c) \
		$(wildcard src/physics/world/*.c) \
		$(wildcard src/camera/*.c) \
		$(wildcard src/viewport/*.c) \
		$(wildcard src/control/*.c) \
		$(wildcard src/character/*.c) \
		$(wildcard src/entity/*.c) \
		$(wildcard src/scene/*.c) \
		$(wildcard src/player/*.c) \
		$(wildcard src/ui/*.c) \
		$(wildcard src/particles/*.c) \
		$(wildcard src/graphics/*.c) \
		$(wildcard src/shaders/*.c) \
		$(wildcard src/resources/*.c) \
		$(wildcard src/light/*.c) \
		$(wildcard src/render/*.c) \
		$(wildcard src/assets/*.c) \
		$(wildcard src/sound/*.c) \
		$(wildcard src/menu/*.c) \

assets_png = $(wildcard assets/textures/*.png)
assets_gltf = $(wildcard assets/models/*.glb)
assets_ttf = $(wildcard assets/fonts/*.ttf)
assets_wav = $(wildcard assets/audio/*.wav)
assets_conv = $(addprefix filesystem/textures/,$(notdir $(assets_png:%.png=%.sprite))) \
			  $(addprefix filesystem/models/,$(notdir $(assets_gltf:%.glb=%.t3dm))) \
			  $(addprefix filesystem/fonts/,$(notdir $(assets_ttf:%.ttf=%.font64))) \
			  $(addprefix filesystem/audio/,$(notdir $(assets_wav:%.wav=%.wav64)))

# Models with a collision mesh (declared one per line)
assets_collision = filesystem/collision/room.collision \
                   filesystem/collision/brew_flag.collision \
                   filesystem/collision/bandera-uruguay.collision \
                   filesystem/collision/brasil.collision


all: $(PROJECT_NAME).z64

filesystem/textures/%.sprite: assets/textures/%.png
	@mkdir -p $(dir $@)
	@echo "    [SPRITE] $@"
	$(N64_MKSPRITE) $(MKSPRITE_FLAGS) -o filesystem/textures "$<"

filesystem/models/%.t3dm: assets/models/%.glb
	@mkdir -p $(dir $@)
	@echo "    [T3D-MODEL] $@"
	$(T3D_GLTF_TO_3D) $(GLTF_FLAGS) "$<" $@
	$(N64_BINDIR)/mkasset -c 2 -o filesystem/models $@

COLLISION_IMPORTER = tools/collision_importer/collision_importer

$(COLLISION_IMPORTER): tools/collision_importer/main.c
	gcc -O2 -o $@ $< -I$(T3D_INST)/tools/gltf_importer/src/lib -lm

filesystem/collision/%.collision: assets/models/%.glb $(COLLISION_IMPORTER)
	@mkdir -p $(dir $@)
	@echo "    [COLLISION] $@"
	$(COLLISION_IMPORTER) "$<" $@ $(COL_MESHES)
	$(N64_BINDIR)/mkasset -c 1 -o filesystem/collision $@

filesystem/fonts/%.font64: assets/fonts/%.ttf
	@mkdir -p $(dir $@)
	@echo "    [FONT] $@"
	$(N64_MKFONT) $(MKFONT_FLAGS) -o filesystem/fonts "$<"

filesystem/fonts/DroidSans.font64:  MKFONT_FLAGS += --size 10
filesystem/fonts/Headliner10.font64: MKFONT_FLAGS += --size 10
filesystem/fonts/Headliner14.font64: MKFONT_FLAGS += --size 14
filesystem/fonts/Headliner20.font64: MKFONT_FLAGS += --size 20
filesystem/fonts/Headliner40.font64: MKFONT_FLAGS += --size 43
filesystem/fonts/Headliner60.font64: MKFONT_FLAGS += --size 60

filesystem/audio/%.wav64: assets/audio/%.wav
	@mkdir -p $(dir $@)
	@echo "    [AUDIO] $@"
	@$(N64_AUDIOCONV) --wav-compress 3 -o filesystem/audio $<

$(BUILD_DIR)/$(PROJECT_NAME).dfs: $(assets_conv) $(assets_collision)
$(BUILD_DIR)/$(PROJECT_NAME).elf: $(src:%.c=$(BUILD_DIR)/%.o)

$(PROJECT_NAME).z64: N64_ROM_TITLE="not a game"
$(PROJECT_NAME).z64: $(BUILD_DIR)/$(PROJECT_NAME).dfs

clean:
	rm -rf $(BUILD_DIR) *.z64
	rm -rf filesystem

build_lib:
	rm -rf $(BUILD_DIR) *.z64
	make -C $(T3D_INST)
	make all

-include $(wildcard $(BUILD_DIR)/*.d)

.PHONY: all clean
