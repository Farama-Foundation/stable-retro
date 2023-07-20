/* Copyright (c) 2013-2017 Jeffrey Pfau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#include <mgba/internal/lr35902/debugger/memory-debugger.h>

#include <mgba/internal/debugger/parser.h>
#include <mgba/internal/lr35902/debugger/debugger.h>

#include <mgba-util/math.h>

#include <string.h>

static bool _checkWatchpoints(struct LR35902Debugger* debugger, uint16_t address, struct mDebuggerEntryInfo* info, enum mWatchpointType type, uint8_t newValue);

#define FIND_DEBUGGER(DEBUGGER, CPU) \
	do { \
		DEBUGGER = 0; \
		size_t i; \
		for (i = 0; i < CPU->numComponents; ++i) { \
			if (CPU->components[i]->id == DEBUGGER_ID) { \
				DEBUGGER = (struct LR35902Debugger*) ((struct mDebugger*) cpu->components[i])->platform; \
				goto debuggerFound; \
			} \
		} \
		abort(); \
		debuggerFound: break; \
	} while(0)

#define CREATE_WATCHPOINT_SHIM(NAME, RW, VALUE, RETURN, TYPES, ...) \
	static RETURN DebuggerShim_ ## NAME TYPES { \
		struct LR35902Debugger* debugger; \
		FIND_DEBUGGER(debugger, cpu); \
		struct mDebuggerEntryInfo info; \
		if (_checkWatchpoints(debugger, address, &info, WATCHPOINT_ ## RW, VALUE)) { \
			mDebuggerEnter(debugger->d.p, DEBUGGER_ENTER_WATCHPOINT, &info); \
		} \
		return debugger->originalMemory.NAME(cpu, __VA_ARGS__); \
	}

CREATE_WATCHPOINT_SHIM(load8, READ, 0, uint8_t, (struct LR35902Core* cpu, uint16_t address), address)
CREATE_WATCHPOINT_SHIM(store8, WRITE, value, void, (struct LR35902Core* cpu, uint16_t address, int8_t value), address, value)

static bool _checkWatchpoints(struct LR35902Debugger* debugger, uint16_t address, struct mDebuggerEntryInfo* info, enum mWatchpointType type, uint8_t newValue) {
	struct LR35902DebugWatchpoint* watchpoint;
	size_t i;
	for (i = 0; i < LR35902DebugWatchpointListSize(&debugger->watchpoints); ++i) {
		watchpoint = LR35902DebugWatchpointListGetPointer(&debugger->watchpoints, i);
		if (watchpoint->address == address && (watchpoint->segment < 0 || watchpoint->segment == debugger->originalMemory.currentSegment(debugger->cpu, address)) && watchpoint->type & type) {
			if (watchpoint->condition) {
				int32_t value;
				int segment;
				if (!mDebuggerEvaluateParseTree(debugger->d.p, watchpoint->condition, &value, &segment) || !(value || segment >= 0)) {
					return false;
				}
			}
			info->type.wp.oldValue = debugger->originalMemory.load8(debugger->cpu, address);
			info->type.wp.newValue = newValue;
			info->address = address;
			info->type.wp.watchType = watchpoint->type;
			info->type.wp.accessType = type;
			return true;
		}
	}
	return false;
}

void LR35902DebuggerInstallMemoryShim(struct LR35902Debugger* debugger) {
	debugger->originalMemory = debugger->cpu->memory;
	debugger->cpu->memory.store8 = DebuggerShim_store8;
	debugger->cpu->memory.load8 = DebuggerShim_load8;
}

void LR35902DebuggerRemoveMemoryShim(struct LR35902Debugger* debugger) {
	debugger->cpu->memory.store8 = debugger->originalMemory.store8;
	debugger->cpu->memory.load8 = debugger->originalMemory.load8;
}
