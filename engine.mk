# --- engine64 ----------------------------------------------------------------
# The engine's source list and the rule that compiles it from a game. The
# engine never builds on its own: it is always built from the consuming
# project, into that project's build directory.
#
# Consumer contract, before including this file:
#   ENGINE_DIR   path to this repo.
#   BUILD_DIR    the project's build dir (objects land in $(BUILD_DIR)/engine).
#   ENGINE_SKIP  engine units the game replaces (optional), paths relative
#                to $(ENGINE_DIR)/src.
#
# Provides engine_src (paths relative to $(ENGINE_DIR)/src) and the compile
# rule. The project collects its objects like this:
#   objects = $(engine_src:%.c=$(BUILD_DIR)/engine/%.o) ...

# The order matters, and not for tidiness: the VR4300 icache is 16 KB direct
# mapped, so where each function lands in RAM decides who it evicts. Hot path
# first, cold code last.
ENGINE_ORDER = \
	time \
	physics/math physics/geometry physics/memory physics/shapes \
	physics/body physics/broadphase physics/collision physics/world \
	physics/spring physics/cloth physics/buoyancy \
	character entity player control graphics shaders render scene3d \
	camera viewport particles sound game scene2d ui menu resources

engine_listed = $(foreach m,$(ENGINE_ORDER),\
	$(if $(filter %.c,$(m)),$(m),\
	  $(patsubst $(ENGINE_DIR)/src/%,%,$(wildcard $(ENGINE_DIR)/src/$(m)/*.c))))

engine_src = $(filter-out $(ENGINE_SKIP),$(engine_listed))

# A module the order above misses would vanish from the build without a
# word, so whatever is left over gets appended instead of lost.
engine_all  = $(patsubst $(ENGINE_DIR)/src/%,%,$(shell find $(ENGINE_DIR)/src -name '*.c'))
engine_rest = $(filter-out $(ENGINE_SKIP) $(engine_listed),$(engine_all))
engine_src += $(engine_rest)

# Engine objects land under the consumer's build/engine, so nothing is ever
# written inside the engine repo.
$(BUILD_DIR)/engine/%.o: $(ENGINE_DIR)/src/%.c
	@mkdir -p $(dir $@)
	@echo "    [CC] $<"
	$(CC) -c $(CFLAGS) -o $@ $<
