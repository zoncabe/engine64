BUILD_DIR = build

# Path to the engine64 checkout. Set it to wherever the engine lives; this
# example sits two levels down inside the repo.
ENGINE_DIR = ../..

include $(N64_INST)/include/n64.mk
include $(T3D_INST)/t3d.mk

N64_CFLAGS += -std=gnu2x -I$(ENGINE_DIR)/include -I$(ENGINE_DIR)/src

# --asset-path: the importer looks every material's texture up by FILE NAME,
# recursively, under this path, and builds the path left in the .t3dm by
# replacing this prefix with "rom:/".
GLTF_FLAGS = '--base-scale=1' '--asset-path=assets'

PROJECT_NAME = 02_character_physics

# --- sources ------------------------------------------------------------------

include $(ENGINE_DIR)/engine.mk

src = $(wildcard *.c) $(wildcard prefabs/*.c)

objects = $(engine_src:%.c=$(BUILD_DIR)/engine/%.o) \
          $(src:%.c=$(BUILD_DIR)/%.o)

# --- assets -------------------------------------------------------------------

assets_png  = $(wildcard assets/textures/*.png)
assets_gltf = $(wildcard assets/models/*.glb)

# Models that also need a collision mesh, declared one per line: the room is
# what the body walks on, the water plane is what the waves run on.
assets_collision = filesystem/collision/room.collision \
                   filesystem/collision/water.collision

assets_conv = $(addprefix filesystem/textures/,$(notdir $(assets_png:%.png=%.sprite))) \
              $(addprefix filesystem/models/,$(notdir $(assets_gltf:%.glb=%.t3dm))) \
              $(assets_collision)


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

# The importer comes from the engine and is compiled into build/, the only
# place this example writes to.
COLLISION_IMPORTER = $(BUILD_DIR)/tools/collision_importer

$(COLLISION_IMPORTER): $(ENGINE_DIR)/tools/collision_importer/main.c
	@mkdir -p $(dir $@)
	@echo "    [CC] $<"
	gcc -O2 -o $@ $< -I$(T3D_INST)/tools/gltf_importer/src/lib -lm

# No mesh names after the output: every mesh in the file becomes collision.
filesystem/collision/%.collision: assets/models/%.glb $(COLLISION_IMPORTER)
	@mkdir -p $(dir $@)
	@echo "    [COLLISION] $@"
	$(COLLISION_IMPORTER) "$<" $@
	$(N64_BINDIR)/mkasset -c 1 -o filesystem/collision $@

$(BUILD_DIR)/$(PROJECT_NAME).dfs: $(assets_conv)
$(BUILD_DIR)/$(PROJECT_NAME).elf: $(objects)

$(PROJECT_NAME).z64: N64_ROM_TITLE="02_character_physics"
$(PROJECT_NAME).z64: $(BUILD_DIR)/$(PROJECT_NAME).dfs

clean:
	rm -rf $(BUILD_DIR) *.z64
	rm -rf filesystem

-include $(objects:%.o=%.d)

.PHONY: all clean
