/**
 * @file kernel.h - exposes an interface to the engine's startup/runtime/teardown lifecycle.
 * 
 * The name "kernel" was chosen because much like an OS kernel, this object is not the entrypoint
 * (that would be the bootloader, in OS terms) but it does contain the 'core functionality.'
 * 
 * - On native platforms, the file 'main.cpp' will call these functions.
 * - On web platforms, the WASM module will export these functions, and a JS layer will call them.
 */

#pragma once

/** Opaque pointer to engine runtime state */
struct EngineKernel;

/**
 * An extern "C" interface was chosen for portability with FFI's.
 */
extern "C" {
    EngineKernel* kernel_create(bool editor);

    bool kernel_running(EngineKernel*);

    void kernel_update(EngineKernel*);

    void kernel_destroy(EngineKernel*);
}