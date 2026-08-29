/* Dormant diagnostic: identifies every memset caller in the frame.
   Found the -ftrivial-auto-var-init=pattern cost (~24 KB/frame) on 2026-08-21.

   To arm it:
     1. Copy this file into src/game/ (any compiled dir works).
     2. Makefile: N64_LDFLAGS += --wrap memset
     3. End of game_runStep (src/game/game.c):
          extern void memsetAudit_tick(void);
          memsetAudit_tick();
     4. Run, read the [memset audit] tables from the ISViewer log, translate:
          mips64-elf-addr2line -f -e build/game.elf <ra...>
   Disarm by undoing the three steps. */

#include <stddef.h>
#include <stdint.h>
#include <libdragon.h>

void *__real_memset(void *s, int c, size_t n);

#define AUDIT_SLOTS 96

static struct {
	uint32_t ra;
	uint32_t count;
	uint32_t bytes;
} audit[AUDIT_SLOTS];

void *__wrap_memset(void *s, int c, size_t n)
{
	uint32_t ra = (uint32_t)(uintptr_t)__builtin_return_address(0);

	for (int i = 0; i < AUDIT_SLOTS; i++) {
		if (audit[i].ra == ra || audit[i].ra == 0) {
			audit[i].ra     = ra;
			audit[i].count += 1;
			audit[i].bytes += (uint32_t)n;
			break;
		}
	}

	return __real_memset(s, c, n);
}

void memsetAudit_tick(void)
{
	static uint32_t frames;
	if (++frames < 300) return;
	frames = 0;

	debugf("[memset audit] caller count bytes\n");
	for (int i = 0; i < AUDIT_SLOTS; i++) {
		if (!audit[i].ra) break;
		debugf("  %08lx %6lu %8lu\n",
		       (unsigned long)audit[i].ra,
		       (unsigned long)audit[i].count,
		       (unsigned long)audit[i].bytes);
		audit[i] = (typeof(audit[0])){0};
	}
}
