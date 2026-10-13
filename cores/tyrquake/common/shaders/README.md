# tyrquake Vulkan shaders

This directory holds the shader assets the Vulkan backend
(`common/backend_vulkan.c`) uses to draw with.  The build sees
only the **pre-compiled SPIR-V byte arrays** under
`generated/spv/`; the GLSL source under `src/` and the regen
tooling under `tools/` are developer-side artifacts and never
participate in compiling the libretro core.

## Why this layout

The tyrquake core has a **zero build dependency** rule for
shader compilation -- anyone checking out the repository should
be able to `make RHI_HAVE_VULKAN=1` with nothing beyond a C
toolchain and the vendored Vulkan headers (under
`deps/vulkan/`).  No `glslangValidator` invocation in
Makefile.common, no Python preprocessor, no offline cross-
compiler.

The cost of that rule is that the SPIR-V bytecode itself has
to be checked into the repository.  This directory is the
home for those check-ins.

## Layout

```
common/shaders/
  src/                                 -- GLSL source (reference only)
    fullscreen_quad.vert
    gradient.frag
  generated/
    spv/                               -- pre-compiled SPIR-V byte arrays
      fullscreen_quad_vs.h               (consumed by backend_vulkan.c)
      gradient_fs.h
  README.md                            -- this file
```

The headers under `generated/spv/` declare each shader as a
`static const uint32_t spv_<name>[]` array plus a
`spv_<name>_size` size constant.  `backend_vulkan.c`
`#includes` these directly and feeds the array address +
size to `vkCreateShaderModule`.  No string parsing, no file
IO -- the bytecode lives in `.rodata` as a compile-time
constant.

## Authoring workflow

1. Write or edit a GLSL source file under `src/`.  Vulkan
   GLSL conventions apply: `#version 450`,
   `layout(location = N)` for I/O, `gl_VertexIndex` /
   `gl_FragCoord` for built-ins, Y-down NDC.

2. Run `sh tools/shaders_regen.sh` from the repository
   root.  The script:
     - locates every `.vert` / `.frag` listed at the bottom
       of the script
     - compiles each one to SPIR-V via `glslang -V`
     - optionally runs `spirv-opt -O` if available
     - converts the binary blob to a C `uint32_t` array via
       `tools/spv2h.py`
     - writes the resulting `.h` under `generated/spv/`

3. Commit both the `src/*.glsl` change and the corresponding
   `generated/spv/*.h` change together.  The commit message
   should mention what the shader does, not how it was
   compiled.

## Tools (developer-side, not build-time)

Required:

  - **glslang**: the Khronos reference GLSL -> SPIR-V
    compiler.  Ubuntu/Debian: `apt-get install glslang-tools`.
    Windows: ships in the LunarG Vulkan SDK.  macOS: brew or
    LunarG SDK.

  - **python3**: for `tools/spv2h.py`, which formats the
    SPIR-V binary as a C header.  Any Python 3 will do; no
    third-party libraries used.

Optional:

  - **spirv-opt**: `apt-get install spirv-tools`.  The regen
    script applies `spirv-opt -O` (the balanced optimization
    profile) when the tool is available, falling through
    cleanly when it isn't.  Drivers are free to optimize SPIR-V
    further at `vkCreateShaderModule` time, so the offline
    pass is a startup-cost win rather than a runtime
    correctness requirement.

  - **spirv-dis**: same package.  Useful for inspecting what
    glslang actually produced: `spirv-dis some_shader.spv`.

  - **spirv-cross**: `apt-get install spirv-cross`.  Required
    when authoring shaders that will eventually need to be
    cross-compiled for non-Vulkan backends (GL via GLSL, D3D
    via HLSL, Metal via MSL).  Not used by the current Vulkan-
    only setup but will be wired into the regen script when
    other RHI backends arrive.

The Microsoft DXC compiler (HLSL -> DXBC/DXIL/SPIR-V) is **not
required** at this stage -- we author in GLSL.  If a future
backend needs DXBC/DXIL, `spirv-cross --hlsl` produces HLSL
that DXC can compile, keeping GLSL as the single source of
truth.

## Why GLSL and not HLSL

The original plan was to author in HLSL because DXC produces
SPIR-V, DXBC, and DXIL from one source.  We switched to GLSL
because:

  - DXC is not in the standard Linux package archives;
    glslang is.  Lowers the dev-environment barrier.
  - GLSL maps more directly to the SPIR-V instruction set the
    Vulkan backend consumes.
  - SPIR-V Cross converts SPIR-V -> HLSL cleanly, so the D3D
    backends still get HLSL source when they need it -- they
    just receive it via SPIRV-Cross rather than as the
    authored form.

The choice can be revisited per shader if any specific case
benefits from HLSL semantics (e.g. matrix-major conventions,
register binding annotations).

## Reproducing the current generated headers

```
$ glslang --version
Glslang Version: 11:15.1.0
$ spirv-opt --version
spirv-opt 2025.1
$ python3 --version
Python 3.12.x

$ sh tools/shaders_regen.sh
regen: common/shaders/src/fullscreen_quad.vert -> common/shaders/generated/spv/fullscreen_quad_vs.h
regen: common/shaders/src/gradient.frag -> common/shaders/generated/spv/gradient_fs.h
done.
```

The output should be byte-identical to what's checked in.  If
it isn't, either the toolchain version changed or somebody
edited a generated file by hand.
