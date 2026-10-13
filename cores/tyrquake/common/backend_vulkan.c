/*
 * Vulkan renderer backend.
 *
 * Single-TU implementation.  All internal state is file-static;
 * the only exported symbol is the g_rhi_backend_vk vtable
 * instance.  The vtable is exposed unconditionally so rhi.c can
 * take its address regardless of whether the build flag is set;
 * its init() returns false when RHI_HAVE_VULKAN is undefined,
 * which causes rhi.c's auto path to fall through to the software
 * backend.
 *
 * Build flag:
 *   RHI_HAVE_VULKAN
 *     When defined, the file includes vulkan_core.h and
 *     libretro_vulkan.h (vendored under deps/vulkan/include/,
 *     see deps/vulkan/README.md) and performs the libretro HW
 *     context handshake + actual Vulkan setup.  When undefined,
 *     the file compiles as a no-op vtable instance with init
 *     returning false; the build has no dependency on Vulkan
 *     headers in the default configuration.
 *
 *     The build flag is named RHI_HAVE_VULKAN rather than
 *     RHI_BACKEND_VULKAN to avoid a textual collision with the
 *     enum value of the same name in rhi.h (which is set to a
 *     small integer for the rhi_backend_kind enum -- a
 *     -DRHI_BACKEND_VULKAN=1 macro at compile time would
 *     replace the enum identifier with the literal 1).
 *
 * Phase 4g (this file's current state): the per-frame
 * palette LUT moved off the graphics pipeline and into a
 * compute dispatch.  Same on-screen output as Phase 4f;
 * what changes is the GPU work that produces it.
 *
 * Phase 4f reserved a "compute door" for exactly this
 * swap: VK_IMAGE_USAGE_STORAGE_BIT on the render target,
 * descriptor-set-layout binding stage flags declared as
 * FRAGMENT | COMPUTE, per-frame command recording from
 * Phase 4e ready to issue compute dispatches as well as
 * graphics draws.  This phase walks through that door.
 *
 * What's gone:
 *
 *   - vk_render_pass.  No render pass; the compute
 *     dispatch is the entire per-frame GPU workload.
 *   - vk_framebuffer.  Render passes need framebuffers;
 *     compute dispatches write directly into images via
 *     storage descriptors.
 *   - vk_pipeline (graphics) + vk_vs_module + vk_fs_module.
 *     No graphics pipeline; no vertex or fragment shader.
 *     The fullscreen_quad.vert and textured_palette.frag
 *     SPIR-V headers stay in the tree as reference
 *     material but are no longer #included from this file.
 *   - CmdBeginRenderPass / CmdEndRenderPass / CmdDraw /
 *     CreateRenderPass / CreateFramebuffer /
 *     CreateGraphicsPipelines / DestroyRenderPass /
 *     DestroyFramebuffer entries in vk_fn -- the function
 *     pointers themselves drop out because nothing calls
 *     them.
 *
 * What's new:
 *
 *   - textured_palette.comp + its generated SPIR-V
 *     header (common/shaders/generated/spv/
 *     textured_palette_cs.h).  Functional equivalent of
 *     textured_palette.frag.
 *   - vk_cs_module + vk_compute_pipeline.
 *   - CreateComputePipelines + CmdDispatch in vk_fn.
 *   - Descriptor set layout grows from 2 to 3 bindings:
 *       binding 0: sampler2D u_index   (unchanged)
 *       binding 1: sampler2D u_palette (unchanged)
 *       binding 2: writeonly image2D u_output -- the
 *         compute shader's storage-image write target,
 *         pointing at vk_image_view.  Stage flags =
 *         COMPUTE only (no fragment use).
 *   - Descriptor pool gains 1 STORAGE_IMAGE entry.
 *   - record_frame loses the BeginRenderPass / Draw /
 *     EndRenderPass triplet and gains a CmdDispatch
 *     bracketed by two extra layout transitions on
 *     vk_image: UNDEFINED -> GENERAL on entry (compute
 *     needs GENERAL to write storage images), then
 *     GENERAL -> SHADER_READ_ONLY_OPTIMAL on exit so the
 *     libretro frontend reads it as a sampled image.
 *
 * What's the same:
 *
 *   - vid.buffer upload pattern.  R_RenderView still
 *     populates the 8bpp framebuffer; backend_vk_upload_
 *     vid_buffer still memcpys the index data and
 *     d_8to24table_shifted into the staging buffer.
 *   - CmdCopyBufferToImage staging -> vk_texture and
 *     staging -> vk_palette_texture, with their pre /
 *     post layout transitions.
 *   - Per-frame command recording (Phase 4e mechanics).
 *   - QueueSubmit + QueueWaitIdle + set_image to the
 *     libretro frontend.
 *
 * Why this is worth doing:
 *
 *   - Validates the Phase 4f compute-door design end-to-
 *     end.  Without the swap, the door is just an
 *     assertion; with it, the assertion is exercised.
 *   - Removes graphics-pipeline state that wasn't going
 *     to survive into Phase 4h+.  When real geometry
 *     drawing arrives, its render pass / pipeline / blend
 *     state / vertex input will be designed for that
 *     workload (overlay quads with alpha blending,
 *     world surfaces with depth + multitexturing, etc.).
 *     The fullscreen-palette-quad pipeline this file used
 *     to carry was specific to "show the SW framebuffer
 *     with an LUT" and wouldn't have been the right
 *     ancestor for any of those.  Better to drop it now
 *     and write the right thing later than to carry it
 *     forward as legacy.
 *   - Sets up the recording pattern (compute dispatch as
 *     a first-class per-frame operation) that future
 *     compute work -- HDR tonemap, post-process,
 *     compute-based rasterizer -- will reuse.
 *
 * Phase 4k (this file's current state): the overlay's hard-
 * coded test pics give way to a real Draw_Pic intercept.
 *
 *   - struct overlay_slot grows a `const void *key`
 *     field used as the cache key; backend_vk_queue_2d_pic
 *     does a linear scan over the slot array looking for
 *     a slot whose key == pic, and on miss finds the
 *     first empty slot and uploads pic->data into it via
 *     the existing begin_uploads / upload_pic_slot /
 *     end_uploads trio.  Once the slot is populated (or
 *     was already cached) the function appends an entry
 *     to vk_overlay_draws with NDC corners converted
 *     from Quake's vid.width / vid.height screen space.
 *
 *   - The render_backend_t vtable grows a queue_2d_pic
 *     pointer (rhi.h).  Software backend leaves it NULL
 *     (falls through to vid.buffer memcpy); Vulkan
 *     backend points it at backend_vk_queue_2d_pic.
 *
 *   - draw.c's Draw_Pic gains a one-liner that fires
 *     g_rhi->queue_2d_pic if the backend implements it,
 *     before the original SW memcpy runs.  Phase 4k
 *     deliberately keeps the SW path -- Vulkan overlay
 *     and SW HUD render the same pic at the same screen
 *     position, the Vulkan one paints on top of the
 *     compute-uploaded SW HUD, and they overlap pixel-
 *     perfectly.  Phase 4l suppresses the SW path when
 *     the Vulkan intercept is active.
 *
 *   - vk_overlay_draws is filled per-frame instead of
 *     once at create_resources: backend_vk_record_frame
 *     calls a new backend_vk_fill_overlay_vb at the
 *     start of its overlay work (filling the vertex
 *     buffer from whatever Draw_Pic queued during
 *     retro_run), and resets vk_overlay_draw_count to
 *     zero at the end of record_frame (ready for the
 *     next frame's queue).  The three hardcoded demo
 *     quads from Phase 4j are gone; the visible
 *     overlay is now whatever Quake actually draws.
 *
 * Uploads are synchronous (begin_uploads /
 * upload_pic_slot / end_uploads inside queue_2d_pic).
 * First frame after a level load may stall briefly
 * while ~50 HUD pics get uploaded; steady state hits
 * the cache and only queues draws.  Phase 4m+ can batch
 * uploads with a fence-based ring if needed.
 *
 * Phase 4l..N -- suppress SW HUD; intercept Draw_Character,
 *     models, sprites, sky, liquids, shadows; palette +
 *     colormap LUT path; dynamic lightmap update; etc.
 *     R_RenderView's dispatch from backend_vk_draw_view
 *     gradually gets replaced by Vulkan-native geometry
 *     recording in record_frame.  vid.buffer becomes
 *     unused for the Vulkan path at the end of the
 *     migration.  Each stage lands as a separate commit
 *     against this same file.
 *
 * Convention: every internal symbol is static.  Function-pointer
 * tables (vk_fn) are file-static structs populated by
 * context_reset and torn down by context_destroy.  No libvulkan
 * link -- every Vulkan symbol is resolved through the
 * frontend-supplied loader at run time.
 */

#include "rhi.h"
#include "perf_timing.h"
#include "vid.h"          /* viddef_t vid, d_8to24table_shifted --
                           * Phase 4d needs vid.buffer for the per-
                           * frame index upload; Phase 4f reads
                           * d_8to24table_shifted (the tinted
                           * RGBA palette that tracks damage / quad /
                           * underwater shifts) for the palette
                           * texture upload. */
#include "draw.h"         /* draw_chars -- the 128x128 conchars atlas
                           * Phase 4o queue_2d_char uploads into a
                           * dedicated slot to back Draw_Character /
                           * Draw_String. */

extern float skytime;       /* r_sky.c -- modulo'd cl.time-derived
                             * cycle; D_DrawSkyScans8 multiplies it
                             * by skyspeed/skyspeed2 inside the SW
                             * raster, our compute sky does the same
                             * inside backend_vk_record_sky_dispatch */
extern float skyspeed, skyspeed2;
extern void R_RenderView(void);   /* r_main.c -- backend_vk_draw_view
                                   * dispatches into the SW rasterizer
                                   * for Phase 4d.  The real geometry
                                   * pipeline lands in Phase 4e+. */

#ifdef RHI_HAVE_VULKAN

#include "vulkan/vulkan_core.h"
#include "libretro_vulkan.h"
#include "shaders/generated/spv/textured_palette_cs.h"
#include "shaders/generated/spv/overlay_quad_vs.h"
#include "shaders/generated/spv/overlay_quad_fs.h"
#include "shaders/generated/spv/particles_cs.h"
#include "shaders/generated/spv/warpscreen_cs.h"
#include "shaders/generated/spv/sprite_cs.h"
#include "shaders/generated/spv/alias_cs.h"
#include "shaders/generated/spv/sky_cs.h"
#include "shaders/generated/spv/turb_cs.h"
#include "surf_atlas.h"   /* RHI-agnostic brush-atlas allocator;
                           * Phase 5b-07.  The Vulkan backend owns
                           * the VkImage + staging buffer that the
                           * brush compute dispatch samples / uploads
                           * to; surf_atlas owns the (key -> rect)
                           * map and the 2D bin-packing.  The
                           * allocator instance is constructed in
                           * backend_vk_create_resources and lives as
                           * long as the resources do. */
#include "d_iface.h"      /* particle_t -- the GPU compute particle
                           * rasterizer walks the active linked list
                           * via this typedef.  Also TURB_CYCLE for
                           * the warp shader's phase modulus. */
#include "d_local.h"      /* d_pzbuffer / d_pix_* / d_vrect*_particle /
                           * d_y_aspect_shift -- the SW raster state
                           * the particle compute shader's push
                           * constants snapshot. */
#include "r_local.h"      /* xcenter, ycenter, r_origin -- the rest
                           * of the SW raster state the particle
                           * compute shader snapshots.  Plus scr_vrect
                           * (transitively via screen.h) for the warp
                           * shader's bounds. */
#include "r_shared.h"     /* intsintable[] -- the sin lookup table the
                           * warp shader uploads once at init. */
#include "screen.h"       /* scr_vrect -- the rectangle the warp
                           * dispatch's push constants snapshot each
                           * frame. */
#include "client.h"       /* cl -- cl.time drives the warp phase. */
#include <string.h>

extern retro_environment_t    environ_cb;
extern retro_log_printf_t     log_cb;
extern retro_video_refresh_t  video_cb;     /* libretro.c */
extern bool                   did_flip;     /* libretro.c -- set true after end_frame
                                             * to suppress retro_run's dupe path */
extern unsigned               width;        /* libretro.c -- libretro framebuffer width */
extern unsigned               height;       /* libretro.c -- libretro framebuffer height */

/* Forward decls for the context callbacks the frontend will
 * call.  Backend_vk_init's struct initialization references
 * their addresses; definitions appear below the init body. */
static void backend_vk_context_reset(void);
static void backend_vk_context_destroy(void);

/* The frontend's interface, retrieved at context_reset and
 * zeroed at context_destroy.  All Vulkan work below the
 * vtable surface gates on this being non-NULL: if the
 * frontend hasn't called context_reset yet, vk_iface is
 * NULL and the per-frame vtable functions noop. */
static const struct retro_hw_render_interface_vulkan *vk_iface;

/* Convenience cached handles -- duplicated from vk_iface for
 * brevity at use sites.  Lifetime matches vk_iface. */
static VkInstance       vk_instance;
static VkPhysicalDevice vk_gpu;
static VkDevice         vk_device;
static VkQueue          vk_queue;
static uint32_t         vk_queue_family;

/* Per-context Vulkan objects.  Created during context_reset
 * (after the function-pointer table is populated), destroyed
 * during context_destroy.  All zeroed via VK_NULL_HANDLE at
 * teardown.
 *
 * vk_resources_ready is true only after every object has been
 * successfully created and the clear command buffer has been
 * recorded; end_frame gates on it.  If any creation step fails
 * mid-context_reset, the flag stays false and end_frame
 * silently no-ops, falling through to the dupe path in
 * retro_run -- the same visual end state as Phase 3b. */
static qboolean         vk_resources_ready;
/* Phase 5b-08 step 1: ring depth used by ALL N-buffered
 * arrays below.  Defined here (above the first use) so
 * every static array decl can reference it.  Full
 * explanation of the ring design at the
 * vk_frame_ctx_t declaration further down. */
#define VK_FRAME_RING_DEPTH 2

/* Phase 5b-08 step 3a: vk_image / _memory / _view ring-
 * buffered.  vk_image is the R8G8B8A8_UNORM final compose
 * target the libretro frontend reads via set_image.
 * Frame N's CB writes to vk_image[slot N]; while the
 * frontend may still be reading it (the set_image spec
 * says the image is valid only until video_refresh
 * returns, but with the step-3b render-done semaphore the
 * frontend defers its actual read until signal), frame
 * N+1's CB writes to vk_image[slot N+1] -- a different
 * image, so no race.  This commit lays down the ring
 * wiring without flipping behavior; 3b adds the semaphore
 * handoff and removes QueueWaitIdle so the ring becomes
 * load-bearing. */
static VkImage          vk_image[VK_FRAME_RING_DEPTH];
static VkDeviceMemory   vk_image_memory[VK_FRAME_RING_DEPTH];
static VkImageView      vk_image_view[VK_FRAME_RING_DEPTH];
static VkCommandPool    vk_cmd_pool;
static VkCommandBuffer  vk_cmd_buffer;
/* Phase 5b-08 step 3b: dedicated one-shot upload fence
 * for backend_vk_end_uploads.  Pre-3b the upload path
 * used QueueWaitIdle to drain everything and guarantee
 * the slot's CB was no longer in pending state before
 * record_frame's BeginCommandBuffer.  With end_frame's
 * QueueWaitIdle gone in this commit, that drain would
 * also wait on prior-frame submits still in flight,
 * stalling unrelated GPU work.  This fence narrows the
 * wait to exactly the upload's submit. */
static VkFence          vk_upload_fence;

/* Phase 5b-08 step 1: per-frame ring infrastructure.
 *
 * The previous design used a single VkCommandBuffer + a
 * QueueWaitIdle at the end of every frame to keep CPU and
 * GPU work strictly serialized.  Measurement showed that
 * serialization costs ~3.7 ms / frame at 1080p in the
 * test scene (the entire SUBMIT_PRESENT section minus
 * negligible QueueSubmit / set_image / video_cb time),
 * which is the bulk of the compute backend's gap vs. the
 * SW renderer.
 *
 * This commit introduces the structural piece needed to
 * remove that serialization: a ring of N command buffers
 * + per-slot fences.  Each frame uses the next slot in
 * the ring; begin_frame waits on that slot's fence (which
 * signals when the previous use of this slot finishes on
 * the GPU), and end_frame submits with that slot's fence
 * as the signal target.
 *
 * In THIS commit, QueueWaitIdle is preserved at end_frame
 * so behavior is identical -- the fences are technically
 * waited on at acquire but the wait is instant because
 * QueueWaitIdle already drained the queue.  A follow-up
 * commit (5b-08 step 3) removes QueueWaitIdle and lets
 * the GPU run ahead.
 *
 * N=2 is the canonical "double-buffered" choice: minimum
 * memory overhead, minimum latency cost.  If profiling
 * later shows the CPU stalls often at acquire (i.e. CPU
 * is faster than GPU on average), N can be bumped to 3.
 *
 * vk_cmd_buffer remains as a "current frame" pointer that
 * record_frame and the one-shot recording paths reference
 * unchanged; begin_frame sets it to the active slot's CB.
 * The ring slots own the actual VkCommandBuffer handles. */

typedef struct vk_frame_ctx_s {
    VkCommandBuffer cmd_buffer;
    VkFence         done_fence;
    /* Phase 5b-08 step 3b: render-done semaphore signaled
     * by end_frame's QueueSubmit and passed to set_image
     * so the frontend defers its image read until GPU
     * completion.  Created in create_resources alongside
     * the slot's fence; destroyed in destroy_resources. */
    VkSemaphore     render_done_semaphore;
} vk_frame_ctx_t;

static vk_frame_ctx_t vk_frame_ctx[VK_FRAME_RING_DEPTH];
static unsigned       vk_active_slot;   /* index into vk_frame_ctx; advances at begin_frame */

/* Phase 4g: compute pipeline + its shader module.  Same
 * lifetime as vk_image -- created in context_reset after
 * the function-pointer table is populated, destroyed in
 * context_destroy.  The pipeline binds vk_pipeline_layout
 * which references the (3-binding) descriptor set layout
 * declared further down; the layout has no push constants. */
static VkShaderModule   vk_cs_module;
static VkPipelineLayout vk_pipeline_layout;
static VkPipeline       vk_compute_pipeline;

/* Phase 4h: 2D overlay graphics pipeline.  Render pass +
 * framebuffer wrap the same vk_image_view that the compute
 * pipeline writes to, with loadOp = LOAD so the compute
 * output is preserved.  The render pass's attachment
 * initialLayout = GENERAL receives vk_image straight from
 * the compute dispatch (no explicit pipeline barrier
 * between them; the subpass dependency carries the
 * memory dependency), and finalLayout =
 * SHADER_READ_ONLY_OPTIMAL hands it to the libretro
 * frontend.
 *
 * Phase 4i: the overlay pipeline reuses vk_pipeline_layout
 * (the 3-binding compute layout) instead of its own empty
 * layout.  The FS samples a palette-indexed R8_UNORM
 * texture + the shared 256x1 vk_palette_texture and
 * outputs the resulting RGB.
 *
 * Phase 4j: the single test texture grows into a slot
 * array (vk_overlay_slots), each entry carrying its own
 * image / memory / view / descriptor set + dimensions;
 * the single draw turns into a per-frame draw list
 * (vk_overlay_draws), each entry pointing at a slot and
 * supplying NDC corners for the quad.  The vertex buffer
 * is sized for OVERLAY_DRAW_MAX quads (HOST_VISIBLE
 * persistent map; populated at create_resources for
 * Phase 4j, will become per-frame in Phase 4k).  All
 * three demo slots are populated at create_resources via
 * the shared begin_uploads / upload_pic_slot /
 * end_uploads helper trio. */
#define OVERLAY_SLOT_MAX 128   /* room for HUD + menu pics; Phase 4j uses 3 */
#define OVERLAY_DRAW_MAX 4096  /* per-frame draw cap; bumped from 256 in
                                * Phase 4o because Draw_Character / Draw_String
                                * intercepts can push thousands of entries when
                                * the console is open at high resolution (every
                                * visible character is one entry).  At 4096
                                * entries the VB is 4096 * 4 * 16 = 256 KB --
                                * still trivial. */

struct overlay_slot {
    VkImage         image;
    VkDeviceMemory  memory;
    VkImageView     view;
    VkDescriptorSet descriptor_set;
    unsigned        width;
    unsigned        height;
    const void     *key;     /* cache key: qpic_t pointer for pic slots, or
                              * &draw_chars for the conchars slot, or NULL for
                              * an empty slot */
    uint64_t        last_used_frame;
                             /* Phase 5b-06 follow-up: monotonic
                              * vk_overlay_frame_counter at the most
                              * recent hit / fresh upload.  Begin-of-
                              * frame eviction pass picks the slot
                              * with the smallest stamp (older = less
                              * recently used) so the table can't
                              * deadlock at 128 stale entries from
                              * sustained Cache_FreeLow churn.  Zero
                              * for an empty slot. */
};

struct overlay_draw {
    unsigned slot_idx;
    float    x0;   /* NDC top-left x */
    float    y0;   /* NDC top-left y */
    float    x1;   /* NDC bottom-right x */
    float    y1;   /* NDC bottom-right y */
    /* Sub-UV control (Phase 4o).  pic draws set (0, 0)-(1, 1)
     * to sample the whole slot texture; character draws set
     * (col/16, row/16)-((col+1)/16, (row+1)/16) to pick one
     * 8x8 cell out of the 128x128 conchars atlas. */
    float    u0;
    float    v0;
    float    u1;
    float    v1;
};

static VkShaderModule       vk_overlay_vs_module;
static VkShaderModule       vk_overlay_fs_module;
static VkRenderPass         vk_overlay_render_pass;
/* Phase 5b-08 step 3a: ring-buffered overlay framebuffer.
 * Each slot's framebuffer wraps its slot's vk_image_view. */
static VkFramebuffer        vk_overlay_framebuffer[VK_FRAME_RING_DEPTH];
static VkPipeline           vk_overlay_pipeline;
/* Phase 5b-08 step 2e: overlay vertex buffer ring-buffered.
 * backend_vk_fill_overlay_vb writes the per-frame 2D draw
 * list into this VkBuffer on the CPU side; record_frame's
 * CmdBindVertexBuffers + CmdDraw consume it from the GPU.
 * With QueueWaitIdle gone (step 3), frame N+1's fill could
 * race frame N's still-in-flight reads if this were
 * single-instance. */
static VkBuffer             vk_overlay_vertex_buffer[VK_FRAME_RING_DEPTH];
static VkDeviceMemory       vk_overlay_vertex_memory[VK_FRAME_RING_DEPTH];
static void                *vk_overlay_vertex_ptr[VK_FRAME_RING_DEPTH];
static struct overlay_slot  vk_overlay_slots[OVERLAY_SLOT_MAX];
static struct overlay_draw  vk_overlay_draws[OVERLAY_DRAW_MAX];
static unsigned             vk_overlay_draw_count;
static size_t               vk_overlay_upload_offset;

/* Phase 5b-06 follow-up: monotonic counter advanced at every
 * backend_vk_begin_frame.  Used as the LRU stamp on slot hits /
 * fresh uploads and as the grace-period reference in the begin_
 * frame slot eviction pass (slots whose last_used_frame fell more
 * than LRU_EVICT_GRACE_FRAMES behind the current value are torn
 * down to reclaim their GPU image / memory / view / descriptor).
 *
 * Starts at 1 so a freshly-uploaded slot's stamp (=current frame)
 * is strictly greater than the BSS-zero "never used" sentinel; the
 * eviction pass would otherwise see brand-new slots as
 * already-stale on the first frame and immediately tear them down. */
static uint64_t             vk_overlay_frame_counter = 1;
#define LRU_EVICT_GRACE_FRAMES 4u

/* Phase 4c: sampled-texture objects.  vk_texture is the
 * source image that the compute shader reads through u_index;
 * separate from vk_image (the render target).  Sized at
 * width x height: the texture is a 1:1 image of the SW
 * renderer's vid.buffer (Phase 4d).
 *
 * Phase 4f: vk_texture is R8_UNORM (single channel holding
 * the palette index) instead of R8G8B8A8_UNORM.
 * vk_palette_texture is the 256x1 RGBA8 lookup the compute
 * shader indexes into using vk_texture's value as the U
 * coordinate.  (Phase 5b-01 then promoted vk_texture to
 * R8_UINT + STORAGE_BIT so compute SW raster ports can
 * imageStore palette indices directly; see the format-
 * choice rationale at the vk_texture CreateImage call
 * site.)
 *
 * Phase 4g: the descriptor set grows from 2 bindings to 3.
 * Binding 2 is a writeonly STORAGE_IMAGE pointing at
 * vk_image_view -- the compute dispatch's output target.
 * The same vk_image_view is reused for both compute-write
 * (descriptor binding 2 with VK_IMAGE_LAYOUT_GENERAL) and
 * frontend-sample (libretro set_image with VK_IMAGE_LAYOUT_
 * SHADER_READ_ONLY_OPTIMAL); R8G8B8A8_UNORM is storage-
 * capable per the Vulkan 1.0 mandatory format support table
 * and the view inherits the image's full usage flags.
 *
 * vk_staging is a HOST_VISIBLE buffer.  Layout:
 *   bytes [0 .. width*height-1]   = index data (R8 per pixel)
 *   bytes [width*height .. +1023] = palette (256 RGBA8 entries)
 * Mapped persistently at create_resources time;
 * vk_staging_ptr stays valid until destroy_resources frees
 * the memory (vkFreeMemory implicitly unmaps).  Per-frame
 * upload writes through vk_staging_ptr; the next QueueSubmit's
 * two CmdCopyBufferToImage commands carry the two regions
 * into their respective textures.  HOST_COHERENT means no
 * Flush call needed.
 *
 * vk_descriptor_set is allocated from vk_descriptor_pool and
 * is freed implicitly when the pool is destroyed; no separate
 * FreeDescriptorSets call. */
static VkSampler              vk_sampler;
static VkImage                vk_texture;
static VkDeviceMemory         vk_texture_memory;
static VkImageView            vk_texture_view;
static VkImage                vk_palette_texture;
static VkDeviceMemory         vk_palette_texture_memory;
static VkImageView            vk_palette_texture_view;
/* Phase 5b-08 step 2a: vid.buffer staging is now ring-
 * buffered (one slot per frame context) so that, once
 * QueueWaitIdle is removed in step 3, frame N+1's CPU
 * memcpy of vid.buffer into the staging region can run
 * while frame N's GPU is still doing its
 * CmdCopyBufferToImage from frame N's staging.  Without
 * this, the CPU write and the GPU read would race on the
 * same memory.
 *
 * vk_staging_size remains scalar -- every slot is the
 * same size (width*height + palette bytes).
 *
 * The mid-frame one-shot upload helpers (begin_uploads
 * / upload_pic_slot / end_uploads) target the ACTIVE
 * slot's staging via vk_staging_ptr[vk_active_slot]
 * because they reuse the staging buffer that the
 * per-frame upload also uses, and they execute
 * synchronously within the active frame. */
static VkBuffer               vk_staging_buffer[VK_FRAME_RING_DEPTH];
static VkDeviceMemory         vk_staging_memory[VK_FRAME_RING_DEPTH];
static VkDeviceSize           vk_staging_size;
static void                  *vk_staging_ptr[VK_FRAME_RING_DEPTH];
static VkDescriptorSetLayout  vk_descriptor_set_layout;
static VkDescriptorPool       vk_descriptor_pool;
/* Phase 5b-08 step 3a: ring-buffered.  Binding 2 (storage
 * image u_output) differs per slot because vk_image_view is
 * now N-buffered.  Bindings 0 + 1 (sampled vk_texture +
 * vk_palette_texture) are shared across slots. */
static VkDescriptorSet        vk_descriptor_set[VK_FRAME_RING_DEPTH];

/* Phase 4f staging-buffer layout constants.  The palette is
 * 256 RGBA8 entries = 1 KiB, placed immediately after the
 * width*height bytes of index data.  Compile-time -- staging
 * is allocated with this exact total at create_resources. */
#define VK_PALETTE_TEXELS  256u
#define VK_PALETTE_BYTES   (VK_PALETTE_TEXELS * 4u)

/* --------------------------------------------------------
 * Phase 5b-02: GPU compute particle rasterizer resources.
 *
 * vk_zbuffer is the GPU side of d_pzbuffer.  R16_UINT
 * storage image (matches the SW path's `short *` Z values
 * bit-for-bit; the Z-test only ever compares non-negative
 * izi values so unsigned read works), sized to width x
 * height.  Filled from a host-visible staging buffer
 * (vk_particles_zstaging) once per frame ahead of the
 * particle dispatch, so the dispatch's Z-tests see the
 * world / alias / brush Z values the CPU SW raster left
 * in d_pzbuffer.  No need to read this back to the CPU --
 * subsequent CPU stages don't depend on whatever the
 * particle pass wrote to the GPU copy.
 *
 * vk_particles_buffer is an SSBO holding up to
 * VK_PARTICLES_MAX records of struct vk_gpu_particle (vec3
 * origin + uint colour, 16 bytes each).  Host-visible +
 * host-coherent so backend_vk_dispatch_3d_particles can
 * memcpy directly with no Flush.  Capacity sized
 * generously above Quake's typical active count (default
 * MAX_PARTICLES is 2048; the cmdline can raise
 * r_numparticles arbitrarily but actual active particles
 * rarely exceed a few hundred in normal gameplay).
 * Overflow is silently truncated -- a degenerate scene
 * with thousands of simultaneous particles is a glitch
 * worth accepting over wasting hundreds of MiB on a worst-
 * case allocation.
 *
 * vk_particles_zstaging is the host-visible staging
 * buffer for d_pzbuffer uploads.  Separate from
 * vk_staging_buffer (which is sized for the vid.buffer
 * index data + the 1 KiB palette, no room for the 2-byte-
 * per-pixel zbuffer) -- simpler than retrofitting the
 * existing buffer layout.  Sized to width * height * 2.
 *
 * vk_particles_cs_module / vk_particles_pipeline_layout /
 * vk_particles_pipeline are the compute pipeline objects
 * for the particles.comp shader.  Layout: a 3-binding DSL
 * (SSBO at binding 0, vk_texture as storage image at
 * binding 1, vk_zbuffer as storage image at binding 2) +
 * a 96-byte push-constant block (struct vk_particles_pc).
 *
 * vk_particles_dsl / vk_particles_pool / vk_particles_set
 * are the particle pipeline's descriptor objects.
 * Separate pool from vk_descriptor_pool because the
 * binding layouts differ; sharing would complicate the
 * pool sizing.  Allocated once at create_resources,
 * updated once with the final image views / SSBO handle.
 *
 * vk_pending_particle_count is the per-frame staging
 * count.  Set by backend_vk_dispatch_3d_particles; read
 * by backend_vk_record_frame to decide whether to emit
 * the particle dispatch + barrier sequence; reset to
 * zero after every record.  When zero, record_frame
 * follows the original (no-particle) path -- no zbuffer
 * upload, no GENERAL transitions on vk_texture, no
 * dispatch.  Avoids paying for the extra ~4 MiB upload
 * + extra barriers on frames without particles. */
#define VK_PARTICLES_MAX   8192u
#define VK_PARTICLES_BYTES (VK_PARTICLES_MAX * 16u)

struct vk_gpu_particle {
    float    origin[3];
    uint32_t color;
};

struct vk_particles_pc {
    float    r_origin[3];
    float    xcenter;
    float    r_pright[3];
    float    ycenter;
    float    r_pup[3];
    int32_t  d_y_aspect_shift;
    float    r_ppn[3];
    int32_t  d_pix_shift;
    int32_t  d_vrectx;
    int32_t  d_vrecty;
    int32_t  d_vrectright_particle;
    int32_t  d_vrectbottom_particle;
    int32_t  d_pix_min;
    int32_t  d_pix_max;
    uint32_t particle_count;
    uint32_t _pad;
};

static VkImage                vk_zbuffer;
static VkDeviceMemory         vk_zbuffer_memory;
static VkImageView            vk_zbuffer_view;
/* Phase 5b-08 step 2b: particles SSBO is ring-buffered.
 * dispatch_3d_particles memcpys GpuParticle records into
 * vk_particles_ptr[active_slot] each frame, and the
 * compute dispatch reads from the same slot via the
 * descriptor set bound for that slot.  Without N-buffering,
 * once QueueWaitIdle is removed, frame N+1's CPU memcpy
 * would race with frame N's GPU read of the SSBO. */
static VkBuffer               vk_particles_buffer[VK_FRAME_RING_DEPTH];
static VkDeviceMemory         vk_particles_memory[VK_FRAME_RING_DEPTH];
static void                  *vk_particles_ptr[VK_FRAME_RING_DEPTH];
/* Phase 5b-08 step 2a: zbuf staging is ring-buffered for
 * the same reason as vid.buffer staging -- once
 * QueueWaitIdle is removed, frame N+1's CPU memcpy of
 * d_pzbuffer must not race with frame N's GPU still
 * reading from its staging.  Size is identical across
 * slots (set by backend_vk_resize_zstaging when the
 * viewport size changes); only the buffer/memory/ptr
 * triple is ringed. */
static VkBuffer               vk_particles_zstaging[VK_FRAME_RING_DEPTH];
static VkDeviceMemory         vk_particles_zstaging_memory[VK_FRAME_RING_DEPTH];
static void                  *vk_particles_zstaging_ptr[VK_FRAME_RING_DEPTH];
static VkShaderModule         vk_particles_cs_module;
static VkPipelineLayout       vk_particles_pipeline_layout;
static VkPipeline             vk_particles_pipeline;
static VkDescriptorSetLayout  vk_particles_dsl;
static VkDescriptorPool       vk_particles_pool;
/* Phase 5b-08 step 2b: N descriptor sets, one per ring
 * slot.  Each set's binding 0 points at the corresponding
 * vk_particles_buffer[i]; bindings 1 + 2 (vk_texture_view,
 * vk_zbuffer_view) are shared across slots (those output
 * images are still single-instance; step 3 N-buffers
 * vk_texture).  At dispatch time the active slot's set is
 * bound. */
static VkDescriptorSet        vk_particles_set[VK_FRAME_RING_DEPTH];
static uint32_t               vk_pending_particle_count;
static struct vk_particles_pc vk_particles_push;

/* --------------------------------------------------------
 * Phase 5b-03: GPU compute water-warp resources.
 *
 * Fused warp + palette-mapping compute pipeline that
 * replaces the regular palette dispatch (vk_compute_
 * pipeline) on frames where the player's view leaf is in
 * water.  Reads u_index (vk_texture, the just-uploaded
 * vid.buffer + any particle dispatch writes), samples it
 * with a per-pixel sin offset from intsintable, palette-
 * maps the sampled byte through u_palette, and writes
 * the RGBA result directly to u_output (vk_image, the
 * frame's final swapchain image).  Single dispatch per
 * frame -- no intermediate vk_texture_warped buffer
 * needed because the warp and palette steps are fused
 * into one shader.
 *
 * Resources:
 *   vk_warp_table_buffer  : UBO holding the 256-entry
 *                           intsintable[] from R_InitTurb,
 *                           uploaded ONCE at backend init
 *                           (the table only depends on
 *                           TURB_SCREEN_AMP and TURB_CYCLE,
 *                           neither of which change).
 *                           std140-packed as 64 ivec4s.
 *   vk_warp_table_memory  : DEVICE_LOCAL backing memory
 *                           for vk_warp_table_buffer
 *                           (uploaded via a one-shot
 *                           CopyBuffer at init time).
 *   vk_warp_cs_module     : compiled SPIR-V for
 *                           warpscreen.comp.
 *   vk_warp_dsl           : 4-binding DSL (matches the
 *                           palette compute's 3-binding
 *                           DSL plus binding 3 for the
 *                           sin table UBO).
 *   vk_warp_pool          : descriptor pool, 1 set
 *                           sized for the four bindings.
 *   vk_warp_set           : the allocated descriptor set;
 *                           bindings 0-2 point at the
 *                           same handles vk_descriptor_set
 *                           does (vk_texture / vk_palette
 *                           _texture / vk_image), binding
 *                           3 at vk_warp_table_buffer.
 *   vk_warp_pipeline_layout : DSL + 32 B push constants.
 *   vk_warp_pipeline      : the compute pipeline.
 *
 * Per-frame state:
 *   vk_warp_active        : set by backend_vk_dispatch_3d_
 *                           warp_screen_impl, read by
 *                           record_frame to choose between
 *                           the regular palette dispatch
 *                           and the warp+palette dispatch.
 *                           Reset to false at end of
 *                           record_frame.
 *   vk_warp_push          : push-constant snapshot at
 *                           dispatch-stage time
 *                           (phase / scr_vrect bounds);
 *                           record_frame pushes this
 *                           verbatim.
 */
#define VK_WARP_TABLE_SIZE  (2u * 128u)              /* TURB_TABLE_SIZE */
#define VK_WARP_TABLE_BYTES (16u * 64u)              /* 64 ivec4 slots */

struct vk_warp_pc {
    int32_t phase;
    int32_t scr_x;
    int32_t scr_y;
    int32_t scr_w;
    int32_t scr_h;
    int32_t _pad0;
    int32_t _pad1;
    int32_t _pad2;
};

static VkBuffer               vk_warp_table_buffer;
static VkDeviceMemory         vk_warp_table_memory;
/* Temporary handles used between the warp pipeline
 * setup block and the post-cmd-pool upload block in
 * create_resources.  Held in file-static scope only so
 * the two blocks can communicate; the upload destroys
 * them before create_resources returns.  Outside
 * create_resources they're always VK_NULL_HANDLE. */
static VkBuffer               vk_warp_table_staging;
static VkDeviceMemory         vk_warp_table_staging_memory;
static VkShaderModule         vk_warp_cs_module;
static VkPipelineLayout       vk_warp_pipeline_layout;
static VkPipeline             vk_warp_pipeline;
static VkDescriptorSetLayout  vk_warp_dsl;
static VkDescriptorPool       vk_warp_pool;
/* Phase 5b-08 step 3a: ring-buffered.  Binding 2 (storage
 * image u_output = vk_image_view) differs per slot.
 * Bindings 0/1/3 (sampled vk_texture, sampled vk_palette_
 * texture, warp UBO) are shared. */
static VkDescriptorSet        vk_warp_set[VK_FRAME_RING_DEPTH];
static qboolean               vk_warp_active;
static struct vk_warp_pc      vk_warp_push;

/* --------------------------------------------------------
 * Phase 5b-07a: GPU compute sky raster resources.
 *
 * Two storage images (vk_sky_front_image, vk_sky_back_image,
 * R8_UINT 128x128 each) hold the two Quake sky layers:
 * front (masked overlay, palette index 0 = transparent) and
 * back (opaque underlay).  Populated via notify_sky_texture
 * from R_InitSky at level load -- the call can predate
 * resources_ready, so the data is first parked in the CPU
 * cache (vk_sky_cache_front / vk_sky_cache_back) and the
 * vk_sky_upload_pending flag is set; the per-frame
 * record_frame setup performs the actual CmdCopyBufferToImage
 * the first time it sees both resources_ready and pending.
 *
 * vk_sky_staging is a small persistent host-visible buffer
 * sized for both layers back-to-back (2 * 128 * 128 = 32 KiB).
 * Re-uploaded only when the level's sky texture changes;
 * across context resets the CPU cache survives so
 * destroy_resources / create_resources cycles automatically
 * re-push without R_InitSky needing to re-fire.
 *
 * Consumed by sky.comp in commit 3 of this phase; commit 1
 * stands the resources up only.  Until commit 3 lands the
 * images sit at GENERAL layout with valid contents but
 * unbound to any pipeline. */
#define VK_SKY_TEXEL_W            128u
#define VK_SKY_TEXEL_H            128u
#define VK_SKY_LAYER_BYTES        (VK_SKY_TEXEL_W * VK_SKY_TEXEL_H)
#define VK_SKY_STAGING_BYTES      (VK_SKY_LAYER_BYTES * 2u)
static VkImage                vk_sky_front_image;
static VkDeviceMemory         vk_sky_front_memory;
static VkImageView            vk_sky_front_view;
static VkImage                vk_sky_back_image;
static VkDeviceMemory         vk_sky_back_memory;
static VkImageView            vk_sky_back_view;
/* Phase 5b-08 step 2d: sky_staging ring-buffered.  Used
 * primarily on level loads (sky front/back atlas upload)
 * but ring-buffered so that two back-to-back uploads
 * across a frame boundary don't race once QueueWaitIdle
 * is removed in step 3. */
static VkBuffer               vk_sky_staging[VK_FRAME_RING_DEPTH];
static VkDeviceMemory         vk_sky_staging_memory[VK_FRAME_RING_DEPTH];
static void                  *vk_sky_staging_ptr[VK_FRAME_RING_DEPTH];
static byte                   vk_sky_cache_front[VK_SKY_LAYER_BYTES];
static byte                   vk_sky_cache_back[VK_SKY_LAYER_BYTES];
static qboolean               vk_sky_cache_populated;
static qboolean               vk_sky_upload_pending;

/* --------------------------------------------------------
 * Phase 5b-07a step 2 + 3: GPU compute sky raster pipeline.
 *
 * Per-frame the SW pass-1 surface scan in d_edge.c calls
 * dispatch_3d_sky_span(u, v, count) once for each espan_t
 * belonging to a SURF_DRAWSKY surface.  We accumulate them
 * into vk_sky_collected[] (CPU array of u16 / u16 / u16
 * triples) and track the screen-space bbox as we go.  At
 * record_frame, if any spans were collected, we bucket the
 * span list by scanline -- giving the shader an O(spans-on-
 * this-row) per-pixel scan instead of O(total-spans) -- and
 * upload three buffers to the GPU:
 *
 *   - vk_sky_ubo:    view-direction parameters and the bbox
 *                    (std140; 96 bytes).
 *   - vk_sky_rows:   per-row (first_span_idx, span_count)
 *                    pairs (std430 SSBO; uvec2 * vid.height).
 *   - vk_sky_spans:  flat (u, count) pairs sorted by row
 *                    (std430 SSBO; uvec2 * span_count).
 *
 * All three buffers are HOST_VISIBLE | HOST_COHERENT and
 * permanently mapped; the per-frame upload is just three
 * memcpys into mapped pointers followed by one CmdDispatch.
 * Sky textures live in vk_sky_front_view / vk_sky_back_view
 * from step 1, bound through the same descriptor set as
 * sampled storage images.
 *
 * Dispatch shape: 8x8 workgroups covering the screen-space
 * bbox; per-invocation the shader does the per-row bucket
 * lookup, the SW point-in-poly test against the bucket's
 * span list, and (on hit) the SW sphere-projection +
 * two-layer texel lookup, writing palette indices into
 * vk_texture.  The shader is sky.comp; see comments there
 * for the per-pixel math correspondence with d_sky.c::D_-
 * DrawSkyScans8. */
#define VK_SKY_MAX_SPANS     8192u
#define VK_SKY_MAX_ROWS      2048u   /* vid.height ceiling */
#define VK_SKY_SPAN_BYTES    (VK_SKY_MAX_SPANS * 2u * sizeof(uint32_t))
#define VK_SKY_ROW_BYTES     (VK_SKY_MAX_ROWS  * 2u * sizeof(uint32_t))
#define VK_SKY_UBO_BYTES     96u

struct vk_sky_collected_span {
    uint16_t u;
    uint16_t v;
    uint16_t count;
    uint16_t _pad;
};

struct vk_sky_ubo {
    float    vpn[3];        float _pad0;
    float    vright[3];     float _pad1;
    float    vup[3];        float _pad2;
    float    xcenter;
    float    ycenter;
    float    xscale;
    float    yscale;
    float    timespeed1;
    float    timespeed2;
    uint32_t bbox_min_x;
    uint32_t bbox_min_y;
    uint32_t bbox_w;
    uint32_t bbox_h;
    uint32_t _pad3;
    uint32_t _pad4;
};

static struct vk_sky_collected_span vk_sky_collected[VK_SKY_MAX_SPANS];
static unsigned               vk_sky_collected_count;
static int                    vk_sky_bbox_min_x;
static int                    vk_sky_bbox_min_y;
static int                    vk_sky_bbox_max_x;
static int                    vk_sky_bbox_max_y;

/* Phase 5b-08 step 2d: sky UBO + spans + rows are ring-
 * buffered.  All three are written by the dispatch
 * builder (record_frame's pre-dispatch CPU pass) and
 * read by the compute dispatch. */
static VkBuffer               vk_sky_ubo_buffer[VK_FRAME_RING_DEPTH];
static VkDeviceMemory         vk_sky_ubo_memory[VK_FRAME_RING_DEPTH];
static void                  *vk_sky_ubo_ptr[VK_FRAME_RING_DEPTH];
static VkBuffer               vk_sky_spans_buffer[VK_FRAME_RING_DEPTH];
static VkDeviceMemory         vk_sky_spans_memory[VK_FRAME_RING_DEPTH];
static void                  *vk_sky_spans_ptr[VK_FRAME_RING_DEPTH];
static VkBuffer               vk_sky_rows_buffer[VK_FRAME_RING_DEPTH];
static VkDeviceMemory         vk_sky_rows_memory[VK_FRAME_RING_DEPTH];
static void                  *vk_sky_rows_ptr[VK_FRAME_RING_DEPTH];

static VkShaderModule         vk_sky_cs_module;
static VkDescriptorSetLayout  vk_sky_dsl;
static VkDescriptorPool       vk_sky_pool;
/* Phase 5b-08 step 2d: N descriptor sets, one per slot.
 * Bindings 0/1/2/6 (storage images vk_texture / sky_front /
 * sky_back / vk_zbuffer) shared; bindings 3/4/5 (UBO + two
 * SSBOs) differ per slot. */
static VkDescriptorSet        vk_sky_set[VK_FRAME_RING_DEPTH];
static VkPipelineLayout       vk_sky_pipeline_layout;
static VkPipeline             vk_sky_pipeline;

/* --------------------------------------------------------
 * Phase 5b-07b: GPU compute turb (water/slime/lava) raster.
 *
 * Per-surface dispatch with push-constant gradient.  d_edge.c
 * calls dispatch_3d_turb_surface(spans_head, grad, texture,
 * phase) once per SURF_DRAWTURB surface that hits the simple
 * single-pass opaque branch (the common default with no
 * r_liquidblend / r_wateralpha tweaks).  Backend per surface:
 *   - resolves the cacheblock pointer to an atlas slot (per-
 *     frame pointer-keyed cache, VK_TURB_TEX_SLOTS = 32
 *     unique textures, dedups across the frame's surfaces and
 *     uploads the 64x64 mip-0 once per slot per frame).
 *   - walks the espan_t list, packs (u, count) into vk_turb_-
 *     spans at the current span_head, bucket-by-row counts +
 *     prefix-sums into vk_turb_rows scoped to the surface's
 *     row range.
 *   - records a per-surface entry (bbox, rows_first,
 *     spans_first, tex_slot, phase, gradient).
 * In record_frame we upload any pending atlas slots and emit
 * one CmdDispatch per collected surface, push-constant-driven.
 *
 * Reuses vk_zbuffer (R32_UINT, atomicMax target shared with
 * the alias / particle / sprite compute paths).  turb.comp
 * writes per-pixel 1/z directly -- unlike sky's "infinity"
 * sentinel -- so closer geometry behind a water surface
 * correctly loses the atomicMax race.  Mirrors the SW single-
 * pass mode's D_DrawZSpans semantics. */
#define VK_TURB_MAX_SURFACES    256u
#define VK_TURB_TEX_SLOTS       32u      /* unique 64x64 atlases per frame */
#define VK_TURB_TEX_SIZE        64u
#define VK_TURB_SLOT_BYTES      (VK_TURB_TEX_SIZE * VK_TURB_TEX_SIZE)
#define VK_TURB_ATLAS_BYTES     (VK_TURB_SLOT_BYTES * VK_TURB_TEX_SLOTS)
#define VK_TURB_MAX_ROWS        16384u   /* sum of bbox_h across surfaces */
#define VK_TURB_MAX_SPANS       8192u
#define VK_TURB_ROW_BYTES       (VK_TURB_MAX_ROWS  * 2u * sizeof(uint32_t))
#define VK_TURB_SPAN_BYTES      (VK_TURB_MAX_SPANS * 2u * sizeof(uint32_t))

struct vk_turb_surface {
    uint32_t bbox_min_x;
    uint32_t bbox_min_y;
    uint32_t bbox_w;
    uint32_t bbox_h;
    uint32_t rows_first;
    uint32_t spans_first;
    uint32_t tex_slot;
    int32_t  phase;
    uint32_t alpha;          /* 0..255; 255 = no stipple */
    uint32_t is_pass2;       /* 0 = single-pass, 1 = pass-2 */
    rhi_turb_gradient_t grad;
};

struct vk_turb_tex_cache_entry {
    const void *ptr;
    uint32_t    slot;
};

static struct vk_turb_surface       vk_turb_surfaces[VK_TURB_MAX_SURFACES];
static unsigned                     vk_turb_surface_count;
static unsigned                     vk_turb_rows_used;
static unsigned                     vk_turb_spans_used;
static struct vk_turb_tex_cache_entry vk_turb_tex_cache[VK_TURB_TEX_SLOTS];
static unsigned                     vk_turb_tex_count;
static unsigned                     vk_turb_tex_upload_first;
static unsigned                     vk_turb_tex_upload_last;

static VkImage                vk_turb_atlas_image;
static VkDeviceMemory         vk_turb_atlas_memory;
static VkImageView            vk_turb_atlas_view;
/* Phase 5b-08 step 2d: turb atlas staging ring-buffered.
 * Per-frame writes (texture slot uploads) on the CPU
 * side; record_frame CmdCopyBufferToImage's into the
 * single-instance atlas image, which is layout-
 * transitioned by per-frame barriers (the image itself
 * doesn't need ring-buffering thanks to queue-order
 * execution + transition barriers, but the staging
 * does since the CPU writes happen during dispatch
 * setup). */
static VkBuffer               vk_turb_atlas_staging[VK_FRAME_RING_DEPTH];
static VkDeviceMemory         vk_turb_atlas_staging_memory[VK_FRAME_RING_DEPTH];
static void                  *vk_turb_atlas_staging_ptr[VK_FRAME_RING_DEPTH];

/* Phase 5b-08 step 2d: turb rows + spans SSBOs ring-
 * buffered.  Written by dispatch_3d_turb_surface (CPU)
 * each frame, read by the compute dispatch. */
static VkBuffer               vk_turb_rows_buffer[VK_FRAME_RING_DEPTH];
static VkDeviceMemory         vk_turb_rows_memory[VK_FRAME_RING_DEPTH];
static void                  *vk_turb_rows_ptr[VK_FRAME_RING_DEPTH];
static VkBuffer               vk_turb_spans_buffer[VK_FRAME_RING_DEPTH];
static VkDeviceMemory         vk_turb_spans_memory[VK_FRAME_RING_DEPTH];
static void                  *vk_turb_spans_ptr[VK_FRAME_RING_DEPTH];

static VkShaderModule         vk_turb_cs_module;
static VkDescriptorSetLayout  vk_turb_dsl;
static VkDescriptorPool       vk_turb_pool;
/* Phase 5b-08 step 2d: N descriptor sets, one per slot.
 * Bindings 0/1/4 (storage images: vk_texture / turb_atlas
 * / vk_zbuffer) shared; bindings 2/3 (two SSBOs) per slot. */
static VkDescriptorSet        vk_turb_set[VK_FRAME_RING_DEPTH];
static VkPipelineLayout       vk_turb_pipeline_layout;
static VkPipeline             vk_turb_pipeline;

/* --------------------------------------------------------
 * Phase 5b-07: GPU compute brush-surface rasterizer atlas.
 *
 * The brush surface atlas is a single 4096x4096 R8_UINT
 * storage image holding D_CacheSurface composites (already-
 * lit texture x lightmap byte buffers, palette indices) for
 * every brush surface currently in view.  Sized to the
 * cross-API max-texture floor (D3D9 SM2 / GL 2.x guarantee
 * 4096, real Vulkan / D3D11+ hardware goes much higher) so
 * the same atlas dims work unchanged when future rasterized
 * backends are added.
 *
 * Resource layout:
 *   vk_brush_atlas_image       (R8_UINT, 4096x4096, GENERAL
 *                              layout after first frame --
 *                              the compute shader does
 *                              imageLoad + imageStore via a
 *                              storage view, no sampler)
 *   vk_brush_atlas_staging     (HOST_VISIBLE | HOST_COHERENT
 *                              VkBuffer of VK_BRUSH_ATLAS_BYTES,
 *                              permanently mapped; the
 *                              dispatch site memcpy's CPU
 *                              cacheblock contents directly
 *                              into here at the rect offset
 *                              returned by surf_atlas_get,
 *                              and record_frame issues a
 *                              CmdCopyBufferToImage that
 *                              transfers all dirty rects into
 *                              the storage image in one shot)
 *   vk_brush_atlas             (surf_atlas_t *, the RHI-
 *                              agnostic allocator instance --
 *                              tracks the key->rect map and
 *                              the strip free-lists; see
 *                              surf_atlas.h)
 *
 * The strip ladder picks small strips (16/32 tall) heavy and
 * tall strips (256/512) sparse, matching id1 surface-extent
 * distribution at typical mip levels.  All sizing knobs are
 * preprocessor constants; rebalance once the brush dispatch
 * is wired and per-tier utilization stats are available. */
#define VK_BRUSH_ATLAS_W      4096u
#define VK_BRUSH_ATLAS_H      4096u
#define VK_BRUSH_ATLAS_BYTES  ((size_t)VK_BRUSH_ATLAS_W \
                            *  (size_t)VK_BRUSH_ATLAS_H)
#define VK_BRUSH_MAX_ENTRIES  512u

static VkImage                vk_brush_atlas_image;
static VkDeviceMemory         vk_brush_atlas_memory;
static VkImageView            vk_brush_atlas_view;
static VkBuffer               vk_brush_atlas_staging;
static VkDeviceMemory         vk_brush_atlas_staging_memory;
static void                  *vk_brush_atlas_staging_ptr;
static surf_atlas_t          *vk_brush_atlas;

/* --------------------------------------------------------
 * Phase 5b-05: GPU compute sprite rasterizer resources.
 *
 * Mirrors the particle pipeline's shape (one compute
 * dispatch per primitive, atomicMax Z-test, palette-index
 * writes into vk_texture) but for textured-polygon
 * sprites instead of point splats.  Reuses vk_zbuffer +
 * the d_pzbuffer upload path the particle dispatch built
 * (Phase 5b-02 + 5b-04), so when both sprites and
 * particles fire in the same frame the zbuffer upload
 * happens exactly once.
 *
 * Per-frame data flow:
 *
 *   D_DrawSprite (CPU, d_sprite.c)
 *     -> g_rhi->dispatch_3d_sprite(...)
 *        -> backend_vk_dispatch_3d_sprite_impl
 *           - Appends nump verts to vk_sprite_vertex_pool
 *             (host-mapped).
 *           - Copies the sprite frame's pixel bytes
 *             into the next free slot of vk_sprite_
 *             texture_pool (host-mapped, slot-allocated
 *             round-robin within the frame).
 *           - Computes the screen-space bbox + push-
 *             constant block; appends to
 *             vk_sprite_calls[vk_sprite_count++].
 *
 *   backend_vk_record_frame (CPU)
 *     - Emits the zbuffer upload + GENERAL transitions if
 *       any 3D compute dispatch (particles OR sprites) is
 *       pending.
 *     - Particle dispatch (if any).
 *     - Per-sprite dispatch loop: bind the sprite pipeline
 *       + set once, then for each queued sprite push its
 *       constants + dispatch.  No intra-loop barriers
 *       between sprites -- each operates on a disjoint
 *       (well, sometimes overlapping) bbox of vk_texture +
 *       vk_zbuffer, and the atomicMax Z-test handles
 *       overlap correctly across dispatches the same way
 *       it does within a single dispatch.
 *     - vk_texture GENERAL -> SHADER_READ_ONLY, then
 *       palette / warp dispatch as before.
 *
 * Resources:
 *   vk_sprite_vertex_buffer  : host-mapped SSBO of
 *                              VK_SPRITE_VERT_POOL_VERTS
 *                              SpriteVert records (32
 *                              sprites * 8 max verts).
 *   vk_sprite_texture_buffer : host-mapped SSBO of palette
 *                              indices, addressable as
 *                              packed uint slots in
 *                              std430.  VK_SPRITE_TEX_
 *                              POOL_SLOTS slots * VK_
 *                              SPRITE_TEX_POOL_SLOT_BYTES
 *                              bytes; each sprite frame
 *                              gets one slot per frame.
 *   vk_sprite_cs_module      : compiled SPIR-V for
 *                              sprite.comp.
 *   vk_sprite_dsl            : 4-binding DSL (u_index +
 *                              u_zbuffer storage images,
 *                              vertex SSBO, texture SSBO;
 *                              all COMPUTE).
 *   vk_sprite_pool / _set    : descriptor pool + the
 *                              single set; bindings 0/1
 *                              point at vk_texture +
 *                              vk_zbuffer views in
 *                              GENERAL layout, bindings
 *                              2/3 at the vertex /
 *                              texture SSBO handles.
 *   vk_sprite_pipeline_layout: DSL + 48 B push constants
 *                              (struct vk_sprite_pc).
 *   vk_sprite_pipeline       : compute pipeline.
 *
 * Per-frame state:
 *   vk_sprite_count          : number of sprites queued
 *                              this frame.  Read by
 *                              record_frame, reset to 0
 *                              at end of recording.
 *   vk_sprite_calls          : per-sprite push-constants
 *                              snapshot, indexed
 *                              [0..vk_sprite_count).
 *   vk_sprite_vertex_cursor  : monotonic write offset
 *                              into the vertex pool
 *                              within the frame.  Reset
 *                              at end of recording.
 *
 * Pool sizing is conservative.  Typical Quake scenes
 * render 0..5 sprites per frame in vanilla gameplay;
 * 32 is generous headroom.  Sprite frames in stock
 * Quake top out around 64x64 (4 KiB); the 16 KiB slot
 * size covers up to 128x128 with room to spare and
 * keeps the pool a tidy 512 KiB total.
 */
#define VK_SPRITE_MAX_PER_FRAME       32u
#define VK_SPRITE_MAX_VERTS_PER       8u
#define VK_SPRITE_VERT_POOL_VERTS    (VK_SPRITE_MAX_PER_FRAME * VK_SPRITE_MAX_VERTS_PER)
#define VK_SPRITE_VERT_BYTES         32u   /* sizeof(struct vk_sprite_vert) */
#define VK_SPRITE_VERT_POOL_BYTES    (VK_SPRITE_VERT_POOL_VERTS * VK_SPRITE_VERT_BYTES)
#define VK_SPRITE_TEX_POOL_SLOT_BYTES 16384u
#define VK_SPRITE_TEX_POOL_BYTES     (VK_SPRITE_MAX_PER_FRAME * VK_SPRITE_TEX_POOL_SLOT_BYTES)

struct vk_sprite_vert {
    float    u, v;
    float    inv_z;
    float    s_over_z;
    float    t_over_z;
    float    _pad0;
    float    _pad1;
    float    _pad2;
};

struct vk_sprite_pc {
    int32_t  bbox[4];           /* x_min, y_min, x_max, y_max (exclusive) */
    int32_t  dispatch_info[4];  /* vertex_offset, nump, tex_offset, tex_width */
    int32_t  tex_info[4];       /* tex_height, transparent_idx, _pad, _pad */
};

/* Phase 5b-08 step 2c: sprite SSBOs are ring-buffered.
 * dispatch_3d_sprite memcpys per-sprite vertex + texture
 * data into the active slot's pair each frame; the
 * dispatch reads from the same slot's SSBOs via that
 * slot's descriptor set. */
static VkBuffer               vk_sprite_vertex_buffer[VK_FRAME_RING_DEPTH];
static VkDeviceMemory         vk_sprite_vertex_memory[VK_FRAME_RING_DEPTH];
static void                  *vk_sprite_vertex_ptr[VK_FRAME_RING_DEPTH];
static VkBuffer               vk_sprite_texture_buffer[VK_FRAME_RING_DEPTH];
static VkDeviceMemory         vk_sprite_texture_memory[VK_FRAME_RING_DEPTH];
static void                  *vk_sprite_texture_ptr[VK_FRAME_RING_DEPTH];
static VkShaderModule         vk_sprite_cs_module;
static VkPipelineLayout       vk_sprite_pipeline_layout;
static VkPipeline             vk_sprite_pipeline;
static VkDescriptorSetLayout  vk_sprite_dsl;
static VkDescriptorPool       vk_sprite_pool;
/* Phase 5b-08 step 2c: N descriptor sets, one per ring
 * slot.  Bindings 0+1 (vk_texture_view, vk_zbuffer_view)
 * are shared across slots; bindings 2+3 (the two SSBOs)
 * differ per slot. */
static VkDescriptorSet        vk_sprite_set[VK_FRAME_RING_DEPTH];
static uint32_t               vk_sprite_count;
static uint32_t               vk_sprite_vertex_cursor;
static struct vk_sprite_pc    vk_sprite_calls[VK_SPRITE_MAX_PER_FRAME];

/* --------------------------------------------------------
 * Phase 5b-06: GPU compute alias-model rasterizer
 * resources.
 *
 * Per-frame data flow:
 *
 *   D_PolysetDraw (CPU, d_polyse.c)
 *     -> g_rhi->dispatch_3d_alias(...)
 *        -> backend_vk_dispatch_3d_alias_impl
 *           - Appends num_verts AliasVerts to
 *             vk_alias_vertex_pool (host-mapped).
 *           - Appends num_tris AliasTris to
 *             vk_alias_triangle_pool (host-mapped).
 *           - Resolves the skin pointer to a slot in
 *             vk_alias_skin_pool via the per-frame skin
 *             pointer cache (linear-probe table); on
 *             miss, copies the skin bytes into the next
 *             free slot.
 *           - Same for the colormap pointer (almost
 *             always host_colormap; one slot is reused
 *             across the entire frame in practice).
 *           - Computes the screen-space bbox from the
 *             vertex positions of the triangles in this
 *             call, clamped to the framebuffer.
 *           - Appends the push-constants block to
 *             vk_alias_calls[vk_alias_count++].
 *
 *   backend_vk_record_frame (CPU)
 *     - In the same zbuf_active branch as particles +
 *       sprites: bind the alias pipeline + set once,
 *       then per-call push constants + CmdDispatch.
 *
 * Resources:
 *   vk_alias_vertex_buffer   : host-mapped SSBO, 192 KiB
 *                              (8192 verts * 24 bytes,
 *                              std430 stride for a 6-int
 *                              struct).  Host struct
 *                              vk_alias_vert mirrors the
 *                              layout; we copy only the
 *                              v[0..5] portion of each
 *                              finalvert_t into it.
 *   vk_alias_triangle_buffer : host-mapped SSBO, 96 KiB
 *                              (8192 tris * 12 bytes).
 *   vk_alias_skin_buffer     : host-mapped SSBO, 2 MiB
 *                              (16 slots * 128 KiB).
 *                              Slot size covers the
 *                              largest stock Quake alias
 *                              skin (the player model
 *                              skin at 384*256 = 96 KiB);
 *                              16 slots is well above
 *                              typical visible-entity
 *                              counts.
 *   vk_alias_colormap_buffer : host-mapped SSBO, 64 KiB
 *                              (4 slots * 16 KiB).  The
 *                              colormap is global
 *                              (host_colormap, 64 light
 *                              levels * 256 palette
 *                              entries = 16 KiB); 4 slots
 *                              is overkill but cheap.
 *
 * Per-frame state:
 *   vk_alias_count           : number of dispatches
 *                              queued this frame.
 *   vk_alias_calls           : per-dispatch push-constant
 *                              snapshots.
 *   vk_alias_vertex_cursor   : monotonic write offset
 *                              into the vertex pool
 *                              (in verts, not bytes).
 *   vk_alias_triangle_cursor : monotonic write offset
 *                              into the triangle pool
 *                              (in triangles).
 *   vk_alias_skin_count      : number of skin slots used
 *                              this frame.
 *   vk_alias_skin_cache      : per-frame skin pointer ->
 *                              slot offset cache.
 *   vk_alias_cmap_count      : same for colormap.
 *   vk_alias_cmap_cache      : same for colormap.
 *
 * Caps are sized for typical scenes:
 *   - 256 dispatches/frame (mix of per-triangle clipped
 *     and per-model unclipped calls; vanilla Quake never
 *     hits this).
 *   - 8192 verts/frame (40 visible entities * average 200
 *     vert/model).
 *   - 8192 tris/frame (same).
 *   - 16 unique skins/frame.
 *   - 4 unique colormaps/frame.
 *
 * Overflow is silent: the offending dispatch is dropped
 * (the affected entity-or-triangle goes unrendered for
 * that frame).  Acceptable for a libretro core; better
 * than asserting.
 */
/* Per-frame caps.  Sized for the worst case Quake throws at the
 * alias path: a viewmodel close enough to camera to straddle the
 * z-clip plane AND the bottom screen edge during the bob cycle.
 * Each non-trivial-accept triangle of such an entity can produce
 * a Sutherland-Hodgman fan of up to ~5 sub-triangles (one per
 * crossed clip plane), and R_AliasClipTriangle dispatches each
 * fan triangle as its own D_PolysetDraw -> dispatch_3d_alias
 * call.  A heavily-clipped viewmodel can ask for 200-500 of
 * these by itself, on top of every other on-screen entity, so
 * the cap has to comfortably exceed any plausible sum.  The
 * previous 256 was just enough at a static gun pose and dropped
 * the gun's bottom triangles -- whichever clipped-fan dispatches
 * came after vk_alias_count hit the cap -- once the bob cycle
 * pushed the gun lower.  8192 leaves ~30x headroom over a single
 * heavily-clipped viewmodel, plenty for any scene.
 *
 * VERT/TRI pool capacities track the dispatch cap: at one fan-
 * triangle per dispatch (3 verts, 1 tri) the worst case sits
 * around 25k verts / 8k tris, and at one batched-call per entity
 * (~150 verts / ~200 tris each) modest entity counts add a few
 * thousand of each.  32k / 16k covers both regimes with margin.
 *
 * Memory cost above the previous caps: push-constant array 12 ->
 * 384 KiB, vertex pool 192 -> 768 KiB, triangle pool 96 -> 192
 * KiB.  ~1.2 MiB extra BSS, well inside the libretro core's
 * working set. */
#define VK_ALIAS_MAX_PER_FRAME      8192u
#define VK_ALIAS_VERT_POOL_VERTS   32768u
#define VK_ALIAS_VERT_BYTES         24u
#define VK_ALIAS_VERT_POOL_BYTES    (VK_ALIAS_VERT_POOL_VERTS * VK_ALIAS_VERT_BYTES)
#define VK_ALIAS_TRI_POOL_TRIS     16384u
#define VK_ALIAS_TRI_BYTES          12u
#define VK_ALIAS_TRI_POOL_BYTES     (VK_ALIAS_TRI_POOL_TRIS * VK_ALIAS_TRI_BYTES)
#define VK_ALIAS_SKIN_SLOTS         16u
#define VK_ALIAS_SKIN_SLOT_BYTES    131072u  /* 128 KiB */
#define VK_ALIAS_SKIN_POOL_BYTES    (VK_ALIAS_SKIN_SLOTS * VK_ALIAS_SKIN_SLOT_BYTES)
#define VK_ALIAS_CMAP_SLOTS         4u
#define VK_ALIAS_CMAP_SLOT_BYTES    16384u   /* 64 * 256, host_colormap size */
#define VK_ALIAS_CMAP_POOL_BYTES    (VK_ALIAS_CMAP_SLOTS * VK_ALIAS_CMAP_SLOT_BYTES)

struct vk_alias_vert {
    int32_t v[6];   /* u, v, s, t, l, 1/z (24 bytes, std430 stride) */
};

struct vk_alias_tri {
    int32_t a;
    int32_t b;
    int32_t c;
};

struct vk_alias_pc {
    int32_t bbox[4];           /* x_min, y_min, x_max, y_max */
    int32_t dispatch_info[4];  /* vert_off, tri_off, tri_count, skin_off */
    int32_t skin_info[4];      /* skin_w, skin_h, cmap_off, _pad */
};

struct vk_alias_slot_cache_entry {
    const void *ptr;
    uint32_t    offset;
    uint32_t    width;
    uint32_t    height;
};

/* Phase 5b-08 step 2c: alias SSBOs are ring-buffered.
 * The four host-mapped pools (vertex, triangle, skin,
 * colormap) are written by dispatch_3d_alias each frame
 * (and by the upload-skin / upload-colormap helpers at
 * material change), read by the dispatch through the
 * active slot's descriptor set. */
static VkBuffer               vk_alias_vertex_buffer[VK_FRAME_RING_DEPTH];
static VkDeviceMemory         vk_alias_vertex_memory[VK_FRAME_RING_DEPTH];
static void                  *vk_alias_vertex_ptr[VK_FRAME_RING_DEPTH];
static VkBuffer               vk_alias_triangle_buffer[VK_FRAME_RING_DEPTH];
static VkDeviceMemory         vk_alias_triangle_memory[VK_FRAME_RING_DEPTH];
static void                  *vk_alias_triangle_ptr[VK_FRAME_RING_DEPTH];
static VkBuffer               vk_alias_skin_buffer[VK_FRAME_RING_DEPTH];
static VkDeviceMemory         vk_alias_skin_memory[VK_FRAME_RING_DEPTH];
static void                  *vk_alias_skin_ptr[VK_FRAME_RING_DEPTH];
static VkBuffer               vk_alias_colormap_buffer[VK_FRAME_RING_DEPTH];
static VkDeviceMemory         vk_alias_colormap_memory[VK_FRAME_RING_DEPTH];
static void                  *vk_alias_colormap_ptr[VK_FRAME_RING_DEPTH];
static VkShaderModule         vk_alias_cs_module;
static VkPipelineLayout       vk_alias_pipeline_layout;
static VkPipeline             vk_alias_pipeline;
static VkDescriptorSetLayout  vk_alias_dsl;
static VkDescriptorPool       vk_alias_pool;
/* Phase 5b-08 step 2c: N descriptor sets per ring slot.
 * Bindings 0+1 (storage images) shared, 2+3+4+5 (SSBOs)
 * per-slot. */
static VkDescriptorSet        vk_alias_set[VK_FRAME_RING_DEPTH];
static uint32_t               vk_alias_count;
static uint32_t               vk_alias_vertex_cursor;
static uint32_t               vk_alias_triangle_cursor;
static uint32_t               vk_alias_skin_count;
static uint32_t               vk_alias_cmap_count;
static struct vk_alias_pc     vk_alias_calls[VK_ALIAS_MAX_PER_FRAME];
static struct vk_alias_slot_cache_entry vk_alias_skin_cache[VK_ALIAS_SKIN_SLOTS];
static struct vk_alias_slot_cache_entry vk_alias_cmap_cache[VK_ALIAS_CMAP_SLOTS];

/* The retro_vulkan_image we hand to set_image.  Built once
 * after the image view is created (the contained
 * VkImageViewCreateInfo is what the frontend uses to know
 * how to sample our image).  Lifetime: until context_destroy.
 * Phase 5b-08 step 3a: N copies, one per slot; each holds
 * the slot's image_view and create_info.  end_frame's
 * set_image call hands the active slot's entry to the
 * frontend. */
static struct retro_vulkan_image vk_retro_image[VK_FRAME_RING_DEPTH];

/* Function-pointer table.  Populated at context_reset via
 * the frontend-supplied get_instance_proc_addr /
 * get_device_proc_addr; zeroed at context_destroy.  Grows
 * as later phases load more entry points.
 *
 * Naming mirrors the upstream symbol with the 'vk' prefix
 * dropped:
 *   vkGetPhysicalDeviceProperties -> vk_fn.GetPhysicalDeviceProperties */
static struct {
    /* Instance-level (loaded via GetInstanceProcAddr) */
    PFN_vkGetInstanceProcAddr             GetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr               GetDeviceProcAddr;
    PFN_vkGetPhysicalDeviceProperties     GetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties;

    /* Device-level (loaded via GetDeviceProcAddr) */
    PFN_vkDeviceWaitIdle                  DeviceWaitIdle;
    PFN_vkCreateImage                     CreateImage;
    PFN_vkDestroyImage                    DestroyImage;
    PFN_vkGetImageMemoryRequirements      GetImageMemoryRequirements;
    PFN_vkAllocateMemory                  AllocateMemory;
    PFN_vkFreeMemory                      FreeMemory;
    PFN_vkBindImageMemory                 BindImageMemory;
    PFN_vkCreateImageView                 CreateImageView;
    PFN_vkDestroyImageView                DestroyImageView;
    PFN_vkCreateCommandPool               CreateCommandPool;
    PFN_vkDestroyCommandPool              DestroyCommandPool;
    PFN_vkAllocateCommandBuffers          AllocateCommandBuffers;
    PFN_vkFreeCommandBuffers              FreeCommandBuffers;
    PFN_vkBeginCommandBuffer              BeginCommandBuffer;
    PFN_vkEndCommandBuffer                EndCommandBuffer;
    PFN_vkCmdPipelineBarrier              CmdPipelineBarrier;
    PFN_vkCmdClearColorImage              CmdClearColorImage;
    PFN_vkQueueSubmit                     QueueSubmit;
    PFN_vkQueueWaitIdle                   QueueWaitIdle;
    /* Frame-context fence pair (Phase 5b-08 step 1).  Added
     * for the N=2 ring of command buffers: each ring slot has
     * its own VkFence so begin_frame can wait on slot N's
     * previous-pass completion before re-recording into it.
     * Still QueueWaitIdle-serialized in this commit; the
     * fence is signaled by submit and waited on at acquire,
     * but the wait is instant because QueueWaitIdle already
     * drained everything.  Removed-and-replaced by fence-
     * only sync in a follow-up commit. */
    PFN_vkCreateFence                     CreateFence;
    PFN_vkDestroyFence                    DestroyFence;
    PFN_vkWaitForFences                   WaitForFences;
    PFN_vkResetFences                     ResetFences;

    /* Phase 5b-08 step 3b: render-done semaphores for the
     * set_image handoff.  end_frame's QueueSubmit signals
     * the active slot's render_done_semaphore alongside
     * the done_fence; set_image passes the semaphore to
     * the frontend so it defers its image consumption
     * until GPU completion.  With QueueWaitIdle removed
     * from end_frame, the begin_frame fence-wait of the
     * next rotation is the new cross-frame sync. */
    PFN_vkCreateSemaphore                 CreateSemaphore;
    PFN_vkDestroySemaphore                DestroySemaphore;

    /* Shader / pipeline (Phase 4b for the layout, Phase 4g
     * for compute, Phase 4h for the overlay graphics
     * path).  Render-pass / framebuffer / graphics-
     * pipeline / Cmd{BeginRenderPass,EndRenderPass,Draw}
     * came back at Phase 4h for the overlay path on top
     * of the compute output.  CmdBindVertexBuffers is new
     * at Phase 4h -- the first time the file uses vertex
     * input. */
    PFN_vkCreateShaderModule              CreateShaderModule;
    PFN_vkDestroyShaderModule             DestroyShaderModule;
    PFN_vkCreatePipelineLayout            CreatePipelineLayout;
    PFN_vkDestroyPipelineLayout           DestroyPipelineLayout;
    PFN_vkCreateComputePipelines          CreateComputePipelines;
    PFN_vkCreateGraphicsPipelines         CreateGraphicsPipelines;
    PFN_vkDestroyPipeline                 DestroyPipeline;
    PFN_vkCreateRenderPass                CreateRenderPass;
    PFN_vkDestroyRenderPass               DestroyRenderPass;
    PFN_vkCreateFramebuffer               CreateFramebuffer;
    PFN_vkDestroyFramebuffer              DestroyFramebuffer;
    PFN_vkCmdBindPipeline                 CmdBindPipeline;
    PFN_vkCmdDispatch                     CmdDispatch;
    PFN_vkCmdBeginRenderPass              CmdBeginRenderPass;
    PFN_vkCmdEndRenderPass                CmdEndRenderPass;
    PFN_vkCmdDraw                         CmdDraw;
    PFN_vkCmdBindVertexBuffers            CmdBindVertexBuffers;

    /* Sampler / descriptor / staging upload (Phase 4c) */
    PFN_vkCreateSampler                   CreateSampler;
    PFN_vkDestroySampler                  DestroySampler;
    PFN_vkCreateBuffer                    CreateBuffer;
    PFN_vkDestroyBuffer                   DestroyBuffer;
    PFN_vkGetBufferMemoryRequirements     GetBufferMemoryRequirements;
    PFN_vkBindBufferMemory                BindBufferMemory;
    PFN_vkMapMemory                       MapMemory;
    PFN_vkUnmapMemory                     UnmapMemory;
    PFN_vkCreateDescriptorSetLayout       CreateDescriptorSetLayout;
    PFN_vkDestroyDescriptorSetLayout      DestroyDescriptorSetLayout;
    PFN_vkCreateDescriptorPool            CreateDescriptorPool;
    PFN_vkDestroyDescriptorPool           DestroyDescriptorPool;
    PFN_vkAllocateDescriptorSets          AllocateDescriptorSets;
    PFN_vkUpdateDescriptorSets            UpdateDescriptorSets;
    PFN_vkResetCommandBuffer              ResetCommandBuffer;
    PFN_vkCmdCopyBufferToImage            CmdCopyBufferToImage;
    PFN_vkCmdBindDescriptorSets           CmdBindDescriptorSets;
    PFN_vkCmdPushConstants                CmdPushConstants;
    PFN_vkCmdCopyBuffer                   CmdCopyBuffer;
} vk_fn;

/* The retro_hw_render_callback we register with the frontend.
 * Lifetime: from backend_vk_init until backend_vk_shutdown.
 * The frontend keeps a pointer to this struct so it must
 * outlive any context_reset / context_destroy dispatch --
 * making it file-static is the canonical pattern. */
static struct retro_hw_render_callback vk_hwrender;

#endif /* RHI_HAVE_VULKAN */

static qboolean
backend_vk_init(void)
{
#ifdef RHI_HAVE_VULKAN
    if (!environ_cb)
        return false;

    memset(&vk_hwrender, 0, sizeof(vk_hwrender));
    vk_hwrender.context_type    = RETRO_HW_CONTEXT_VULKAN;
    vk_hwrender.version_major   = 1;  /* Vulkan 1.0 is the floor */
    vk_hwrender.version_minor   = 0;
    vk_hwrender.context_reset   = backend_vk_context_reset;
    vk_hwrender.context_destroy = backend_vk_context_destroy;
    vk_hwrender.cache_context   = false;

    if (!environ_cb(RETRO_ENVIRONMENT_SET_HW_RENDER, &vk_hwrender)) {
        if (log_cb)
            log_cb(RETRO_LOG_INFO,
                   "rhi-vk: SET_HW_RENDER refused by frontend (no Vulkan available?)\n");
        return false;
    }

    if (log_cb)
        log_cb(RETRO_LOG_INFO,
               "rhi-vk: SET_HW_RENDER accepted; awaiting context_reset\n");
    return true;
#else
    return false;
#endif
}

#ifdef RHI_HAVE_VULKAN

/*
 * Find a memory type that satisfies the type_bits requirements
 * mask (from GetImageMemoryRequirements) and exposes every flag
 * in required_props.  Standard pattern -- walk the
 * memoryTypes[] array and pick the first one that matches both.
 * Returns 0xFFFFFFFF if none found.
 */
static uint32_t
backend_vk_find_memory_type(uint32_t type_bits,
                            VkMemoryPropertyFlags required_props)
{
    VkPhysicalDeviceMemoryProperties mem_props;
    uint32_t i;

    if (!vk_fn.GetPhysicalDeviceMemoryProperties || vk_gpu == VK_NULL_HANDLE)
        return 0xFFFFFFFFu;

    memset(&mem_props, 0, sizeof(mem_props));
    vk_fn.GetPhysicalDeviceMemoryProperties(vk_gpu, &mem_props);

    for (i = 0; i < mem_props.memoryTypeCount; ++i) {
        if (!(type_bits & (1u << i)))
            continue;
        if ((mem_props.memoryTypes[i].propertyFlags & required_props)
            != required_props)
            continue;
        return i;
    }
    return 0xFFFFFFFFu;
}

/*
 * Phase 4j: batched one-shot overlay-pic uploads.
 *
 * Three helpers cooperate.  backend_vk_begin_uploads
 * opens a recording on vk_cmd_buffer (one-time-submit
 * usage) and resets the staging-buffer running offset.
 * backend_vk_upload_pic_slot then runs once per pic:
 * it creates the slot's image + memory + view +
 * descriptor set, copies the pic's bytes into the
 * staging buffer at the running offset, records the
 * required pipeline barriers + CmdCopyBufferToImage into
 * the same command buffer, updates the descriptor set,
 * and advances the offset.  backend_vk_end_uploads
 * closes the buffer, submits it, and QueueWaitIdles so
 * everything is visible by the time create_resources
 * returns.  vk_cmd_buffer auto-resets on the next
 * per-frame BeginCommandBuffer; the staging region is
 * overwritten by the first per-frame vid.buffer upload
 * (assuming the running offset stays within the
 * vk_staging_size budget -- which it trivially does for
 * Phase 4j's three small pics, totalling 1024 + 256 +
 * 256 = 1536 bytes against ~1.2 MB of staging).
 *
 * These helpers will be reused in Phase 4k from inside
 * the Draw_Pic intercept's "qpic_t not in cache, upload
 * it" path -- the cache lookup decides _whether_ to
 * call upload_pic_slot, and the helpers do the same
 * thing in either case.
 */
static qboolean
backend_vk_begin_uploads(void)
{
    VkCommandBufferBeginInfo bi;
    VkResult                 r;

    memset(&bi, 0, sizeof(bi));
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    r = vk_fn.BeginCommandBuffer(vk_cmd_buffer, &bi);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: BeginCommandBuffer (upload batch) failed (%d)\n",
                   (int)r);
        return false;
    }
    vk_overlay_upload_offset = 0;
    return true;
}

/*
 * Tear down one overlay slot's GPU resources in place.  Used by
 * backend_vk_begin_frame's LRU eviction pass and by the slot loop
 * inside destroy_resources at backend teardown.  descriptor_set is
 * not freed explicitly here -- vk_descriptor_pool's lifetime covers
 * every set it allocated, the pool teardown happens after the slot
 * loop in destroy_resources, and the pool has 1 + OVERLAY_SLOT_MAX
 * sets reserved (see VkDescriptorPoolCreateInfo.maxSets at the
 * create site) so an evicted-then-reused slot trivially allocates a
 * fresh DS without touching the leaked one.
 *
 * Caller is responsible for ensuring the slot's GPU resources are
 * not referenced by any in-flight command buffer; backend_vk_begin_
 * frame enforces that by only evicting slots whose last_used_frame
 * is more than LRU_EVICT_GRACE_FRAMES behind the current counter
 * (libretro frame pacing means previous-frame commands have
 * completed by then). */
static void
backend_vk_free_overlay_slot(struct overlay_slot *slot)
{
    if (!slot)
        return;
    if (slot->view != VK_NULL_HANDLE) {
        if (vk_fn.DestroyImageView)
            vk_fn.DestroyImageView(vk_device, slot->view, NULL);
    }
    if (slot->image != VK_NULL_HANDLE) {
        if (vk_fn.DestroyImage)
            vk_fn.DestroyImage(vk_device, slot->image, NULL);
    }
    if (slot->memory != VK_NULL_HANDLE) {
        if (vk_fn.FreeMemory)
            vk_fn.FreeMemory(vk_device, slot->memory, NULL);
    }
    slot->view            = VK_NULL_HANDLE;
    slot->image           = VK_NULL_HANDLE;
    slot->memory          = VK_NULL_HANDLE;
    slot->descriptor_set  = VK_NULL_HANDLE;
    slot->width           = 0;
    slot->height          = 0;
    slot->key             = NULL;
    slot->last_used_frame = 0;
}

static qboolean
backend_vk_upload_pic_slot(unsigned slot_idx,
                           unsigned w, unsigned h,
                           const uint8_t *data)
{
    struct overlay_slot         *slot;
    VkImageCreateInfo            image_ci;
    VkMemoryRequirements         mem_req;
    VkMemoryAllocateInfo         alloc_info;
    VkImageViewCreateInfo        view_ci;
    VkDescriptorSetAllocateInfo  ds_alloc;
    VkDescriptorImageInfo        ds_image_infos[3];
    VkWriteDescriptorSet         ds_writes[3];
    VkImageMemoryBarrier         barrier;
    VkBufferImageCopy            region;
    uint32_t                     mem_type;
    size_t                       bytes;
    VkResult                     r;

    if (slot_idx >= OVERLAY_SLOT_MAX) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: upload_pic_slot: slot_idx %u >= %d\n",
                   slot_idx, OVERLAY_SLOT_MAX);
        return false;
    }
    slot = &vk_overlay_slots[slot_idx];
    if (slot->image != VK_NULL_HANDLE) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: upload_pic_slot: slot %u already in use\n",
                   slot_idx);
        return false;
    }

    bytes = (size_t)w * (size_t)h;
    if (vk_overlay_upload_offset + bytes > vk_staging_size) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: upload_pic_slot: staging overflow "
                   "(offset %llu + %llu > %llu)\n",
                   (unsigned long long)vk_overlay_upload_offset,
                   (unsigned long long)bytes,
                   (unsigned long long)vk_staging_size);
        return false;
    }

    /* Slot's GPU image: R8_UNORM palette-index pic. */
    memset(&image_ci, 0, sizeof(image_ci));
    image_ci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.imageType     = VK_IMAGE_TYPE_2D;
    image_ci.format        = VK_FORMAT_R8_UNORM;
    image_ci.extent.width  = w;
    image_ci.extent.height = h;
    image_ci.extent.depth  = 1;
    image_ci.mipLevels     = 1;
    image_ci.arrayLayers   = 1;
    image_ci.samples       = VK_SAMPLE_COUNT_1_BIT;
    image_ci.tiling        = VK_IMAGE_TILING_OPTIMAL;
    image_ci.usage         = VK_IMAGE_USAGE_SAMPLED_BIT
                           | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    image_ci.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    r = vk_fn.CreateImage(vk_device, &image_ci, NULL, &slot->image);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: CreateImage (slot %u) failed (%d)\n",
                   slot_idx, (int)r);
        return false;
    }

    vk_fn.GetImageMemoryRequirements(vk_device, slot->image, &mem_req);
    mem_type = backend_vk_find_memory_type(mem_req.memoryTypeBits,
                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (mem_type == 0xFFFFFFFFu) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: no DEVICE_LOCAL memory for slot %u image\n",
                   slot_idx);
        return false;
    }

    memset(&alloc_info, 0, sizeof(alloc_info));
    alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize  = mem_req.size;
    alloc_info.memoryTypeIndex = mem_type;

    r = vk_fn.AllocateMemory(vk_device, &alloc_info, NULL, &slot->memory);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: AllocateMemory (slot %u) failed (%d)\n",
                   slot_idx, (int)r);
        return false;
    }

    r = vk_fn.BindImageMemory(vk_device, slot->image, slot->memory, 0);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: BindImageMemory (slot %u) failed (%d)\n",
                   slot_idx, (int)r);
        return false;
    }

    memset(&view_ci, 0, sizeof(view_ci));
    view_ci.sType                       = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_ci.image                       = slot->image;
    view_ci.viewType                    = VK_IMAGE_VIEW_TYPE_2D;
    view_ci.format                      = VK_FORMAT_R8_UNORM;
    view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_ci.subresourceRange.levelCount = 1;
    view_ci.subresourceRange.layerCount = 1;

    r = vk_fn.CreateImageView(vk_device, &view_ci, NULL, &slot->view);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: CreateImageView (slot %u) failed (%d)\n",
                   slot_idx, (int)r);
        return false;
    }

    /* Per-slot descriptor set out of the pool reserved at
     * pool-creation time.  Binding 0 -> this slot's view,
     * binding 1 -> shared palette, binding 2 -> vk_image
     * placeholder (DSL has the slot; the overlay FS
     * doesn't access it). */
    memset(&ds_alloc, 0, sizeof(ds_alloc));
    ds_alloc.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    ds_alloc.descriptorPool     = vk_descriptor_pool;
    ds_alloc.descriptorSetCount = 1;
    ds_alloc.pSetLayouts        = &vk_descriptor_set_layout;

    r = vk_fn.AllocateDescriptorSets(vk_device, &ds_alloc, &slot->descriptor_set);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: AllocateDescriptorSets (slot %u) failed (%d)\n",
                   slot_idx, (int)r);
        return false;
    }

    memset(&ds_image_infos, 0, sizeof(ds_image_infos));
    ds_image_infos[0].sampler     = vk_sampler;
    ds_image_infos[0].imageView   = slot->view;
    ds_image_infos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ds_image_infos[1].sampler     = vk_sampler;
    ds_image_infos[1].imageView   = vk_palette_texture_view;
    ds_image_infos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ds_image_infos[2].sampler     = VK_NULL_HANDLE;
    /* Phase 5b-08 step 3a: overlay graphics path does NOT
     * read binding 2 (storage image is a compute-only slot
     * in the shared DSL).  Bind slot 0's view as a
     * placeholder to satisfy the DSL; the value is never
     * sampled. */
    ds_image_infos[2].imageView   = vk_image_view[0];
    ds_image_infos[2].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    memset(&ds_writes, 0, sizeof(ds_writes));
    ds_writes[0].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    ds_writes[0].dstSet          = slot->descriptor_set;
    ds_writes[0].dstBinding      = 0;
    ds_writes[0].descriptorCount = 1;
    ds_writes[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ds_writes[0].pImageInfo      = &ds_image_infos[0];
    ds_writes[1].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    ds_writes[1].dstSet          = slot->descriptor_set;
    ds_writes[1].dstBinding      = 1;
    ds_writes[1].descriptorCount = 1;
    ds_writes[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    ds_writes[1].pImageInfo      = &ds_image_infos[1];
    ds_writes[2].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    ds_writes[2].dstSet          = slot->descriptor_set;
    ds_writes[2].dstBinding      = 2;
    ds_writes[2].descriptorCount = 1;
    ds_writes[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    ds_writes[2].pImageInfo      = &ds_image_infos[2];

    vk_fn.UpdateDescriptorSets(vk_device, 3, ds_writes, 0, NULL);

    /* Copy the pic into the staging buffer at the running
     * offset, then record barrier + copy + barrier into
     * the open command buffer.  Uses the active ring
     * slot's staging since the one-shot upload happens
     * inside the active frame's CB recording. */
    memcpy((uint8_t *)vk_staging_ptr[vk_active_slot] + vk_overlay_upload_offset, data, bytes);

    memset(&barrier, 0, sizeof(barrier));
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask                   = 0;
    barrier.dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = slot->image;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount     = 1;
    barrier.subresourceRange.layerCount     = 1;

    vk_fn.CmdPipelineBarrier(vk_cmd_buffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0, NULL, 0, NULL,
                             1, &barrier);

    memset(&region, 0, sizeof(region));
    region.bufferOffset                    = vk_overlay_upload_offset;
    region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount     = 1;
    region.imageExtent.width               = w;
    region.imageExtent.height              = h;
    region.imageExtent.depth               = 1;

    vk_fn.CmdCopyBufferToImage(vk_cmd_buffer,
                               vk_staging_buffer[vk_active_slot],
                               slot->image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &region);

    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    vk_fn.CmdPipelineBarrier(vk_cmd_buffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0, NULL, 0, NULL,
                             1, &barrier);

    slot->width  = w;
    slot->height = h;
    vk_overlay_upload_offset += bytes;
    return true;
}

/*
 * backend_vk_refresh_pic_slot -- re-upload the pixel
 * contents of an already-allocated slot in place.
 *
 * backend_vk_upload_pic_slot above creates a fresh slot
 * (image, memory, view, descriptor set) and uploads the
 * initial pixels.  That's wrong for the player-colour
 * preview's translated pic, which has constant
 * dimensions but pixel contents that change every time
 * the user navigates the colour picker -- we'd either
 * tear down and re-create the slot every frame (leaks
 * descriptor sets because the pool isn't flagged
 * FREE_DESCRIPTOR_SET_BIT), or refresh in place.  This
 * helper does the in-place refresh.
 *
 * Assumes the slot already exists (image, view,
 * descriptor set bound) and its current layout is
 * SHADER_READ_ONLY_OPTIMAL (the layout
 * backend_vk_upload_pic_slot leaves it in).  Restores
 * that layout at the end so subsequent overlay draws
 * sample correctly.  Width / height match the slot's
 * existing dimensions; caller is responsible for
 * passing a `data` buffer of the right size.
 *
 * Must be called inside a backend_vk_begin_uploads /
 * end_uploads pair, same as the upload helper.
 */
static qboolean
backend_vk_refresh_pic_slot(unsigned slot_idx, const uint8_t *data)
{
    struct overlay_slot  *slot;
    VkImageMemoryBarrier  barrier;
    VkBufferImageCopy     region;
    size_t                bytes;

    if (slot_idx >= OVERLAY_SLOT_MAX) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: refresh_pic_slot: slot_idx %u >= %d\n",
                   slot_idx, OVERLAY_SLOT_MAX);
        return false;
    }
    slot = &vk_overlay_slots[slot_idx];
    if (slot->image == VK_NULL_HANDLE) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: refresh_pic_slot: slot %u empty\n",
                   slot_idx);
        return false;
    }

    bytes = (size_t)slot->width * (size_t)slot->height;
    if (vk_overlay_upload_offset + bytes > vk_staging_size) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: refresh_pic_slot: staging overflow "
                   "(offset %llu + %llu > %llu)\n",
                   (unsigned long long)vk_overlay_upload_offset,
                   (unsigned long long)bytes,
                   (unsigned long long)vk_staging_size);
        return false;
    }

    /* SHADER_READ_ONLY_OPTIMAL -> TRANSFER_DST_OPTIMAL */
    memset(&barrier, 0, sizeof(barrier));
    barrier.sType                       = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask               = VK_ACCESS_SHADER_READ_BIT;
    barrier.dstAccessMask               = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.oldLayout                   = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.newLayout                   = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex         = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                       = slot->image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.layerCount = 1;

    vk_fn.CmdPipelineBarrier(vk_cmd_buffer,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0, NULL, 0, NULL,
                             1, &barrier);

    /* Staging copy + image copy.  Uses the active ring
     * slot's staging since the one-shot upload happens
     * inside the active frame's CB recording. */
    memcpy((uint8_t *)vk_staging_ptr[vk_active_slot] + vk_overlay_upload_offset, data, bytes);

    memset(&region, 0, sizeof(region));
    region.bufferOffset                = vk_overlay_upload_offset;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.layerCount = 1;
    region.imageExtent.width           = slot->width;
    region.imageExtent.height          = slot->height;
    region.imageExtent.depth           = 1;

    vk_fn.CmdCopyBufferToImage(vk_cmd_buffer,
                               vk_staging_buffer[vk_active_slot],
                               slot->image,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &region);

    /* TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL */
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    vk_fn.CmdPipelineBarrier(vk_cmd_buffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0,
                             0, NULL, 0, NULL,
                             1, &barrier);

    vk_overlay_upload_offset += bytes;
    return true;
}

static qboolean
backend_vk_end_uploads(void)
{
    VkSubmitInfo submit;
    VkResult     r;

    r = vk_fn.EndCommandBuffer(vk_cmd_buffer);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: EndCommandBuffer (upload batch) failed (%d)\n",
                   (int)r);
        return false;
    }

    memset(&submit, 0, sizeof(submit));
    submit.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers    = &vk_cmd_buffer;

    r = vk_fn.QueueSubmit(vk_queue, 1, &submit, vk_upload_fence);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: QueueSubmit (upload batch) failed (%d)\n",
                   (int)r);
        return false;
    }

    /* Phase 5b-08 step 3b: wait on the dedicated upload
     * fence rather than QueueWaitIdle.  The upload's CB
     * shares the active ring slot's command-buffer handle,
     * so record_frame's later BeginCommandBuffer on the
     * same handle requires the upload's pending state to
     * have cleared.  Fence-based wait scopes that drain to
     * the upload only, leaving any other in-flight frame
     * submits (now allowed once end_frame's QueueWaitIdle
     * is gone) alone.
     *
     * Reset for the next end_uploads call; the fence was
     * created UNSIGNALED so this brings it back to the
     * starting state. */
    vk_fn.WaitForFences(vk_device, 1, &vk_upload_fence, VK_TRUE, UINT64_MAX);
    vk_fn.ResetFences(vk_device, 1, &vk_upload_fence);
    return true;
}

/*
 * Create the per-context render target (image + memory + view),
 * the compute-pipeline objects (shader module + pipeline layout
 * + compute pipeline), the texture / palette-texture / staging
 * resources, the descriptor set, and the command-buffer
 * infrastructure (pool + one primary buffer).  Per-frame
 * command recording (Phase 4e) is dispatched from
 * backend_vk_record_frame, not here.  On any failure rolls
 * back and leaves vk_resources_ready false; end_frame then
 * no-ops and the frontend falls back to dupe.
 */
static qboolean
backend_vk_create_resources(void)
{
    VkImageCreateInfo                       image_ci;
    VkMemoryRequirements                    mem_req;
    VkMemoryAllocateInfo                    alloc_info;
    VkImageViewCreateInfo                   view_ci;
    VkShaderModuleCreateInfo                sm_ci;
    VkPipelineLayoutCreateInfo              pl_ci;
    VkPipelineShaderStageCreateInfo         cs_stage;
    VkComputePipelineCreateInfo             cp_ci;
    VkCommandPoolCreateInfo                 pool_ci;
    VkCommandBufferAllocateInfo             cmd_alloc;
    VkSamplerCreateInfo                     sampler_ci;
    VkBufferCreateInfo                      buf_ci;
    VkDescriptorSetLayoutBinding            dsl_bindings[3];
    VkDescriptorSetLayoutCreateInfo         dsl_ci;
    VkDescriptorPoolSize                    dp_sizes[2];
    VkDescriptorPoolCreateInfo              dp_ci;
    VkDescriptorSetAllocateInfo             ds_alloc;
    VkDescriptorImageInfo                   ds_image_infos[3];
    VkWriteDescriptorSet                    ds_writes[3];
    /* Phase 4h: overlay render-pass + graphics-pipeline locals.
     * Kept inline rather than factored into a helper so the
     * single-function create-resources structure is preserved;
     * the overlay setup is mechanically distinct from the
     * compute setup above but uses the same conventions
     * (memset-to-zero, sType, populate, create, error-check). */
    VkAttachmentDescription                 ov_attachment;
    VkAttachmentReference                   ov_color_ref;
    VkSubpassDescription                    ov_subpass;
    VkSubpassDependency                     ov_dep;
    VkRenderPassCreateInfo                  ov_rp_ci;
    VkFramebufferCreateInfo                 ov_fb_ci;
    VkImageView                             ov_fb_attachments[1];
    VkPipelineShaderStageCreateInfo         ov_stages[2];
    VkVertexInputBindingDescription         ov_vb_binding;
    VkVertexInputAttributeDescription       ov_vb_attribs[2];
    VkPipelineVertexInputStateCreateInfo    ov_vi;
    VkPipelineInputAssemblyStateCreateInfo  ov_ia;
    VkViewport                              ov_viewport;
    VkRect2D                                ov_scissor;
    VkPipelineViewportStateCreateInfo       ov_vp;
    VkPipelineRasterizationStateCreateInfo  ov_rs;
    VkPipelineMultisampleStateCreateInfo    ov_ms;
    VkPipelineColorBlendAttachmentState     ov_cba;
    VkPipelineColorBlendStateCreateInfo     ov_cb;
    VkGraphicsPipelineCreateInfo            ov_gp_ci;
    VkBufferCreateInfo                      ov_vb_ci;
    VkMemoryRequirements                    ov_vb_mem_req;
    VkMemoryAllocateInfo                    ov_vb_alloc;
    uint32_t                                mem_type;
    VkResult                                r;

    /* ---------------------------------------------------------
     * Render-target image: R8G8B8A8_UNORM with MUTABLE_FORMAT
     * (libretro spec recommends mutable for 8-bit formats so
     * the frontend can reinterpret as sRGB if it wants).
     * Usage covers our needs (COLOR_ATTACHMENT for the render
     * pass output, SAMPLED for the frontend's read) plus
     * TRANSFER_SRC which the libretro Vulkan interface docs
     * call out as required for set_image.  We retain
     * TRANSFER_DST as well so a future phase can do
     * vkCmdCopyBufferToImage uploads (e.g. lifting the SW
     * framebuffer for a Phase 4c texture sample) without
     * needing to recreate the image.  STORAGE_BIT is added
     * in Phase 4f to keep the door open for compute-based
     * post-process or a compute-rasterizer end state:
     * R8G8B8A8_UNORM's STORAGE_IMAGE_BIT is mandatory in
     * Vulkan 1.0 so the flag is free now. */
    memset(&image_ci, 0, sizeof(image_ci));
    image_ci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.flags         = VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;
    image_ci.imageType     = VK_IMAGE_TYPE_2D;
    image_ci.format        = VK_FORMAT_R8G8B8A8_UNORM;
    image_ci.extent.width  = width;
    image_ci.extent.height = height;
    image_ci.extent.depth  = 1;
    image_ci.mipLevels     = 1;
    image_ci.arrayLayers   = 1;
    image_ci.samples       = VK_SAMPLE_COUNT_1_BIT;
    image_ci.tiling        = VK_IMAGE_TILING_OPTIMAL;
    image_ci.usage         = VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                           | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                           | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                           | VK_IMAGE_USAGE_SAMPLED_BIT
                           | VK_IMAGE_USAGE_STORAGE_BIT;
    image_ci.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    /* Phase 5b-08 step 3a: ring-buffer the render target.
     * Each slot gets its own VkImage + VkDeviceMemory +
     * VkImageView + retro_vulkan_image, so frame N+1's
     * compute write can target a different image than frame
     * N's still-being-read-by-frontend image once 3b lifts
     * QueueWaitIdle.  The image_ci itself is identical
     * across slots, so it's built once outside the loop. */
    {
        unsigned imsi;
        for (imsi = 0; imsi < VK_FRAME_RING_DEPTH; imsi++) {
            r = vk_fn.CreateImage(vk_device, &image_ci, NULL, &vk_image[imsi]);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: CreateImage (slot %u) failed (%d)\n",
                           imsi, (int)r);
                return false;
            }

            memset(&mem_req, 0, sizeof(mem_req));
            vk_fn.GetImageMemoryRequirements(vk_device, vk_image[imsi], &mem_req);

            mem_type = backend_vk_find_memory_type(mem_req.memoryTypeBits,
                                                   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            if (mem_type == 0xFFFFFFFFu) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: no device-local memory type for render target slot %u\n",
                           imsi);
                return false;
            }

            memset(&alloc_info, 0, sizeof(alloc_info));
            alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize  = mem_req.size;
            alloc_info.memoryTypeIndex = mem_type;

            r = vk_fn.AllocateMemory(vk_device, &alloc_info, NULL, &vk_image_memory[imsi]);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: AllocateMemory %lu bytes (slot %u) failed (%d)\n",
                           (unsigned long)mem_req.size, imsi, (int)r);
                return false;
            }

            r = vk_fn.BindImageMemory(vk_device, vk_image[imsi], vk_image_memory[imsi], 0);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: BindImageMemory (slot %u) failed (%d)\n",
                           imsi, (int)r);
                return false;
            }

            /* Image view + retro_vulkan_image: per-slot
             * view_ci because view_ci.image points at the
             * slot's image.  Each slot's vk_retro_image
             * holds its own image_view + create_info copy
             * for the spec-required lifetime. */
            memset(&view_ci, 0, sizeof(view_ci));
            view_ci.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            view_ci.image            = vk_image[imsi];
            view_ci.viewType         = VK_IMAGE_VIEW_TYPE_2D;
            view_ci.format           = VK_FORMAT_R8G8B8A8_UNORM;
            view_ci.components.r     = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_ci.components.g     = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_ci.components.b     = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_ci.components.a     = VK_COMPONENT_SWIZZLE_IDENTITY;
            view_ci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            view_ci.subresourceRange.baseMipLevel   = 0;
            view_ci.subresourceRange.levelCount     = 1;
            view_ci.subresourceRange.baseArrayLayer = 0;
            view_ci.subresourceRange.layerCount     = 1;

            r = vk_fn.CreateImageView(vk_device, &view_ci, NULL, &vk_image_view[imsi]);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: CreateImageView (slot %u) failed (%d)\n",
                           imsi, (int)r);
                return false;
            }

            memset(&vk_retro_image[imsi], 0, sizeof(vk_retro_image[imsi]));
            vk_retro_image[imsi].image_view   = vk_image_view[imsi];
            vk_retro_image[imsi].image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            vk_retro_image[imsi].create_info  = view_ci;
        }
    }

    /* ---------------------------------------------------------
     * Compute shader module.  Just the SPIR-V bytecode
     * wrapped -- the module can be destroyed immediately
     * after the pipeline that uses it is created, but we
     * keep it around for the lifetime of the context to
     * keep teardown symmetric.  No per-frame cost; the
     * driver compiled the bytecode into its internal ISA at
     * pipeline creation. */
    memset(&sm_ci, 0, sizeof(sm_ci));
    sm_ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    sm_ci.codeSize = spv_textured_palette_cs_size;
    sm_ci.pCode    = spv_textured_palette_cs;
    r = vk_fn.CreateShaderModule(vk_device, &sm_ci, NULL, &vk_cs_module);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: CreateShaderModule (cs) failed (%d)\n", (int)r);
        return false;
    }

    /* ---------------------------------------------------------
     * Sampler.  NEAREST filtering on a CLAMP_TO_EDGE address
     * mode.  NEAREST keeps the Phase 4c checkerboard crisp at
     * any scale and matches Quake's "no texture filtering"
     * aesthetic; LINEAR would be appropriate for a Phase 4e
     * "filtered look" option.  CLAMP_TO_EDGE keeps the
     * fragments sampled by the oversize triangle's out-of-
     * viewport portion (uv.x or uv.y in (1, 2]) from wrapping
     * back into the texture -- they'd be clipped anyway but
     * the safe address mode costs nothing. */
    memset(&sampler_ci, 0, sizeof(sampler_ci));
    sampler_ci.sType        = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_ci.magFilter    = VK_FILTER_NEAREST;
    sampler_ci.minFilter    = VK_FILTER_NEAREST;
    sampler_ci.mipmapMode   = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    sampler_ci.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    sampler_ci.minLod       = 0.0f;
    sampler_ci.maxLod       = 0.0f;

    r = vk_fn.CreateSampler(vk_device, &sampler_ci, NULL, &vk_sampler);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR, "rhi-vk: CreateSampler failed (%d)\n", (int)r);
        return false;
    }

    /* ---------------------------------------------------------
     * Source index texture for the palette compute shader to
     * sample from.  Sized 1:1 with the SW framebuffer (width x
     * height), R8_UINT (one byte per pixel, raw palette
     * index from vid.buffer).  OPTIMAL tiling, DEVICE_LOCAL
     * memory.  Usage covers the data path we need:
     * TRANSFER_DST for the CPU-side upload from vid.buffer
     * (still the producer when compute_rendering is off, and
     * the producer of the unported CPU stages when on),
     * SAMPLED for the palette compute shader's read, and
     * STORAGE so the Phase 5b compute SW rasterizer ports
     * (particles, alias, brush, sprite, sky, liquids, world
     * surfaces) can imageStore palette indices directly.
     *
     * Phase 5b-01 switched the format R8_UNORM -> R8_UINT
     * specifically to satisfy the STORAGE requirement:
     * Vulkan 1.0's mandatory format-support table requires
     * R8_UINT (and R8_SINT) to support storage-image use
     * unconditionally on every implementation; R8_UNORM is
     * optional for storage and a portable backend can't
     * assume it.  The format change is functionally
     * invisible -- vk_texture holds 8-bit palette indices
     * either way, and the palette compute shader was
     * updated in lockstep (sampler2D -> usampler2D, integer
     * texelFetch return) to consume the new format.  No
     * MUTABLE_FORMAT flag needed because every consumer
     * now agrees on the integer interpretation. */
    memset(&image_ci, 0, sizeof(image_ci));
    image_ci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.imageType     = VK_IMAGE_TYPE_2D;
    image_ci.format        = VK_FORMAT_R8_UINT;
    image_ci.extent.width  = width;
    image_ci.extent.height = height;
    image_ci.extent.depth  = 1;
    image_ci.mipLevels     = 1;
    image_ci.arrayLayers   = 1;
    image_ci.samples       = VK_SAMPLE_COUNT_1_BIT;
    image_ci.tiling        = VK_IMAGE_TILING_OPTIMAL;
    image_ci.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT
                           | VK_IMAGE_USAGE_SAMPLED_BIT
                           | VK_IMAGE_USAGE_STORAGE_BIT;
    image_ci.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    r = vk_fn.CreateImage(vk_device, &image_ci, NULL, &vk_texture);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: CreateImage (index texture) failed (%d)\n", (int)r);
        return false;
    }

    memset(&mem_req, 0, sizeof(mem_req));
    vk_fn.GetImageMemoryRequirements(vk_device, vk_texture, &mem_req);

    mem_type = backend_vk_find_memory_type(mem_req.memoryTypeBits,
                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (mem_type == 0xFFFFFFFFu) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: no device-local memory type for index texture\n");
        return false;
    }

    memset(&alloc_info, 0, sizeof(alloc_info));
    alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize  = mem_req.size;
    alloc_info.memoryTypeIndex = mem_type;

    r = vk_fn.AllocateMemory(vk_device, &alloc_info, NULL, &vk_texture_memory);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: AllocateMemory (index texture) failed (%d)\n", (int)r);
        return false;
    }

    r = vk_fn.BindImageMemory(vk_device, vk_texture, vk_texture_memory, 0);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: BindImageMemory (index texture) failed (%d)\n", (int)r);
        return false;
    }

    memset(&view_ci, 0, sizeof(view_ci));
    view_ci.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_ci.image            = vk_texture;
    view_ci.viewType         = VK_IMAGE_VIEW_TYPE_2D;
    view_ci.format           = VK_FORMAT_R8_UINT;
    view_ci.components.r     = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_ci.components.g     = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_ci.components.b     = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_ci.components.a     = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_ci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    view_ci.subresourceRange.baseMipLevel   = 0;
    view_ci.subresourceRange.levelCount     = 1;
    view_ci.subresourceRange.baseArrayLayer = 0;
    view_ci.subresourceRange.layerCount     = 1;

    r = vk_fn.CreateImageView(vk_device, &view_ci, NULL, &vk_texture_view);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: CreateImageView (index texture) failed (%d)\n", (int)r);
        return false;
    }

    /* ---------------------------------------------------------
     * Palette texture.  256x1 R8G8B8A8_UNORM, one texel per
     * palette entry.  The palette compute shader fetches
     * this with texelFetch at u = idx (the R8_UINT index
     * value, which arrives as a uint in [0, 255] after the
     * Phase 5b-01 format change).  texelFetch with the
     * integer coord lands directly on the palette entry --
     * no normalised-coord rounding to worry about. */
    memset(&image_ci, 0, sizeof(image_ci));
    image_ci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_ci.imageType     = VK_IMAGE_TYPE_2D;
    image_ci.format        = VK_FORMAT_R8G8B8A8_UNORM;
    image_ci.extent.width  = VK_PALETTE_TEXELS;
    image_ci.extent.height = 1;
    image_ci.extent.depth  = 1;
    image_ci.mipLevels     = 1;
    image_ci.arrayLayers   = 1;
    image_ci.samples       = VK_SAMPLE_COUNT_1_BIT;
    image_ci.tiling        = VK_IMAGE_TILING_OPTIMAL;
    image_ci.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT
                           | VK_IMAGE_USAGE_SAMPLED_BIT;
    image_ci.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    image_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    r = vk_fn.CreateImage(vk_device, &image_ci, NULL, &vk_palette_texture);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: CreateImage (palette) failed (%d)\n", (int)r);
        return false;
    }

    memset(&mem_req, 0, sizeof(mem_req));
    vk_fn.GetImageMemoryRequirements(vk_device, vk_palette_texture, &mem_req);

    mem_type = backend_vk_find_memory_type(mem_req.memoryTypeBits,
                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    if (mem_type == 0xFFFFFFFFu) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: no device-local memory type for palette\n");
        return false;
    }

    memset(&alloc_info, 0, sizeof(alloc_info));
    alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize  = mem_req.size;
    alloc_info.memoryTypeIndex = mem_type;

    r = vk_fn.AllocateMemory(vk_device, &alloc_info, NULL, &vk_palette_texture_memory);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: AllocateMemory (palette) failed (%d)\n", (int)r);
        return false;
    }

    r = vk_fn.BindImageMemory(vk_device, vk_palette_texture,
                              vk_palette_texture_memory, 0);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: BindImageMemory (palette) failed (%d)\n", (int)r);
        return false;
    }

    memset(&view_ci, 0, sizeof(view_ci));
    view_ci.sType            = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_ci.image            = vk_palette_texture;
    view_ci.viewType         = VK_IMAGE_VIEW_TYPE_2D;
    view_ci.format           = VK_FORMAT_R8G8B8A8_UNORM;
    view_ci.components.r     = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_ci.components.g     = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_ci.components.b     = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_ci.components.a     = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_ci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    view_ci.subresourceRange.baseMipLevel   = 0;
    view_ci.subresourceRange.levelCount     = 1;
    view_ci.subresourceRange.baseArrayLayer = 0;
    view_ci.subresourceRange.layerCount     = 1;

    r = vk_fn.CreateImageView(vk_device, &view_ci, NULL, &vk_palette_texture_view);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: CreateImageView (palette) failed (%d)\n", (int)r);
        return false;
    }

    /* ---------------------------------------------------------
     * Staging buffer.  HOST_VISIBLE | HOST_COHERENT, TRANSFER_
     * SRC.  Layout (Phase 4f):
     *   bytes [0 .. width*height-1]               = index data
     *   bytes [width*height .. +VK_PALETTE_BYTES] = palette
     * Mapped once here and kept mapped through context_destroy
     * -- per-frame uploads memcpy through vk_staging_ptr with
     * no map/unmap cost.  HOST_COHERENT means the GPU sees
     * host writes as soon as the next QueueSubmit happens; no
     * Flush calls.  At 1280x960 the buffer is ~1.25 MiB
     * (vs. ~5 MiB in Phase 4d's RGBA-per-pixel layout). */
    vk_staging_size = (VkDeviceSize)width * (VkDeviceSize)height
                    + (VkDeviceSize)VK_PALETTE_BYTES;

    /* Phase 5b-08 step 2a: ring-buffered staging.  Create N
     * identical slots; each frame writes into and reads
     * from its own slot via vk_staging_*[vk_active_slot]. */
    {
        unsigned si;
        for (si = 0; si < VK_FRAME_RING_DEPTH; si++) {
            memset(&buf_ci, 0, sizeof(buf_ci));
            buf_ci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            buf_ci.size        = vk_staging_size;
            buf_ci.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            buf_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            r = vk_fn.CreateBuffer(vk_device, &buf_ci, NULL, &vk_staging_buffer[si]);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR, "rhi-vk: CreateBuffer (staging slot %u) failed (%d)\n",
                           si, (int)r);
                return false;
            }

            memset(&mem_req, 0, sizeof(mem_req));
            vk_fn.GetBufferMemoryRequirements(vk_device, vk_staging_buffer[si], &mem_req);

            mem_type = backend_vk_find_memory_type(mem_req.memoryTypeBits,
                                                   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                                 | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            if (mem_type == 0xFFFFFFFFu) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: no HOST_VISIBLE|HOST_COHERENT memory for staging slot %u\n",
                           si);
                return false;
            }

            memset(&alloc_info, 0, sizeof(alloc_info));
            alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            alloc_info.allocationSize  = mem_req.size;
            alloc_info.memoryTypeIndex = mem_type;

            r = vk_fn.AllocateMemory(vk_device, &alloc_info, NULL, &vk_staging_memory[si]);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: AllocateMemory (staging slot %u) failed (%d)\n", si, (int)r);
                return false;
            }

            r = vk_fn.BindBufferMemory(vk_device, vk_staging_buffer[si],
                                       vk_staging_memory[si], 0);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: BindBufferMemory (staging slot %u) failed (%d)\n", si, (int)r);
                return false;
            }

            vk_staging_ptr[si] = NULL;
            r = vk_fn.MapMemory(vk_device, vk_staging_memory[si], 0,
                                VK_WHOLE_SIZE, 0, &vk_staging_ptr[si]);
            if (r != VK_SUCCESS || !vk_staging_ptr[si]) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: MapMemory (staging slot %u) failed (%d)\n", si, (int)r);
                return false;
            }

            /* Zero the staging buffer so the very first
             * CmdCopyBufferToImage doesn't sample
             * uninitialised host memory before the first
             * per-frame upload writes into it. */
            memset(vk_staging_ptr[si], 0, (size_t)vk_staging_size);
        }
    }

    /* ---------------------------------------------------------
     * Descriptor set layout (Phase 4g).  Three bindings,
     * matching textured_palette.comp's
     *   layout(set = 0, binding = 0) uniform sampler2D       u_index;
     *   layout(set = 0, binding = 1) uniform sampler2D       u_palette;
     *   layout(set = 0, binding = 2, rgba8) uniform writeonly image2D u_output;
     * stageFlags on the sampler bindings stay FRAGMENT |
     * COMPUTE -- COMPUTE because the compute pipeline reads
     * them, FRAGMENT preserved for a future Phase 4h
     * graphics pipeline that might want to sample the same
     * textures (e.g. an overlay pass that composites onto
     * the compute output).  The storage-image binding is
     * COMPUTE-only since fragment shaders write through
     * attachments, not storage descriptors. */
    memset(&dsl_bindings, 0, sizeof(dsl_bindings));
    dsl_bindings[0].binding         = 0;
    dsl_bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    dsl_bindings[0].descriptorCount = 1;
    dsl_bindings[0].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT
                                    | VK_SHADER_STAGE_COMPUTE_BIT;
    dsl_bindings[1].binding         = 1;
    dsl_bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    dsl_bindings[1].descriptorCount = 1;
    dsl_bindings[1].stageFlags      = VK_SHADER_STAGE_FRAGMENT_BIT
                                    | VK_SHADER_STAGE_COMPUTE_BIT;
    dsl_bindings[2].binding         = 2;
    dsl_bindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    dsl_bindings[2].descriptorCount = 1;
    dsl_bindings[2].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

    memset(&dsl_ci, 0, sizeof(dsl_ci));
    dsl_ci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dsl_ci.bindingCount = 3;
    dsl_ci.pBindings    = dsl_bindings;

    r = vk_fn.CreateDescriptorSetLayout(vk_device, &dsl_ci, NULL,
                                        &vk_descriptor_set_layout);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: CreateDescriptorSetLayout failed (%d)\n", (int)r);
        return false;
    }

    /* Descriptor pool sized for N compute sets (Phase
     * 5b-08 step 3a: ring-buffered) plus one per overlay
     * slot.  The compute path's storage-image binding
     * (binding 2) needs a per-slot vk_image_view, so each
     * ring slot gets its own descriptor set.  Each set
     * needs 2 sampler descriptors + 1 storage-image
     * descriptor; pool sizes scale accordingly. */
    memset(&dp_sizes, 0, sizeof(dp_sizes));
    dp_sizes[0].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    dp_sizes[0].descriptorCount = 2u * (VK_FRAME_RING_DEPTH + OVERLAY_SLOT_MAX);
    dp_sizes[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    dp_sizes[1].descriptorCount = (uint32_t)VK_FRAME_RING_DEPTH + OVERLAY_SLOT_MAX;

    memset(&dp_ci, 0, sizeof(dp_ci));
    dp_ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dp_ci.maxSets       = (uint32_t)VK_FRAME_RING_DEPTH + OVERLAY_SLOT_MAX;
    dp_ci.poolSizeCount = 2;
    dp_ci.pPoolSizes    = dp_sizes;

    r = vk_fn.CreateDescriptorPool(vk_device, &dp_ci, NULL, &vk_descriptor_pool);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: CreateDescriptorPool failed (%d)\n", (int)r);
        return false;
    }

    {
        VkDescriptorSetLayout ds_layouts[VK_FRAME_RING_DEPTH];
        unsigned              dsi;
        for (dsi = 0; dsi < VK_FRAME_RING_DEPTH; dsi++)
            ds_layouts[dsi] = vk_descriptor_set_layout;

        memset(&ds_alloc, 0, sizeof(ds_alloc));
        ds_alloc.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        ds_alloc.descriptorPool     = vk_descriptor_pool;
        ds_alloc.descriptorSetCount = VK_FRAME_RING_DEPTH;
        ds_alloc.pSetLayouts        = ds_layouts;

        r = vk_fn.AllocateDescriptorSets(vk_device, &ds_alloc, vk_descriptor_set);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: AllocateDescriptorSets failed (%d)\n", (int)r);
            return false;
        }
    }
    /* Per-slot overlay descriptor sets are allocated
     * lazily by backend_vk_upload_pic_slot when each slot
     * is first populated.  The pool reservation above
     * guarantees the allocations will succeed up to
     * OVERLAY_SLOT_MAX. */

    /* Point each binding at its image + (for the sampled
     * bindings) the shared sampler.  The imageLayout we
     * declare here is what the shader expects to find at
     * dispatch time.  record_frame's per-frame transitions
     * land vk_texture and vk_palette_texture in
     * SHADER_READ_ONLY_OPTIMAL for the compute reads, and
     * vk_image in GENERAL for the compute write.
     *
     * Phase 5b-08 step 3a: bindings 0 + 1 (vk_texture_view
     * and vk_palette_texture_view) are shared across all
     * ring slots; binding 2 (vk_image_view) is per-slot.
     * Bindings 0/1 image_info common; binding 2 has a
     * per-slot ds_image_view_storage entry that points at
     * the slot's vk_image_view. */
    memset(&ds_image_infos, 0, sizeof(ds_image_infos));
    ds_image_infos[0].sampler     = vk_sampler;
    ds_image_infos[0].imageView   = vk_texture_view;
    ds_image_infos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    ds_image_infos[1].sampler     = vk_sampler;
    ds_image_infos[1].imageView   = vk_palette_texture_view;
    ds_image_infos[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    /* ds_image_infos[2] (vk_image_view) is set per-slot
     * inside the loop below. */

    {
        VkDescriptorImageInfo ds_storage_infos[VK_FRAME_RING_DEPTH];
        VkWriteDescriptorSet  ds_all_writes[3 * VK_FRAME_RING_DEPTH];
        unsigned              dsi;
        unsigned              wi = 0;

        memset(ds_storage_infos, 0, sizeof(ds_storage_infos));
        memset(ds_all_writes,    0, sizeof(ds_all_writes));

        for (dsi = 0; dsi < VK_FRAME_RING_DEPTH; dsi++) {
            ds_storage_infos[dsi].sampler     = VK_NULL_HANDLE;
            ds_storage_infos[dsi].imageView   = vk_image_view[dsi];
            ds_storage_infos[dsi].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

            /* Binding 0: sampled vk_texture (shared). */
            ds_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            ds_all_writes[wi].dstSet          = vk_descriptor_set[dsi];
            ds_all_writes[wi].dstBinding      = 0;
            ds_all_writes[wi].dstArrayElement = 0;
            ds_all_writes[wi].descriptorCount = 1;
            ds_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            ds_all_writes[wi].pImageInfo      = &ds_image_infos[0];
            wi++;
            /* Binding 1: sampled vk_palette_texture (shared). */
            ds_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            ds_all_writes[wi].dstSet          = vk_descriptor_set[dsi];
            ds_all_writes[wi].dstBinding      = 1;
            ds_all_writes[wi].dstArrayElement = 0;
            ds_all_writes[wi].descriptorCount = 1;
            ds_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            ds_all_writes[wi].pImageInfo      = &ds_image_infos[1];
            wi++;
            /* Binding 2: storage vk_image_view (per-slot). */
            ds_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            ds_all_writes[wi].dstSet          = vk_descriptor_set[dsi];
            ds_all_writes[wi].dstBinding      = 2;
            ds_all_writes[wi].dstArrayElement = 0;
            ds_all_writes[wi].descriptorCount = 1;
            ds_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            ds_all_writes[wi].pImageInfo      = &ds_storage_infos[dsi];
            wi++;
        }

        vk_fn.UpdateDescriptorSets(vk_device, wi, ds_all_writes, 0, NULL);
    }

    /* ---------------------------------------------------------
     * Pipeline layout.  Includes the Phase 4c descriptor set
     * layout at set 0.  No push constants. */
    memset(&pl_ci, 0, sizeof(pl_ci));
    pl_ci.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pl_ci.setLayoutCount = 1;
    pl_ci.pSetLayouts    = &vk_descriptor_set_layout;

    r = vk_fn.CreatePipelineLayout(vk_device, &pl_ci, NULL, &vk_pipeline_layout);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: CreatePipelineLayout failed (%d)\n", (int)r);
        return false;
    }

    /* Compute pipeline.  No vertex / fragment / blend /
     * rasterizer / viewport state -- compute pipelines
     * carry just a single shader stage and the layout
     * (same vk_pipeline_layout that the graphics path used,
     * since it references the same DSL).  basePipelineHandle
     * VK_NULL_HANDLE because we don't derive from anything. */
    memset(&cs_stage, 0, sizeof(cs_stage));
    cs_stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    cs_stage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
    cs_stage.module = vk_cs_module;
    cs_stage.pName  = "main";

    memset(&cp_ci, 0, sizeof(cp_ci));
    cp_ci.sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    cp_ci.stage              = cs_stage;
    cp_ci.layout             = vk_pipeline_layout;
    cp_ci.basePipelineHandle = VK_NULL_HANDLE;
    cp_ci.basePipelineIndex  = -1;

    r = vk_fn.CreateComputePipelines(vk_device, VK_NULL_HANDLE,
                                     1, &cp_ci, NULL, &vk_compute_pipeline);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: CreateComputePipelines failed (%d)\n", (int)r);
        return false;
    }

    /* ---------------------------------------------------------
     * Phase 4h: overlay graphics path.  Render pass +
     * framebuffer + graphics pipeline + vertex buffer for
     * drawing 2D quads on top of the compute output.
     *
     * Shader modules. */
    memset(&sm_ci, 0, sizeof(sm_ci));
    sm_ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    sm_ci.codeSize = spv_overlay_quad_vs_size;
    sm_ci.pCode    = spv_overlay_quad_vs;
    r = vk_fn.CreateShaderModule(vk_device, &sm_ci, NULL, &vk_overlay_vs_module);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: CreateShaderModule (overlay vs) failed (%d)\n", (int)r);
        return false;
    }

    memset(&sm_ci, 0, sizeof(sm_ci));
    sm_ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    sm_ci.codeSize = spv_overlay_quad_fs_size;
    sm_ci.pCode    = spv_overlay_quad_fs;
    r = vk_fn.CreateShaderModule(vk_device, &sm_ci, NULL, &vk_overlay_fs_module);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: CreateShaderModule (overlay fs) failed (%d)\n", (int)r);
        return false;
    }

    /* Render pass.  One COLOR attachment (the same image
     * the compute dispatch wrote to), loadOp = LOAD so the
     * compute output is preserved.  initialLayout =
     * GENERAL receives the image straight from the compute
     * dispatch -- the render pass's own attachment-layout
     * transition takes it to COLOR_ATTACHMENT_OPTIMAL for
     * the subpass, then to SHADER_READ_ONLY_OPTIMAL via
     * finalLayout for the frontend sample.
     *
     * Subpass dependency EXTERNAL -> 0 carries the memory
     * dependency between the compute write and the
     * attachment access: srcStage = COMPUTE_SHADER waits
     * for the dispatch, srcAccess = SHADER_WRITE makes
     * those writes visible; dstStage =
     * COLOR_ATTACHMENT_OUTPUT and dstAccess =
     * COLOR_ATTACHMENT_WRITE | COLOR_ATTACHMENT_READ
     * cover both the LOAD (read) and the subpass draw
     * (write+read for blend).  This replaces the explicit
     * pipeline barrier Phase 4g used to issue between
     * compute and the (then nonexistent) downstream
     * pass. */
    memset(&ov_attachment, 0, sizeof(ov_attachment));
    ov_attachment.format         = VK_FORMAT_R8G8B8A8_UNORM;
    ov_attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    ov_attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
    ov_attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    ov_attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ov_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ov_attachment.initialLayout  = VK_IMAGE_LAYOUT_GENERAL;
    ov_attachment.finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    ov_color_ref.attachment = 0;
    ov_color_ref.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    memset(&ov_subpass, 0, sizeof(ov_subpass));
    ov_subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    ov_subpass.colorAttachmentCount = 1;
    ov_subpass.pColorAttachments    = &ov_color_ref;

    memset(&ov_dep, 0, sizeof(ov_dep));
    ov_dep.srcSubpass    = VK_SUBPASS_EXTERNAL;
    ov_dep.dstSubpass    = 0;
    ov_dep.srcStageMask  = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    ov_dep.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    ov_dep.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    ov_dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
                         | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

    memset(&ov_rp_ci, 0, sizeof(ov_rp_ci));
    ov_rp_ci.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    ov_rp_ci.attachmentCount = 1;
    ov_rp_ci.pAttachments    = &ov_attachment;
    ov_rp_ci.subpassCount    = 1;
    ov_rp_ci.pSubpasses      = &ov_subpass;
    ov_rp_ci.dependencyCount = 1;
    ov_rp_ci.pDependencies   = &ov_dep;

    r = vk_fn.CreateRenderPass(vk_device, &ov_rp_ci, NULL, &vk_overlay_render_pass);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: CreateRenderPass (overlay) failed (%d)\n", (int)r);
        return false;
    }

    /* Framebuffer.  Wraps the slot's vk_image_view (Phase
     * 5b-08 step 3a: per-slot now) the compute dispatch
     * writes to, so the overlay's draws land on top of the
     * compute output.  ov_fb_ci is otherwise identical
     * across slots; only the attachment changes. */
    {
        unsigned ovfi;
        for (ovfi = 0; ovfi < VK_FRAME_RING_DEPTH; ovfi++) {
            ov_fb_attachments[0] = vk_image_view[ovfi];
            memset(&ov_fb_ci, 0, sizeof(ov_fb_ci));
            ov_fb_ci.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            ov_fb_ci.renderPass      = vk_overlay_render_pass;
            ov_fb_ci.attachmentCount = 1;
            ov_fb_ci.pAttachments    = ov_fb_attachments;
            ov_fb_ci.width           = width;
            ov_fb_ci.height          = height;
            ov_fb_ci.layers          = 1;

            r = vk_fn.CreateFramebuffer(vk_device, &ov_fb_ci, NULL,
                                        &vk_overlay_framebuffer[ovfi]);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: CreateFramebuffer (overlay slot %u) failed (%d)\n",
                           ovfi, (int)r);
                return false;
            }
        }
    }

    /* Pipeline layout.  Phase 4i reuses vk_pipeline_layout
     * (the 3-binding compute layout) instead of creating a
     * separate empty one.  Both pipelines reference the
     * same DSL (binding 0 sampler u_index, binding 1
     * sampler u_palette, binding 2 storage_image
     * u_output), neither uses push constants -- so the
     * layouts are identical and one object covers both
     * uses.  The overlay shader ignores binding 2
     * (storage_image, COMPUTE-stage-only); the compute
     * shader ignores no bindings. */
    /* Graphics pipeline.
     *
     * Vertex input: one binding (binding 0) with stride
     * 24 bytes = sizeof(vec2) + sizeof(vec4); two
     * attributes (location 0 = R32G32_SFLOAT at offset 0
     * for position, location 1 = R32G32B32A32_SFLOAT at
     * offset 8 for colour).
     *
     * Input assembly: TRIANGLE_STRIP so 4 vertices
     * produce 2 triangles for a quad without an index
     * buffer.  Strip winding for a screen-aligned quad:
     *   v0 = bottom-left  (NDC -, +y in Vulkan = bottom)
     *   v1 = bottom-right
     *   v2 = top-left
     *   v3 = top-right
     * Note Vulkan NDC has y = -1 at the TOP of the
     * framebuffer; "bottom" above means larger y in NDC.
     *
     * Viewport / scissor: full vk_image extent; resolution
     * is fixed for the lifetime of the context (the
     * core option requires-restart), so static state is
     * fine, no VK_DYNAMIC_STATE_VIEWPORT needed.
     *
     * Rasterization: FILL, no culling (we don't care
     * about winding for 2D quads).
     *
     * Multisample: 1 sample.
     *
     * Color blend: standard alpha blending --
     *   color = src.rgb * src.a + dst.rgb * (1 - src.a)
     *   alpha = src.a       + dst.a * (1 - src.a)
     * so the overlay quad composites smoothly over the
     * compute output. */
    memset(&ov_stages[0], 0, sizeof(ov_stages[0]));
    ov_stages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ov_stages[0].stage  = VK_SHADER_STAGE_VERTEX_BIT;
    ov_stages[0].module = vk_overlay_vs_module;
    ov_stages[0].pName  = "main";
    memset(&ov_stages[1], 0, sizeof(ov_stages[1]));
    ov_stages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ov_stages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    ov_stages[1].module = vk_overlay_fs_module;
    ov_stages[1].pName  = "main";

    memset(&ov_vb_binding, 0, sizeof(ov_vb_binding));
    ov_vb_binding.binding   = 0;
    ov_vb_binding.stride    = 16;  /* vec2 position + vec2 uv */
    ov_vb_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    memset(&ov_vb_attribs, 0, sizeof(ov_vb_attribs));
    ov_vb_attribs[0].location = 0;
    ov_vb_attribs[0].binding  = 0;
    ov_vb_attribs[0].format   = VK_FORMAT_R32G32_SFLOAT;
    ov_vb_attribs[0].offset   = 0;
    ov_vb_attribs[1].location = 1;
    ov_vb_attribs[1].binding  = 0;
    ov_vb_attribs[1].format   = VK_FORMAT_R32G32_SFLOAT;
    ov_vb_attribs[1].offset   = 8;

    memset(&ov_vi, 0, sizeof(ov_vi));
    ov_vi.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    ov_vi.vertexBindingDescriptionCount   = 1;
    ov_vi.pVertexBindingDescriptions      = &ov_vb_binding;
    ov_vi.vertexAttributeDescriptionCount = 2;
    ov_vi.pVertexAttributeDescriptions    = ov_vb_attribs;

    memset(&ov_ia, 0, sizeof(ov_ia));
    ov_ia.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ov_ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;

    memset(&ov_viewport, 0, sizeof(ov_viewport));
    ov_viewport.x        = 0.0f;
    ov_viewport.y        = 0.0f;
    ov_viewport.width    = (float)width;
    ov_viewport.height   = (float)height;
    ov_viewport.minDepth = 0.0f;
    ov_viewport.maxDepth = 1.0f;

    memset(&ov_scissor, 0, sizeof(ov_scissor));
    ov_scissor.extent.width  = width;
    ov_scissor.extent.height = height;

    memset(&ov_vp, 0, sizeof(ov_vp));
    ov_vp.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ov_vp.viewportCount = 1;
    ov_vp.pViewports    = &ov_viewport;
    ov_vp.scissorCount  = 1;
    ov_vp.pScissors     = &ov_scissor;

    memset(&ov_rs, 0, sizeof(ov_rs));
    ov_rs.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    ov_rs.polygonMode = VK_POLYGON_MODE_FILL;
    ov_rs.cullMode    = VK_CULL_MODE_NONE;
    ov_rs.frontFace   = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    ov_rs.lineWidth   = 1.0f;

    memset(&ov_ms, 0, sizeof(ov_ms));
    ov_ms.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ov_ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    memset(&ov_cba, 0, sizeof(ov_cba));
    ov_cba.blendEnable         = VK_TRUE;
    ov_cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    ov_cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    ov_cba.colorBlendOp        = VK_BLEND_OP_ADD;
    ov_cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    ov_cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    ov_cba.alphaBlendOp        = VK_BLEND_OP_ADD;
    ov_cba.colorWriteMask      = VK_COLOR_COMPONENT_R_BIT
                               | VK_COLOR_COMPONENT_G_BIT
                               | VK_COLOR_COMPONENT_B_BIT
                               | VK_COLOR_COMPONENT_A_BIT;

    memset(&ov_cb, 0, sizeof(ov_cb));
    ov_cb.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ov_cb.attachmentCount = 1;
    ov_cb.pAttachments    = &ov_cba;

    memset(&ov_gp_ci, 0, sizeof(ov_gp_ci));
    ov_gp_ci.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    ov_gp_ci.stageCount          = 2;
    ov_gp_ci.pStages             = ov_stages;
    ov_gp_ci.pVertexInputState   = &ov_vi;
    ov_gp_ci.pInputAssemblyState = &ov_ia;
    ov_gp_ci.pViewportState      = &ov_vp;
    ov_gp_ci.pRasterizationState = &ov_rs;
    ov_gp_ci.pMultisampleState   = &ov_ms;
    ov_gp_ci.pColorBlendState    = &ov_cb;
    ov_gp_ci.layout              = vk_pipeline_layout;
    ov_gp_ci.renderPass          = vk_overlay_render_pass;
    ov_gp_ci.subpass             = 0;
    ov_gp_ci.basePipelineHandle  = VK_NULL_HANDLE;
    ov_gp_ci.basePipelineIndex   = -1;

    r = vk_fn.CreateGraphicsPipelines(vk_device, VK_NULL_HANDLE,
                                      1, &ov_gp_ci, NULL, &vk_overlay_pipeline);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: CreateGraphicsPipelines (overlay) failed (%d)\n", (int)r);
        return false;
    }

    /* Vertex buffer.  HOST_VISIBLE + HOST_COHERENT so we
     * can write straight to it from the CPU; persistently
     * mapped so the vk_overlay_vertex_buffer_ptr stays
     * valid for the lifetime of the resources.  Size = 4
     * vertices * 24 bytes = 96 bytes.
     *
     * Contents at create_resources time: one quad in the
     * upper-left of the screen (NDC x in [-0.9, -0.7], y
     * in [-0.9, -0.7]; Vulkan NDC y goes negative
     * upward), with a different bright colour at each
     * corner so the FS interpolation produces a visible
     * gradient.  Alpha = 0.5 so the underlying content
     * shows through.
     *
     * Phase 4i will reuse this buffer for dynamic per-
     * frame quad streams (the Draw_Pic / Draw_Character
     * intercept fills it with as many quads as the SW
     * 2D drawing path requested that frame); for now
     * it's static and written once. */
    /* Phase 5b-08 step 2e: ring-buffered overlay vertex
     * buffer.  N slots created in a single per-slot loop;
     * each gets its own buffer / memory / persistent
     * mapping. */
    {
        unsigned ovsi;
        for (ovsi = 0; ovsi < VK_FRAME_RING_DEPTH; ovsi++) {
            memset(&ov_vb_ci, 0, sizeof(ov_vb_ci));
            ov_vb_ci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            ov_vb_ci.size        = OVERLAY_DRAW_MAX * 4 * 16;
                                             /* OVERLAY_DRAW_MAX quads *
                                              * 4 verts/quad * 16 bytes/vert
                                              * (vec2 pos + vec2 uv) */
            ov_vb_ci.usage       = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            ov_vb_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            r = vk_fn.CreateBuffer(vk_device, &ov_vb_ci, NULL, &vk_overlay_vertex_buffer[ovsi]);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: CreateBuffer (overlay vertex slot %u) failed (%d)\n",
                           ovsi, (int)r);
                return false;
            }

            vk_fn.GetBufferMemoryRequirements(vk_device,
                                              vk_overlay_vertex_buffer[ovsi],
                                              &ov_vb_mem_req);

            mem_type = backend_vk_find_memory_type(
                ov_vb_mem_req.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
              | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            if (mem_type == 0xFFFFFFFFu) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: no HOST_VISIBLE | HOST_COHERENT memory for overlay vertex slot %u\n",
                           ovsi);
                return false;
            }

            memset(&ov_vb_alloc, 0, sizeof(ov_vb_alloc));
            ov_vb_alloc.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            ov_vb_alloc.allocationSize  = ov_vb_mem_req.size;
            ov_vb_alloc.memoryTypeIndex = mem_type;

            r = vk_fn.AllocateMemory(vk_device, &ov_vb_alloc, NULL,
                                     &vk_overlay_vertex_memory[ovsi]);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: AllocateMemory (overlay vertex slot %u) failed (%d)\n",
                           ovsi, (int)r);
                return false;
            }

            r = vk_fn.BindBufferMemory(vk_device, vk_overlay_vertex_buffer[ovsi],
                                       vk_overlay_vertex_memory[ovsi], 0);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: BindBufferMemory (overlay vertex slot %u) failed (%d)\n",
                           ovsi, (int)r);
                return false;
            }

            vk_overlay_vertex_ptr[ovsi] = NULL;
            r = vk_fn.MapMemory(vk_device, vk_overlay_vertex_memory[ovsi], 0,
                                ov_vb_ci.size, 0, &vk_overlay_vertex_ptr[ovsi]);
            if (r != VK_SUCCESS || !vk_overlay_vertex_ptr[ovsi]) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: MapMemory (overlay vertex slot %u) failed (%d)\n",
                           ovsi, (int)r);
                return false;
            }
        }
    }
    /* Buffer contents are written below after the demo
     * draw list is populated; the persistent mapping
     * lives until vkFreeMemory at teardown. */
    /* Don't UnmapMemory: HOST_COHERENT, persistent map
     * is fine (and required to avoid retracted memory
     * mapping invariants in some drivers).  vkFreeMemory
     * at teardown implicitly unmaps. */

    /* ---------------------------------------------------------
     * Phase 5b-02: GPU compute particle rasterizer resources.
     *
     * Order:
     *   1. vk_zbuffer image + memory + view.
     *   2. vk_particles_buffer (SSBO) + memory + map.
     *   3. vk_particles_zstaging (host-visible staging for
     *      d_pzbuffer uploads) + memory + map.
     *   4. vk_particles_cs_module from the precompiled SPIR-V.
     *   5. vk_particles_dsl (3-binding layout: SSBO + 2 storage
     *      images).
     *   6. vk_particles_pool + AllocateDescriptorSets ->
     *      vk_particles_set.
     *   7. UpdateDescriptorSets with the buffer + view handles.
     *   8. vk_particles_pipeline_layout (DSL + 96 B push constants).
     *   9. CreateComputePipelines -> vk_particles_pipeline.
     *
     * Each step on failure jumps to the function's "return
     * false" path; destroy_resources unwinds whatever got
     * built. */
    {
        VkImageCreateInfo            zb_ci;
        VkMemoryRequirements         zb_mem_req;
        VkMemoryAllocateInfo         zb_mem_ai;
        VkImageViewCreateInfo        zb_view_ci;
        VkBufferCreateInfo           pb_ci;
        VkMemoryRequirements         pb_mem_req;
        VkMemoryAllocateInfo         pb_mem_ai;
        VkBufferCreateInfo           zs_ci;
        VkMemoryRequirements         zs_mem_req;
        VkMemoryAllocateInfo         zs_mem_ai;
        VkShaderModuleCreateInfo     psm_ci;
        VkDescriptorSetLayoutBinding p_dsl_bindings[3];
        VkDescriptorSetLayoutCreateInfo p_dsl_ci;
        VkDescriptorPoolSize         p_pool_sizes[2];
        VkDescriptorPoolCreateInfo   p_pool_ci;
        VkDescriptorSetAllocateInfo  p_set_alloc;
        /* Phase 5b-08 step 2b: p_buf_info / p_writes are now
         * declared inside the per-slot loop further down
         * (p_buf_infos[N] / p_all_writes[3*N]) since the
         * writes are emitted N times -- one set's worth per
         * slot. */
        VkDescriptorImageInfo        p_img_info[2];
        VkPushConstantRange          p_push_range;
        VkPipelineLayoutCreateInfo   p_pl_ci;
        VkComputePipelineCreateInfo  p_cp_ci;
        VkPipelineShaderStageCreateInfo p_cs_stage;
        uint32_t                     zb_mem_type;
        uint32_t                     pb_mem_type;
        uint32_t                     zs_mem_type;

        /* 1. vk_zbuffer image (R32_UINT storage + transfer
         *    destination).  Phase 5b-04 widened the format
         *    from R16_UINT to R32_UINT so the particle
         *    shader can use imageAtomicMax for the Z-test
         *    (Vulkan 1.0 mandates atomic image ops on R32_
         *    UINT; R16_UINT atomics need a non-core
         *    extension).  Bit-pattern compatibility with
         *    d_pzbuffer's short is gone, but the staging
         *    upload below widens each short to uint32
         *    on the host before the GPU copy. */
        memset(&zb_ci, 0, sizeof(zb_ci));
        zb_ci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        zb_ci.imageType     = VK_IMAGE_TYPE_2D;
        zb_ci.format        = VK_FORMAT_R32_UINT;
        zb_ci.extent.width  = width;
        zb_ci.extent.height = height;
        zb_ci.extent.depth  = 1;
        zb_ci.mipLevels     = 1;
        zb_ci.arrayLayers   = 1;
        zb_ci.samples       = VK_SAMPLE_COUNT_1_BIT;
        zb_ci.tiling        = VK_IMAGE_TILING_OPTIMAL;
        zb_ci.usage         = VK_IMAGE_USAGE_TRANSFER_DST_BIT
                            | VK_IMAGE_USAGE_STORAGE_BIT;
        zb_ci.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        zb_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        r = vk_fn.CreateImage(vk_device, &zb_ci, NULL, &vk_zbuffer);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateImage (zbuffer) failed (%d)\n", (int)r);
            return false;
        }

        memset(&zb_mem_req, 0, sizeof(zb_mem_req));
        vk_fn.GetImageMemoryRequirements(vk_device, vk_zbuffer, &zb_mem_req);
        zb_mem_type = backend_vk_find_memory_type(
            zb_mem_req.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (zb_mem_type == UINT32_MAX) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: no DEVICE_LOCAL memory type for zbuffer\n");
            return false;
        }

        memset(&zb_mem_ai, 0, sizeof(zb_mem_ai));
        zb_mem_ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        zb_mem_ai.allocationSize  = zb_mem_req.size;
        zb_mem_ai.memoryTypeIndex = zb_mem_type;

        r = vk_fn.AllocateMemory(vk_device, &zb_mem_ai, NULL, &vk_zbuffer_memory);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: AllocateMemory (zbuffer) failed (%d)\n", (int)r);
            return false;
        }

        r = vk_fn.BindImageMemory(vk_device, vk_zbuffer, vk_zbuffer_memory, 0);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: BindImageMemory (zbuffer) failed (%d)\n", (int)r);
            return false;
        }

        memset(&zb_view_ci, 0, sizeof(zb_view_ci));
        zb_view_ci.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        zb_view_ci.image                           = vk_zbuffer;
        zb_view_ci.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
        zb_view_ci.format                          = VK_FORMAT_R32_UINT;
        zb_view_ci.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        zb_view_ci.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        zb_view_ci.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        zb_view_ci.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
        zb_view_ci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        zb_view_ci.subresourceRange.baseMipLevel   = 0;
        zb_view_ci.subresourceRange.levelCount     = 1;
        zb_view_ci.subresourceRange.baseArrayLayer = 0;
        zb_view_ci.subresourceRange.layerCount     = 1;

        r = vk_fn.CreateImageView(vk_device, &zb_view_ci, NULL, &vk_zbuffer_view);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateImageView (zbuffer) failed (%d)\n", (int)r);
            return false;
        }

        /* 2. vk_particles_buffer (SSBO).  Host-visible +
         *    coherent so dispatch_3d_particles can memcpy
         *    GpuParticle records directly with no Flush.
         *    Phase 5b-08 step 2b: ring-buffered (one SSBO
         *    per frame slot). */
        {
            unsigned psi;
            for (psi = 0; psi < VK_FRAME_RING_DEPTH; psi++) {
                memset(&pb_ci, 0, sizeof(pb_ci));
                pb_ci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                pb_ci.size        = VK_PARTICLES_BYTES;
                pb_ci.usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                pb_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                r = vk_fn.CreateBuffer(vk_device, &pb_ci, NULL, &vk_particles_buffer[psi]);
                if (r != VK_SUCCESS) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: CreateBuffer (particles SSBO slot %u) failed (%d)\n",
                               psi, (int)r);
                    return false;
                }

                vk_fn.GetBufferMemoryRequirements(vk_device,
                                                  vk_particles_buffer[psi], &pb_mem_req);
                pb_mem_type = backend_vk_find_memory_type(
                    pb_mem_req.memoryTypeBits,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                  | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                if (pb_mem_type == UINT32_MAX) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: no HOST_VISIBLE|COHERENT memory for particles slot %u\n",
                               psi);
                    return false;
                }

                memset(&pb_mem_ai, 0, sizeof(pb_mem_ai));
                pb_mem_ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                pb_mem_ai.allocationSize  = pb_mem_req.size;
                pb_mem_ai.memoryTypeIndex = pb_mem_type;

                r = vk_fn.AllocateMemory(vk_device, &pb_mem_ai, NULL, &vk_particles_memory[psi]);
                if (r != VK_SUCCESS) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: AllocateMemory (particles slot %u) failed (%d)\n",
                               psi, (int)r);
                    return false;
                }

                r = vk_fn.BindBufferMemory(vk_device, vk_particles_buffer[psi],
                                           vk_particles_memory[psi], 0);
                if (r != VK_SUCCESS) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: BindBufferMemory (particles slot %u) failed (%d)\n",
                               psi, (int)r);
                    return false;
                }

                r = vk_fn.MapMemory(vk_device, vk_particles_memory[psi],
                                    0, VK_WHOLE_SIZE, 0, &vk_particles_ptr[psi]);
                if (r != VK_SUCCESS) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: MapMemory (particles slot %u) failed (%d)\n",
                               psi, (int)r);
                    return false;
                }
            }
        }

        /* 3. vk_particles_zstaging.  Host-visible buffer
         *    sized to width*height*sizeof(uint32_t) for the
         *    per-frame d_pzbuffer upload.  Phase 5b-08
         *    step 2a: ring-buffered. */
        {
            unsigned zsi;
            for (zsi = 0; zsi < VK_FRAME_RING_DEPTH; zsi++) {
                memset(&zs_ci, 0, sizeof(zs_ci));
                zs_ci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                zs_ci.size        = (VkDeviceSize)width * (VkDeviceSize)height * 4u;
                zs_ci.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                zs_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                r = vk_fn.CreateBuffer(vk_device, &zs_ci, NULL, &vk_particles_zstaging[zsi]);
                if (r != VK_SUCCESS) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: CreateBuffer (zbuffer staging slot %u) failed (%d)\n",
                               zsi, (int)r);
                    return false;
                }

                vk_fn.GetBufferMemoryRequirements(vk_device,
                                                  vk_particles_zstaging[zsi], &zs_mem_req);
                zs_mem_type = backend_vk_find_memory_type(
                    zs_mem_req.memoryTypeBits,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                  | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                if (zs_mem_type == UINT32_MAX) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: no HOST_VISIBLE|COHERENT memory for zbuffer staging slot %u\n",
                               zsi);
                    return false;
                }

                memset(&zs_mem_ai, 0, sizeof(zs_mem_ai));
                zs_mem_ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                zs_mem_ai.allocationSize  = zs_mem_req.size;
                zs_mem_ai.memoryTypeIndex = zs_mem_type;

                r = vk_fn.AllocateMemory(vk_device, &zs_mem_ai, NULL,
                                         &vk_particles_zstaging_memory[zsi]);
                if (r != VK_SUCCESS) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: AllocateMemory (zbuffer staging slot %u) failed (%d)\n",
                               zsi, (int)r);
                    return false;
                }

                r = vk_fn.BindBufferMemory(vk_device, vk_particles_zstaging[zsi],
                                           vk_particles_zstaging_memory[zsi], 0);
                if (r != VK_SUCCESS) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: BindBufferMemory (zbuffer staging slot %u) failed (%d)\n",
                               zsi, (int)r);
                    return false;
                }

                r = vk_fn.MapMemory(vk_device, vk_particles_zstaging_memory[zsi],
                                    0, VK_WHOLE_SIZE, 0, &vk_particles_zstaging_ptr[zsi]);
                if (r != VK_SUCCESS) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: MapMemory (zbuffer staging slot %u) failed (%d)\n",
                               zsi, (int)r);
                    return false;
                }
            }
        }

        /* 4. Shader module from precompiled SPIR-V. */
        memset(&psm_ci, 0, sizeof(psm_ci));
        psm_ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        psm_ci.codeSize = spv_particles_cs_size;
        psm_ci.pCode    = spv_particles_cs;

        r = vk_fn.CreateShaderModule(vk_device, &psm_ci, NULL,
                                     &vk_particles_cs_module);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateShaderModule (particles) failed (%d)\n",
                       (int)r);
            return false;
        }

        /* 5. Descriptor set layout: SSBO + 2 storage images,
         *    all COMPUTE-only.  Distinct from vk_descriptor_
         *    set_layout (which is for the palette compute) --
         *    different bindings, different types. */
        memset(&p_dsl_bindings, 0, sizeof(p_dsl_bindings));
        p_dsl_bindings[0].binding         = 0;
        p_dsl_bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        p_dsl_bindings[0].descriptorCount = 1;
        p_dsl_bindings[0].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
        p_dsl_bindings[1].binding         = 1;
        p_dsl_bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        p_dsl_bindings[1].descriptorCount = 1;
        p_dsl_bindings[1].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
        p_dsl_bindings[2].binding         = 2;
        p_dsl_bindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        p_dsl_bindings[2].descriptorCount = 1;
        p_dsl_bindings[2].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

        memset(&p_dsl_ci, 0, sizeof(p_dsl_ci));
        p_dsl_ci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        p_dsl_ci.bindingCount = 3;
        p_dsl_ci.pBindings    = p_dsl_bindings;

        r = vk_fn.CreateDescriptorSetLayout(vk_device, &p_dsl_ci, NULL,
                                            &vk_particles_dsl);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateDescriptorSetLayout (particles) failed (%d)\n",
                       (int)r);
            return false;
        }

        /* 6. Descriptor pool + set allocation.  Phase 5b-08
         *    step 2b: pool sized for N sets (maxSets = N),
         *    poolSizes scaled by N so AllocateDescriptorSets
         *    can pull N sets out of the same pool in one
         *    call. */
        memset(&p_pool_sizes, 0, sizeof(p_pool_sizes));
        p_pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        p_pool_sizes[0].descriptorCount = 1u * VK_FRAME_RING_DEPTH;
        p_pool_sizes[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        p_pool_sizes[1].descriptorCount = 2u * VK_FRAME_RING_DEPTH;

        memset(&p_pool_ci, 0, sizeof(p_pool_ci));
        p_pool_ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        p_pool_ci.maxSets       = VK_FRAME_RING_DEPTH;
        p_pool_ci.poolSizeCount = 2;
        p_pool_ci.pPoolSizes    = p_pool_sizes;

        r = vk_fn.CreateDescriptorPool(vk_device, &p_pool_ci, NULL,
                                       &vk_particles_pool);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateDescriptorPool (particles) failed (%d)\n",
                       (int)r);
            return false;
        }

        /* AllocateDescriptorSets wants an array of N
         * layouts (one per requested set), even though all
         * N share the same layout handle. */
        {
            VkDescriptorSetLayout p_layouts[VK_FRAME_RING_DEPTH];
            unsigned              psi;
            for (psi = 0; psi < VK_FRAME_RING_DEPTH; psi++)
                p_layouts[psi] = vk_particles_dsl;

            memset(&p_set_alloc, 0, sizeof(p_set_alloc));
            p_set_alloc.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            p_set_alloc.descriptorPool     = vk_particles_pool;
            p_set_alloc.descriptorSetCount = VK_FRAME_RING_DEPTH;
            p_set_alloc.pSetLayouts        = p_layouts;

            r = vk_fn.AllocateDescriptorSets(vk_device, &p_set_alloc,
                                             vk_particles_set);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: AllocateDescriptorSets (particles) failed (%d)\n",
                           (int)r);
                return false;
            }
        }

        /* 7. Update each set with handles.  Binding 0
         *    differs per slot (the SSBO is ring-buffered);
         *    bindings 1 and 2 (storage images) are shared
         *    across slots since vk_texture / vk_zbuffer are
         *    still single-instance.  Storage images use
         *    imageLayout = GENERAL because that's the
         *    layout they'll be in during the dispatch
         *    (record_frame transitions them there before
         *    binding this set). */
        memset(&p_img_info, 0, sizeof(p_img_info));
        p_img_info[0].sampler     = VK_NULL_HANDLE;
        p_img_info[0].imageView   = vk_texture_view;
        p_img_info[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        p_img_info[1].sampler     = VK_NULL_HANDLE;
        p_img_info[1].imageView   = vk_zbuffer_view;
        p_img_info[1].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        {
            VkDescriptorBufferInfo p_buf_infos[VK_FRAME_RING_DEPTH];
            VkWriteDescriptorSet   p_all_writes[3 * VK_FRAME_RING_DEPTH];
            unsigned               psi;
            unsigned               wi = 0;

            memset(p_buf_infos, 0, sizeof(p_buf_infos));
            memset(p_all_writes, 0, sizeof(p_all_writes));

            for (psi = 0; psi < VK_FRAME_RING_DEPTH; psi++) {
                p_buf_infos[psi].buffer = vk_particles_buffer[psi];
                p_buf_infos[psi].offset = 0;
                p_buf_infos[psi].range  = VK_WHOLE_SIZE;

                p_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                p_all_writes[wi].dstSet          = vk_particles_set[psi];
                p_all_writes[wi].dstBinding      = 0;
                p_all_writes[wi].descriptorCount = 1;
                p_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                p_all_writes[wi].pBufferInfo     = &p_buf_infos[psi];
                wi++;
                p_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                p_all_writes[wi].dstSet          = vk_particles_set[psi];
                p_all_writes[wi].dstBinding      = 1;
                p_all_writes[wi].descriptorCount = 1;
                p_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                p_all_writes[wi].pImageInfo      = &p_img_info[0];
                wi++;
                p_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                p_all_writes[wi].dstSet          = vk_particles_set[psi];
                p_all_writes[wi].dstBinding      = 2;
                p_all_writes[wi].descriptorCount = 1;
                p_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                p_all_writes[wi].pImageInfo      = &p_img_info[1];
                wi++;
            }

            vk_fn.UpdateDescriptorSets(vk_device, wi, p_all_writes, 0, NULL);
        }

        /* 8. Pipeline layout: DSL + push constants. */
        memset(&p_push_range, 0, sizeof(p_push_range));
        p_push_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        p_push_range.offset     = 0;
        p_push_range.size       = sizeof(struct vk_particles_pc);

        memset(&p_pl_ci, 0, sizeof(p_pl_ci));
        p_pl_ci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        p_pl_ci.setLayoutCount         = 1;
        p_pl_ci.pSetLayouts            = &vk_particles_dsl;
        p_pl_ci.pushConstantRangeCount = 1;
        p_pl_ci.pPushConstantRanges    = &p_push_range;

        r = vk_fn.CreatePipelineLayout(vk_device, &p_pl_ci, NULL,
                                       &vk_particles_pipeline_layout);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreatePipelineLayout (particles) failed (%d)\n",
                       (int)r);
            return false;
        }

        /* 9. Compute pipeline. */
        memset(&p_cs_stage, 0, sizeof(p_cs_stage));
        p_cs_stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        p_cs_stage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
        p_cs_stage.module = vk_particles_cs_module;
        p_cs_stage.pName  = "main";

        memset(&p_cp_ci, 0, sizeof(p_cp_ci));
        p_cp_ci.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        p_cp_ci.stage  = p_cs_stage;
        p_cp_ci.layout = vk_particles_pipeline_layout;

        r = vk_fn.CreateComputePipelines(vk_device, VK_NULL_HANDLE,
                                         1, &p_cp_ci, NULL,
                                         &vk_particles_pipeline);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateComputePipelines (particles) failed (%d)\n",
                       (int)r);
            return false;
        }
    }

    /* ---------------------------------------------------------
     * Phase 5b-03: GPU compute water-warp resources.
     *
     * Order:
     *   1. vk_warp_table_buffer (UBO, DEVICE_LOCAL).
     *   2. Stage and upload intsintable into it via a one-shot
     *      host-visible buffer + CmdCopyBuffer in a one-time
     *      command submission.
     *   3. vk_warp_cs_module from precompiled SPIR-V.
     *   4. vk_warp_dsl (4 bindings: 2 sampled, 1 storage, 1 UBO).
     *   5. vk_warp_pool + AllocateDescriptorSets -> vk_warp_set.
     *   6. UpdateDescriptorSets with the four handles
     *      (vk_texture_view sampled, vk_palette_texture_view
     *      sampled, vk_image_view storage, vk_warp_table_buffer
     *      UBO).
     *   7. vk_warp_pipeline_layout (DSL + 32 B push constants).
     *   8. CreateComputePipelines -> vk_warp_pipeline.
     */
    {
        VkBufferCreateInfo            wt_ci;
        VkMemoryRequirements          wt_mem_req;
        VkMemoryAllocateInfo          wt_mem_ai;
        VkBufferCreateInfo            ws_ci;
        VkMemoryRequirements          ws_mem_req;
        VkMemoryAllocateInfo          ws_mem_ai;
        VkBuffer                      wt_staging;
        VkDeviceMemory                wt_staging_memory;
        void                         *wt_staging_ptr;
        VkCommandBuffer               wt_upload_cmd;
        VkCommandBufferAllocateInfo   wt_cmd_alloc;
        VkCommandBufferBeginInfo      wt_begin;
        VkBufferCopy                  wt_copy;
        VkSubmitInfo                  wt_si;
        VkShaderModuleCreateInfo      wsm_ci;
        VkDescriptorSetLayoutBinding  w_dsl_bindings[4];
        VkDescriptorSetLayoutCreateInfo w_dsl_ci;
        VkDescriptorPoolSize          w_pool_sizes[3];
        VkDescriptorPoolCreateInfo    w_pool_ci;
        VkDescriptorSetAllocateInfo   w_set_alloc;
        VkDescriptorImageInfo         w_img_info[3];
        VkDescriptorBufferInfo        w_buf_info;
        VkWriteDescriptorSet          w_writes[4];
        VkPushConstantRange           w_push_range;
        VkPipelineLayoutCreateInfo    w_pl_ci;
        VkComputePipelineCreateInfo   w_cp_ci;
        VkPipelineShaderStageCreateInfo w_cs_stage;
        uint32_t                      wt_mem_type;
        uint32_t                      ws_mem_type;
        int                           si;

        /* 1. vk_warp_table_buffer: UBO sized for 64 ivec4
         *    slots (4 bytes/lane * 4 lanes/slot * 64 slots
         *    = 1 KiB).  TRANSFER_DST so the one-shot
         *    upload can write to it.  DEVICE_LOCAL because
         *    it's read every frame by the warp dispatch
         *    and never modified by the host after init. */
        memset(&wt_ci, 0, sizeof(wt_ci));
        wt_ci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        wt_ci.size        = VK_WARP_TABLE_BYTES;
        wt_ci.usage       = VK_BUFFER_USAGE_TRANSFER_DST_BIT
                          | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        wt_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        r = vk_fn.CreateBuffer(vk_device, &wt_ci, NULL,
                               &vk_warp_table_buffer);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateBuffer (warp table UBO) failed (%d)\n",
                       (int)r);
            return false;
        }

        vk_fn.GetBufferMemoryRequirements(vk_device, vk_warp_table_buffer,
                                          &wt_mem_req);
        wt_mem_type = backend_vk_find_memory_type(
            wt_mem_req.memoryTypeBits,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (wt_mem_type == UINT32_MAX) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: no DEVICE_LOCAL memory for warp table UBO\n");
            return false;
        }

        memset(&wt_mem_ai, 0, sizeof(wt_mem_ai));
        wt_mem_ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        wt_mem_ai.allocationSize  = wt_mem_req.size;
        wt_mem_ai.memoryTypeIndex = wt_mem_type;

        r = vk_fn.AllocateMemory(vk_device, &wt_mem_ai, NULL,
                                 &vk_warp_table_memory);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: AllocateMemory (warp table UBO) failed (%d)\n",
                       (int)r);
            return false;
        }

        r = vk_fn.BindBufferMemory(vk_device, vk_warp_table_buffer,
                                   vk_warp_table_memory, 0);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: BindBufferMemory (warp table UBO) failed (%d)\n",
                       (int)r);
            return false;
        }

        /* 2. Stage + upload intsintable via a one-shot
         *    HOST_VISIBLE buffer + CmdCopyBuffer.  The
         *    table is 256 ints from R_InitTurb (which has
         *    already run by this point -- R_Init is called
         *    from libretro's retro_load_game ahead of the
         *    backend's context_reset).  Pack as 64 ivec4
         *    slots to match the shader's std140 layout
         *    (each int occupies its own 16-byte slot in
         *    std140; we pack 4 ints per slot using ivec4).
         */
        memset(&ws_ci, 0, sizeof(ws_ci));
        ws_ci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        ws_ci.size        = VK_WARP_TABLE_BYTES;
        ws_ci.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        ws_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        r = vk_fn.CreateBuffer(vk_device, &ws_ci, NULL, &wt_staging);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateBuffer (warp table staging) failed (%d)\n",
                       (int)r);
            return false;
        }

        vk_fn.GetBufferMemoryRequirements(vk_device, wt_staging, &ws_mem_req);
        ws_mem_type = backend_vk_find_memory_type(
            ws_mem_req.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (ws_mem_type == UINT32_MAX) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: no HOST_VISIBLE|COHERENT memory for warp staging\n");
            vk_fn.DestroyBuffer(vk_device, wt_staging, NULL);
            return false;
        }

        memset(&ws_mem_ai, 0, sizeof(ws_mem_ai));
        ws_mem_ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ws_mem_ai.allocationSize  = ws_mem_req.size;
        ws_mem_ai.memoryTypeIndex = ws_mem_type;

        r = vk_fn.AllocateMemory(vk_device, &ws_mem_ai, NULL, &wt_staging_memory);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: AllocateMemory (warp table staging) failed (%d)\n",
                       (int)r);
            vk_fn.DestroyBuffer(vk_device, wt_staging, NULL);
            return false;
        }

        r = vk_fn.BindBufferMemory(vk_device, wt_staging, wt_staging_memory, 0);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: BindBufferMemory (warp table staging) failed (%d)\n",
                       (int)r);
            vk_fn.FreeMemory(vk_device, wt_staging_memory, NULL);
            vk_fn.DestroyBuffer(vk_device, wt_staging, NULL);
            return false;
        }

        r = vk_fn.MapMemory(vk_device, wt_staging_memory,
                            0, VK_WHOLE_SIZE, 0, &wt_staging_ptr);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: MapMemory (warp table staging) failed (%d)\n",
                       (int)r);
            vk_fn.FreeMemory(vk_device, wt_staging_memory, NULL);
            vk_fn.DestroyBuffer(vk_device, wt_staging, NULL);
            return false;
        }

        /* Pack intsintable (256 ints) into 64 ivec4 slots.
         * Slot i holds entries [4i .. 4i+3].  std140 layout
         * in the shader matches: ivec4 packed[64] is 64
         * 16-byte slots, each containing four int lanes. */
        {
            int32_t *slot = (int32_t *)wt_staging_ptr;
            for (si = 0; si < (int)VK_WARP_TABLE_SIZE; si++)
                slot[si] = (int32_t)intsintable[si];
        }

        /* Allocate a one-shot command buffer to issue the
         * CopyBuffer (we already have vk_cmd_pool from up
         * above? -- no, vk_cmd_pool is created below this
         * block.  We need our own short-lived pool here,
         * or we can hand-roll the command buffer with a
         * temporary pool.  Simplest: create a temporary
         * single-use pool that's destroyed immediately
         * after submit + WaitIdle).
         *
         * Actually vk_cmd_pool IS created below, after
         * this block -- which means it doesn't exist yet
         * when we get here.  Defer the upload: write the
         * staging bytes here, but issue the copy at the
         * end of create_resources after vk_cmd_pool is
         * up.  That's what the next sub-block below does. */

        /* 3. Shader module. */
        memset(&wsm_ci, 0, sizeof(wsm_ci));
        wsm_ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        wsm_ci.codeSize = spv_warpscreen_cs_size;
        wsm_ci.pCode    = spv_warpscreen_cs;

        r = vk_fn.CreateShaderModule(vk_device, &wsm_ci, NULL,
                                     &vk_warp_cs_module);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateShaderModule (warp) failed (%d)\n",
                       (int)r);
            vk_fn.UnmapMemory(vk_device, wt_staging_memory);
            vk_fn.FreeMemory(vk_device, wt_staging_memory, NULL);
            vk_fn.DestroyBuffer(vk_device, wt_staging, NULL);
            return false;
        }

        /* 4. Descriptor set layout: 4 bindings, all
         *    COMPUTE-only.  Bindings 0-2 mirror the
         *    palette compute's set; binding 3 is the
         *    sin-table UBO. */
        memset(&w_dsl_bindings, 0, sizeof(w_dsl_bindings));
        w_dsl_bindings[0].binding         = 0;
        w_dsl_bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        w_dsl_bindings[0].descriptorCount = 1;
        w_dsl_bindings[0].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
        w_dsl_bindings[1].binding         = 1;
        w_dsl_bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        w_dsl_bindings[1].descriptorCount = 1;
        w_dsl_bindings[1].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
        w_dsl_bindings[2].binding         = 2;
        w_dsl_bindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        w_dsl_bindings[2].descriptorCount = 1;
        w_dsl_bindings[2].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
        w_dsl_bindings[3].binding         = 3;
        w_dsl_bindings[3].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        w_dsl_bindings[3].descriptorCount = 1;
        w_dsl_bindings[3].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

        memset(&w_dsl_ci, 0, sizeof(w_dsl_ci));
        w_dsl_ci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        w_dsl_ci.bindingCount = 4;
        w_dsl_ci.pBindings    = w_dsl_bindings;

        r = vk_fn.CreateDescriptorSetLayout(vk_device, &w_dsl_ci, NULL,
                                            &vk_warp_dsl);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateDescriptorSetLayout (warp) failed (%d)\n",
                       (int)r);
            vk_fn.UnmapMemory(vk_device, wt_staging_memory);
            vk_fn.FreeMemory(vk_device, wt_staging_memory, NULL);
            vk_fn.DestroyBuffer(vk_device, wt_staging, NULL);
            return false;
        }

        /* 5. Pool + N sets allocation (Phase 5b-08 step
         *    3a: ring-buffered because binding 2 = vk_
         *    image_view is per-slot). */
        memset(&w_pool_sizes, 0, sizeof(w_pool_sizes));
        w_pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        w_pool_sizes[0].descriptorCount = 2u * VK_FRAME_RING_DEPTH;
        w_pool_sizes[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        w_pool_sizes[1].descriptorCount = 1u * VK_FRAME_RING_DEPTH;
        w_pool_sizes[2].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        w_pool_sizes[2].descriptorCount = 1u * VK_FRAME_RING_DEPTH;

        memset(&w_pool_ci, 0, sizeof(w_pool_ci));
        w_pool_ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        w_pool_ci.maxSets       = VK_FRAME_RING_DEPTH;
        w_pool_ci.poolSizeCount = 3;
        w_pool_ci.pPoolSizes    = w_pool_sizes;

        r = vk_fn.CreateDescriptorPool(vk_device, &w_pool_ci, NULL,
                                       &vk_warp_pool);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateDescriptorPool (warp) failed (%d)\n",
                       (int)r);
            vk_fn.UnmapMemory(vk_device, wt_staging_memory);
            vk_fn.FreeMemory(vk_device, wt_staging_memory, NULL);
            vk_fn.DestroyBuffer(vk_device, wt_staging, NULL);
            return false;
        }

        {
            VkDescriptorSetLayout w_layouts[VK_FRAME_RING_DEPTH];
            unsigned              wsi;
            for (wsi = 0; wsi < VK_FRAME_RING_DEPTH; wsi++)
                w_layouts[wsi] = vk_warp_dsl;

            memset(&w_set_alloc, 0, sizeof(w_set_alloc));
            w_set_alloc.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            w_set_alloc.descriptorPool     = vk_warp_pool;
            w_set_alloc.descriptorSetCount = VK_FRAME_RING_DEPTH;
            w_set_alloc.pSetLayouts        = w_layouts;

            r = vk_fn.AllocateDescriptorSets(vk_device, &w_set_alloc, vk_warp_set);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: AllocateDescriptorSets (warp) failed (%d)\n",
                           (int)r);
                vk_fn.UnmapMemory(vk_device, wt_staging_memory);
                vk_fn.FreeMemory(vk_device, wt_staging_memory, NULL);
                vk_fn.DestroyBuffer(vk_device, wt_staging, NULL);
                return false;
            }
        }

        /* 6. Update set bindings.  vk_texture sampled via
         *    SHADER_READ_ONLY (the regular palette layout
         *    transition leaves it there; ditto vk_palette_
         *    texture).  vk_image storage via GENERAL.  The
         *    UBO uses the buffer info union member.
         *
         *    Phase 5b-08 step 3a: bindings 0/1/3 are shared
         *    across slots; binding 2 (vk_image_view) is per
         *    slot.  Emit 4*N writes from a single
         *    UpdateDescriptorSets call. */
        memset(&w_img_info, 0, sizeof(w_img_info));
        w_img_info[0].sampler     = vk_sampler;
        w_img_info[0].imageView   = vk_texture_view;
        w_img_info[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        w_img_info[1].sampler     = vk_sampler;
        w_img_info[1].imageView   = vk_palette_texture_view;
        w_img_info[1].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        /* w_img_info[2] (vk_image_view) populated per-slot
         * inside the write loop below. */

        memset(&w_buf_info, 0, sizeof(w_buf_info));
        w_buf_info.buffer = vk_warp_table_buffer;
        w_buf_info.offset = 0;
        w_buf_info.range  = VK_WHOLE_SIZE;

        {
            VkDescriptorImageInfo w_storage_infos[VK_FRAME_RING_DEPTH];
            VkWriteDescriptorSet  w_all_writes[4 * VK_FRAME_RING_DEPTH];
            unsigned              wsi;
            unsigned              wi = 0;

            memset(w_storage_infos, 0, sizeof(w_storage_infos));
            memset(w_all_writes,    0, sizeof(w_all_writes));

            for (wsi = 0; wsi < VK_FRAME_RING_DEPTH; wsi++) {
                w_storage_infos[wsi].sampler     = VK_NULL_HANDLE;
                w_storage_infos[wsi].imageView   = vk_image_view[wsi];
                w_storage_infos[wsi].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

                /* Binding 0: sampled vk_texture (shared). */
                w_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                w_all_writes[wi].dstSet          = vk_warp_set[wsi];
                w_all_writes[wi].dstBinding      = 0;
                w_all_writes[wi].descriptorCount = 1;
                w_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                w_all_writes[wi].pImageInfo      = &w_img_info[0];
                wi++;
                /* Binding 1: sampled vk_palette_texture (shared). */
                w_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                w_all_writes[wi].dstSet          = vk_warp_set[wsi];
                w_all_writes[wi].dstBinding      = 1;
                w_all_writes[wi].descriptorCount = 1;
                w_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                w_all_writes[wi].pImageInfo      = &w_img_info[1];
                wi++;
                /* Binding 2: storage vk_image_view (per-slot). */
                w_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                w_all_writes[wi].dstSet          = vk_warp_set[wsi];
                w_all_writes[wi].dstBinding      = 2;
                w_all_writes[wi].descriptorCount = 1;
                w_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                w_all_writes[wi].pImageInfo      = &w_storage_infos[wsi];
                wi++;
                /* Binding 3: warp_table UBO (shared, read-only). */
                w_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                w_all_writes[wi].dstSet          = vk_warp_set[wsi];
                w_all_writes[wi].dstBinding      = 3;
                w_all_writes[wi].descriptorCount = 1;
                w_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                w_all_writes[wi].pBufferInfo     = &w_buf_info;
                wi++;
            }

            vk_fn.UpdateDescriptorSets(vk_device, wi, w_all_writes, 0, NULL);
        }

        /* 7. Pipeline layout. */
        memset(&w_push_range, 0, sizeof(w_push_range));
        w_push_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        w_push_range.offset     = 0;
        w_push_range.size       = sizeof(struct vk_warp_pc);

        memset(&w_pl_ci, 0, sizeof(w_pl_ci));
        w_pl_ci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        w_pl_ci.setLayoutCount         = 1;
        w_pl_ci.pSetLayouts            = &vk_warp_dsl;
        w_pl_ci.pushConstantRangeCount = 1;
        w_pl_ci.pPushConstantRanges    = &w_push_range;

        r = vk_fn.CreatePipelineLayout(vk_device, &w_pl_ci, NULL,
                                       &vk_warp_pipeline_layout);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreatePipelineLayout (warp) failed (%d)\n",
                       (int)r);
            vk_fn.UnmapMemory(vk_device, wt_staging_memory);
            vk_fn.FreeMemory(vk_device, wt_staging_memory, NULL);
            vk_fn.DestroyBuffer(vk_device, wt_staging, NULL);
            return false;
        }

        /* 8. Compute pipeline. */
        memset(&w_cs_stage, 0, sizeof(w_cs_stage));
        w_cs_stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        w_cs_stage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
        w_cs_stage.module = vk_warp_cs_module;
        w_cs_stage.pName  = "main";

        memset(&w_cp_ci, 0, sizeof(w_cp_ci));
        w_cp_ci.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        w_cp_ci.stage  = w_cs_stage;
        w_cp_ci.layout = vk_warp_pipeline_layout;

        r = vk_fn.CreateComputePipelines(vk_device, VK_NULL_HANDLE,
                                         1, &w_cp_ci, NULL,
                                         &vk_warp_pipeline);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateComputePipelines (warp) failed (%d)\n",
                       (int)r);
            vk_fn.UnmapMemory(vk_device, wt_staging_memory);
            vk_fn.FreeMemory(vk_device, wt_staging_memory, NULL);
            vk_fn.DestroyBuffer(vk_device, wt_staging, NULL);
            return false;
        }

        /* Stash staging handles in file-static scope so
         * the post-cmd-pool block below can issue the
         * one-shot copy + tear them down.  Use simple
         * leak-style locals: we'll free them at the end
         * of create_resources after the upload is done. */
        (void)wt_upload_cmd;
        (void)wt_cmd_alloc;
        (void)wt_begin;
        (void)wt_copy;
        (void)wt_si;

        /* Move the staging handles to a temporary holding
         * spot via file-static globals -- this is a
         * one-shot upload, so don't bother with a full
         * struct; cast through static dummies that the
         * post-cmd-pool sub-block reads.  Keeping the
         * upload deferred avoids needing a second cmd
         * pool here. */
        vk_warp_table_staging         = wt_staging;
        vk_warp_table_staging_memory  = wt_staging_memory;
    }

    /* ---------------------------------------------------------
     * Phase 5b-05: GPU compute sprite rasterizer resources.
     *
     * Order:
     *   1. vk_sprite_vertex_buffer (host-mapped SSBO).
     *   2. vk_sprite_texture_buffer (host-mapped SSBO).
     *   3. vk_sprite_cs_module.
     *   4. vk_sprite_dsl (4 bindings: 2 storage images, 2 SSBOs).
     *   5. vk_sprite_pool + AllocateDescriptorSets ->
     *      vk_sprite_set.
     *   6. UpdateDescriptorSets with the four handles.
     *   7. vk_sprite_pipeline_layout (DSL + 48 B push
     *      constants).
     *   8. CreateComputePipelines -> vk_sprite_pipeline.
     */
    {
        VkBufferCreateInfo            sv_ci;
        VkMemoryRequirements          sv_mem_req;
        VkMemoryAllocateInfo          sv_mem_ai;
        VkBufferCreateInfo            st_ci;
        VkMemoryRequirements          st_mem_req;
        VkMemoryAllocateInfo          st_mem_ai;
        VkShaderModuleCreateInfo      ssm_ci;
        VkDescriptorSetLayoutBinding  s_dsl_bindings[4];
        VkDescriptorSetLayoutCreateInfo s_dsl_ci;
        VkDescriptorPoolSize          s_pool_sizes[2];
        VkDescriptorPoolCreateInfo    s_pool_ci;
        VkDescriptorSetAllocateInfo   s_set_alloc;
        VkDescriptorImageInfo         s_img_info[2];
        /* Phase 5b-08 step 2c: per-slot SSBO buf-infos and
         * 4*N writes are declared inside the per-slot
         * update scope further down. */
        VkPushConstantRange           s_push_range;
        VkPipelineLayoutCreateInfo    s_pl_ci;
        VkComputePipelineCreateInfo   s_cp_ci;
        VkPipelineShaderStageCreateInfo s_cs_stage;
        uint32_t                      sv_mem_type;
        uint32_t                      st_mem_type;

        /* 1. Vertex SSBO + 2. Texture SSBO.  Phase 5b-08
         * step 2c: ring-buffered, both pairs created in
         * the same per-slot loop so the
         * mem-type-select / alloc / bind / map dance
         * runs once per slot. */
        {
            unsigned ssi;
            for (ssi = 0; ssi < VK_FRAME_RING_DEPTH; ssi++) {
                memset(&sv_ci, 0, sizeof(sv_ci));
                sv_ci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                sv_ci.size        = VK_SPRITE_VERT_POOL_BYTES;
                sv_ci.usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                sv_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                r = vk_fn.CreateBuffer(vk_device, &sv_ci, NULL,
                                       &vk_sprite_vertex_buffer[ssi]);
                if (r != VK_SUCCESS) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: CreateBuffer (sprite vertex SSBO slot %u) failed (%d)\n",
                               ssi, (int)r);
                    return false;
                }

                vk_fn.GetBufferMemoryRequirements(vk_device,
                                                  vk_sprite_vertex_buffer[ssi], &sv_mem_req);
                sv_mem_type = backend_vk_find_memory_type(
                    sv_mem_req.memoryTypeBits,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                  | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                if (sv_mem_type == UINT32_MAX) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: no HOST_VISIBLE|COHERENT memory for sprite verts slot %u\n",
                               ssi);
                    return false;
                }

                memset(&sv_mem_ai, 0, sizeof(sv_mem_ai));
                sv_mem_ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                sv_mem_ai.allocationSize  = sv_mem_req.size;
                sv_mem_ai.memoryTypeIndex = sv_mem_type;

                r = vk_fn.AllocateMemory(vk_device, &sv_mem_ai, NULL,
                                         &vk_sprite_vertex_memory[ssi]);
                if (r != VK_SUCCESS) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: AllocateMemory (sprite verts slot %u) failed (%d)\n",
                               ssi, (int)r);
                    return false;
                }

                r = vk_fn.BindBufferMemory(vk_device, vk_sprite_vertex_buffer[ssi],
                                           vk_sprite_vertex_memory[ssi], 0);
                if (r != VK_SUCCESS) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: BindBufferMemory (sprite verts slot %u) failed (%d)\n",
                               ssi, (int)r);
                    return false;
                }

                r = vk_fn.MapMemory(vk_device, vk_sprite_vertex_memory[ssi],
                                    0, VK_WHOLE_SIZE, 0, &vk_sprite_vertex_ptr[ssi]);
                if (r != VK_SUCCESS) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: MapMemory (sprite verts slot %u) failed (%d)\n",
                               ssi, (int)r);
                    return false;
                }

                memset(&st_ci, 0, sizeof(st_ci));
                st_ci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                st_ci.size        = VK_SPRITE_TEX_POOL_BYTES;
                st_ci.usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                st_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                r = vk_fn.CreateBuffer(vk_device, &st_ci, NULL,
                                       &vk_sprite_texture_buffer[ssi]);
                if (r != VK_SUCCESS) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: CreateBuffer (sprite texture SSBO slot %u) failed (%d)\n",
                               ssi, (int)r);
                    return false;
                }

                vk_fn.GetBufferMemoryRequirements(vk_device,
                                                  vk_sprite_texture_buffer[ssi], &st_mem_req);
                st_mem_type = backend_vk_find_memory_type(
                    st_mem_req.memoryTypeBits,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                  | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                if (st_mem_type == UINT32_MAX) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: no HOST_VISIBLE|COHERENT memory for sprite tex slot %u\n",
                               ssi);
                    return false;
                }

                memset(&st_mem_ai, 0, sizeof(st_mem_ai));
                st_mem_ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                st_mem_ai.allocationSize  = st_mem_req.size;
                st_mem_ai.memoryTypeIndex = st_mem_type;

                r = vk_fn.AllocateMemory(vk_device, &st_mem_ai, NULL,
                                         &vk_sprite_texture_memory[ssi]);
                if (r != VK_SUCCESS) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: AllocateMemory (sprite tex slot %u) failed (%d)\n",
                               ssi, (int)r);
                    return false;
                }

                r = vk_fn.BindBufferMemory(vk_device, vk_sprite_texture_buffer[ssi],
                                           vk_sprite_texture_memory[ssi], 0);
                if (r != VK_SUCCESS) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: BindBufferMemory (sprite tex slot %u) failed (%d)\n",
                               ssi, (int)r);
                    return false;
                }

                r = vk_fn.MapMemory(vk_device, vk_sprite_texture_memory[ssi],
                                    0, VK_WHOLE_SIZE, 0, &vk_sprite_texture_ptr[ssi]);
                if (r != VK_SUCCESS) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: MapMemory (sprite tex slot %u) failed (%d)\n",
                               ssi, (int)r);
                    return false;
                }
            }
        }

        /* 3. Shader module. */
        memset(&ssm_ci, 0, sizeof(ssm_ci));
        ssm_ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        ssm_ci.codeSize = spv_sprite_cs_size;
        ssm_ci.pCode    = spv_sprite_cs;

        r = vk_fn.CreateShaderModule(vk_device, &ssm_ci, NULL,
                                     &vk_sprite_cs_module);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateShaderModule (sprite) failed (%d)\n",
                       (int)r);
            return false;
        }

        /* 4. DSL. */
        memset(&s_dsl_bindings, 0, sizeof(s_dsl_bindings));
        s_dsl_bindings[0].binding         = 0;
        s_dsl_bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        s_dsl_bindings[0].descriptorCount = 1;
        s_dsl_bindings[0].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
        s_dsl_bindings[1].binding         = 1;
        s_dsl_bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        s_dsl_bindings[1].descriptorCount = 1;
        s_dsl_bindings[1].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
        s_dsl_bindings[2].binding         = 2;
        s_dsl_bindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        s_dsl_bindings[2].descriptorCount = 1;
        s_dsl_bindings[2].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
        s_dsl_bindings[3].binding         = 3;
        s_dsl_bindings[3].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        s_dsl_bindings[3].descriptorCount = 1;
        s_dsl_bindings[3].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

        memset(&s_dsl_ci, 0, sizeof(s_dsl_ci));
        s_dsl_ci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        s_dsl_ci.bindingCount = 4;
        s_dsl_ci.pBindings    = s_dsl_bindings;

        r = vk_fn.CreateDescriptorSetLayout(vk_device, &s_dsl_ci, NULL,
                                            &vk_sprite_dsl);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateDescriptorSetLayout (sprite) failed (%d)\n",
                       (int)r);
            return false;
        }

        /* 5. Pool + N sets.  Phase 5b-08 step 2c: pool
         *    sized for N sets, AllocateDescriptorSets
         *    pulls all N out in one call. */
        memset(&s_pool_sizes, 0, sizeof(s_pool_sizes));
        s_pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        s_pool_sizes[0].descriptorCount = 2u * VK_FRAME_RING_DEPTH;
        s_pool_sizes[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        s_pool_sizes[1].descriptorCount = 2u * VK_FRAME_RING_DEPTH;

        memset(&s_pool_ci, 0, sizeof(s_pool_ci));
        s_pool_ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        s_pool_ci.maxSets       = VK_FRAME_RING_DEPTH;
        s_pool_ci.poolSizeCount = 2;
        s_pool_ci.pPoolSizes    = s_pool_sizes;

        r = vk_fn.CreateDescriptorPool(vk_device, &s_pool_ci, NULL,
                                       &vk_sprite_pool);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateDescriptorPool (sprite) failed (%d)\n",
                       (int)r);
            return false;
        }

        {
            VkDescriptorSetLayout s_layouts[VK_FRAME_RING_DEPTH];
            unsigned              ssi;
            for (ssi = 0; ssi < VK_FRAME_RING_DEPTH; ssi++)
                s_layouts[ssi] = vk_sprite_dsl;

            memset(&s_set_alloc, 0, sizeof(s_set_alloc));
            s_set_alloc.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            s_set_alloc.descriptorPool     = vk_sprite_pool;
            s_set_alloc.descriptorSetCount = VK_FRAME_RING_DEPTH;
            s_set_alloc.pSetLayouts        = s_layouts;

            r = vk_fn.AllocateDescriptorSets(vk_device, &s_set_alloc, vk_sprite_set);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: AllocateDescriptorSets (sprite) failed (%d)\n",
                           (int)r);
                return false;
            }
        }

        /* 6. Update set bindings.  Shared image bindings
         *    (0+1) reference the single-instance views;
         *    per-slot SSBO bindings (2+3) reference the
         *    matching slot's buffer.  4*N writes total. */
        memset(&s_img_info, 0, sizeof(s_img_info));
        s_img_info[0].sampler     = VK_NULL_HANDLE;
        s_img_info[0].imageView   = vk_texture_view;
        s_img_info[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        s_img_info[1].sampler     = VK_NULL_HANDLE;
        s_img_info[1].imageView   = vk_zbuffer_view;
        s_img_info[1].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        {
            VkDescriptorBufferInfo s_buf_infos[2 * VK_FRAME_RING_DEPTH];
            VkWriteDescriptorSet   s_all_writes[4 * VK_FRAME_RING_DEPTH];
            unsigned               ssi;
            unsigned               wi = 0;

            memset(s_buf_infos, 0, sizeof(s_buf_infos));
            memset(s_all_writes, 0, sizeof(s_all_writes));

            for (ssi = 0; ssi < VK_FRAME_RING_DEPTH; ssi++) {
                s_buf_infos[ssi * 2 + 0].buffer = vk_sprite_vertex_buffer[ssi];
                s_buf_infos[ssi * 2 + 0].offset = 0;
                s_buf_infos[ssi * 2 + 0].range  = VK_WHOLE_SIZE;
                s_buf_infos[ssi * 2 + 1].buffer = vk_sprite_texture_buffer[ssi];
                s_buf_infos[ssi * 2 + 1].offset = 0;
                s_buf_infos[ssi * 2 + 1].range  = VK_WHOLE_SIZE;

                s_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                s_all_writes[wi].dstSet          = vk_sprite_set[ssi];
                s_all_writes[wi].dstBinding      = 0;
                s_all_writes[wi].descriptorCount = 1;
                s_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                s_all_writes[wi].pImageInfo      = &s_img_info[0];
                wi++;
                s_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                s_all_writes[wi].dstSet          = vk_sprite_set[ssi];
                s_all_writes[wi].dstBinding      = 1;
                s_all_writes[wi].descriptorCount = 1;
                s_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                s_all_writes[wi].pImageInfo      = &s_img_info[1];
                wi++;
                s_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                s_all_writes[wi].dstSet          = vk_sprite_set[ssi];
                s_all_writes[wi].dstBinding      = 2;
                s_all_writes[wi].descriptorCount = 1;
                s_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                s_all_writes[wi].pBufferInfo     = &s_buf_infos[ssi * 2 + 0];
                wi++;
                s_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                s_all_writes[wi].dstSet          = vk_sprite_set[ssi];
                s_all_writes[wi].dstBinding      = 3;
                s_all_writes[wi].descriptorCount = 1;
                s_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                s_all_writes[wi].pBufferInfo     = &s_buf_infos[ssi * 2 + 1];
                wi++;
            }

            vk_fn.UpdateDescriptorSets(vk_device, wi, s_all_writes, 0, NULL);
        }

        /* 7. Pipeline layout. */
        memset(&s_push_range, 0, sizeof(s_push_range));
        s_push_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        s_push_range.offset     = 0;
        s_push_range.size       = sizeof(struct vk_sprite_pc);

        memset(&s_pl_ci, 0, sizeof(s_pl_ci));
        s_pl_ci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        s_pl_ci.setLayoutCount         = 1;
        s_pl_ci.pSetLayouts            = &vk_sprite_dsl;
        s_pl_ci.pushConstantRangeCount = 1;
        s_pl_ci.pPushConstantRanges    = &s_push_range;

        r = vk_fn.CreatePipelineLayout(vk_device, &s_pl_ci, NULL,
                                       &vk_sprite_pipeline_layout);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreatePipelineLayout (sprite) failed (%d)\n",
                       (int)r);
            return false;
        }

        /* 8. Compute pipeline. */
        memset(&s_cs_stage, 0, sizeof(s_cs_stage));
        s_cs_stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        s_cs_stage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
        s_cs_stage.module = vk_sprite_cs_module;
        s_cs_stage.pName  = "main";

        memset(&s_cp_ci, 0, sizeof(s_cp_ci));
        s_cp_ci.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        s_cp_ci.stage  = s_cs_stage;
        s_cp_ci.layout = vk_sprite_pipeline_layout;

        r = vk_fn.CreateComputePipelines(vk_device, VK_NULL_HANDLE,
                                         1, &s_cp_ci, NULL,
                                         &vk_sprite_pipeline);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateComputePipelines (sprite) failed (%d)\n",
                       (int)r);
            return false;
        }
    }

    /* ---------------------------------------------------------
     * Phase 5b-06: GPU compute alias-model rasterizer
     * resources.
     *
     * Order:
     *   1. Four host-mapped SSBOs (vertex / triangle /
     *      skin / colormap pools).
     *   2. vk_alias_cs_module from compiled SPIR-V.
     *   3. 6-binding DSL (2 storage images + 4 SSBOs).
     *   4. Pool + set + UpdateDescriptorSets.
     *   5. Pipeline layout (48 B push constants) +
     *      compute pipeline.
     */
    {
        VkBufferCreateInfo            av_ci, at_ci, as_ci, ac_ci;
        VkMemoryRequirements          av_req, at_req, as_req, ac_req;
        VkMemoryAllocateInfo          av_ai, at_ai, as_ai, ac_ai;
        VkShaderModuleCreateInfo      a_sm_ci;
        VkDescriptorSetLayoutBinding  a_bindings[6];
        VkDescriptorSetLayoutCreateInfo a_dsl_ci;
        VkDescriptorPoolSize          a_pool_sizes[2];
        VkDescriptorPoolCreateInfo    a_pool_ci;
        VkDescriptorSetAllocateInfo   a_set_alloc;
        VkDescriptorImageInfo         a_img_info[2];
        /* Phase 5b-08 step 2c: per-slot SSBO buf-infos and
         * 6*N writes are declared inside the per-slot
         * update scope further down. */
        VkPushConstantRange           a_push_range;
        VkPipelineLayoutCreateInfo    a_pl_ci;
        VkComputePipelineCreateInfo   a_cp_ci;
        VkPipelineShaderStageCreateInfo a_cs_stage;
        VkBuffer                     *bufs[4];
        VkDeviceMemory               *mems[4];
        void                        **ptrs[4];
        VkDeviceSize                  sizes[4];
        const char                   *names[4];
        VkBufferCreateInfo           *cis[4];
        VkMemoryRequirements         *reqs[4];
        VkMemoryAllocateInfo         *ais[4];
        uint32_t                      mt;
        int                           bi;

        /* 1. Four host-mapped SSBOs.  Phase 5b-08 step 2c:
         * ring-buffered.  Outer loop iterates ring slots;
         * inner loop iterates the four buffer kinds.
         * Pointers in bufs[] / mems[] / ptrs[] are
         * rebound to the active slot's storage each pass
         * so the inner loop body is unchanged from
         * pre-ring code. */
        sizes[0] = VK_ALIAS_VERT_POOL_BYTES;
        sizes[1] = VK_ALIAS_TRI_POOL_BYTES;
        sizes[2] = VK_ALIAS_SKIN_POOL_BYTES;
        sizes[3] = VK_ALIAS_CMAP_POOL_BYTES;
        names[0] = "alias vertex";
        names[1] = "alias triangle";
        names[2] = "alias skin";
        names[3] = "alias colormap";
        cis[0]   = &av_ci;
        cis[1]   = &at_ci;
        cis[2]   = &as_ci;
        cis[3]   = &ac_ci;
        reqs[0]  = &av_req;
        reqs[1]  = &at_req;
        reqs[2]  = &as_req;
        reqs[3]  = &ac_req;
        ais[0]   = &av_ai;
        ais[1]   = &at_ai;
        ais[2]   = &as_ai;
        ais[3]   = &ac_ai;

        {
            unsigned asi;
            for (asi = 0; asi < VK_FRAME_RING_DEPTH; asi++) {
                bufs[0]  = &vk_alias_vertex_buffer[asi];
                bufs[1]  = &vk_alias_triangle_buffer[asi];
                bufs[2]  = &vk_alias_skin_buffer[asi];
                bufs[3]  = &vk_alias_colormap_buffer[asi];
                mems[0]  = &vk_alias_vertex_memory[asi];
                mems[1]  = &vk_alias_triangle_memory[asi];
                mems[2]  = &vk_alias_skin_memory[asi];
                mems[3]  = &vk_alias_colormap_memory[asi];
                ptrs[0]  = &vk_alias_vertex_ptr[asi];
                ptrs[1]  = &vk_alias_triangle_ptr[asi];
                ptrs[2]  = &vk_alias_skin_ptr[asi];
                ptrs[3]  = &vk_alias_colormap_ptr[asi];

                for (bi = 0; bi < 4; bi++) {
                    memset(cis[bi], 0, sizeof(VkBufferCreateInfo));
                    cis[bi]->sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                    cis[bi]->size        = sizes[bi];
                    cis[bi]->usage       = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
                    cis[bi]->sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                    r = vk_fn.CreateBuffer(vk_device, cis[bi], NULL, bufs[bi]);
                    if (r != VK_SUCCESS) {
                        if (log_cb)
                            log_cb(RETRO_LOG_ERROR,
                                   "rhi-vk: CreateBuffer (%s slot %u) failed (%d)\n",
                                   names[bi], asi, (int)r);
                        return false;
                    }

                    vk_fn.GetBufferMemoryRequirements(vk_device, *bufs[bi], reqs[bi]);
                    mt = backend_vk_find_memory_type(
                        reqs[bi]->memoryTypeBits,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                      | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                    if (mt == UINT32_MAX) {
                        if (log_cb)
                            log_cb(RETRO_LOG_ERROR,
                                   "rhi-vk: no HOST_VISIBLE|COHERENT memory for %s slot %u\n",
                                   names[bi], asi);
                        return false;
                    }

                    memset(ais[bi], 0, sizeof(VkMemoryAllocateInfo));
                    ais[bi]->sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                    ais[bi]->allocationSize  = reqs[bi]->size;
                    ais[bi]->memoryTypeIndex = mt;

                    r = vk_fn.AllocateMemory(vk_device, ais[bi], NULL, mems[bi]);
                    if (r != VK_SUCCESS) {
                        if (log_cb)
                            log_cb(RETRO_LOG_ERROR,
                                   "rhi-vk: AllocateMemory (%s slot %u) failed (%d)\n",
                                   names[bi], asi, (int)r);
                        return false;
                    }

                    r = vk_fn.BindBufferMemory(vk_device, *bufs[bi], *mems[bi], 0);
                    if (r != VK_SUCCESS) {
                        if (log_cb)
                            log_cb(RETRO_LOG_ERROR,
                                   "rhi-vk: BindBufferMemory (%s slot %u) failed (%d)\n",
                                   names[bi], asi, (int)r);
                        return false;
                    }

                    r = vk_fn.MapMemory(vk_device, *mems[bi], 0, VK_WHOLE_SIZE,
                                        0, ptrs[bi]);
                    if (r != VK_SUCCESS) {
                        if (log_cb)
                            log_cb(RETRO_LOG_ERROR,
                                   "rhi-vk: MapMemory (%s slot %u) failed (%d)\n",
                                   names[bi], asi, (int)r);
                        return false;
                    }
                }
            }
        }

        /* 2. Shader module. */
        memset(&a_sm_ci, 0, sizeof(a_sm_ci));
        a_sm_ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        a_sm_ci.codeSize = spv_alias_cs_size;
        a_sm_ci.pCode    = spv_alias_cs;

        r = vk_fn.CreateShaderModule(vk_device, &a_sm_ci, NULL,
                                     &vk_alias_cs_module);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateShaderModule (alias) failed (%d)\n",
                       (int)r);
            return false;
        }

        /* 3. DSL: 2 storage images + 4 SSBOs. */
        memset(&a_bindings, 0, sizeof(a_bindings));
        a_bindings[0].binding         = 0;
        a_bindings[0].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        a_bindings[0].descriptorCount = 1;
        a_bindings[0].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
        a_bindings[1].binding         = 1;
        a_bindings[1].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        a_bindings[1].descriptorCount = 1;
        a_bindings[1].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
        a_bindings[2].binding         = 2;
        a_bindings[2].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        a_bindings[2].descriptorCount = 1;
        a_bindings[2].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
        a_bindings[3].binding         = 3;
        a_bindings[3].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        a_bindings[3].descriptorCount = 1;
        a_bindings[3].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
        a_bindings[4].binding         = 4;
        a_bindings[4].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        a_bindings[4].descriptorCount = 1;
        a_bindings[4].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
        a_bindings[5].binding         = 5;
        a_bindings[5].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        a_bindings[5].descriptorCount = 1;
        a_bindings[5].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;

        memset(&a_dsl_ci, 0, sizeof(a_dsl_ci));
        a_dsl_ci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        a_dsl_ci.bindingCount = 6;
        a_dsl_ci.pBindings    = a_bindings;

        r = vk_fn.CreateDescriptorSetLayout(vk_device, &a_dsl_ci, NULL,
                                            &vk_alias_dsl);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateDescriptorSetLayout (alias) failed (%d)\n",
                       (int)r);
            return false;
        }

        /* 4. Pool + N sets.  Phase 5b-08 step 2c. */
        memset(&a_pool_sizes, 0, sizeof(a_pool_sizes));
        a_pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        a_pool_sizes[0].descriptorCount = 2u * VK_FRAME_RING_DEPTH;
        a_pool_sizes[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        a_pool_sizes[1].descriptorCount = 4u * VK_FRAME_RING_DEPTH;

        memset(&a_pool_ci, 0, sizeof(a_pool_ci));
        a_pool_ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        a_pool_ci.maxSets       = VK_FRAME_RING_DEPTH;
        a_pool_ci.poolSizeCount = 2;
        a_pool_ci.pPoolSizes    = a_pool_sizes;

        r = vk_fn.CreateDescriptorPool(vk_device, &a_pool_ci, NULL,
                                       &vk_alias_pool);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateDescriptorPool (alias) failed (%d)\n",
                       (int)r);
            return false;
        }

        {
            VkDescriptorSetLayout a_layouts[VK_FRAME_RING_DEPTH];
            unsigned              asi;
            for (asi = 0; asi < VK_FRAME_RING_DEPTH; asi++)
                a_layouts[asi] = vk_alias_dsl;

            memset(&a_set_alloc, 0, sizeof(a_set_alloc));
            a_set_alloc.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            a_set_alloc.descriptorPool     = vk_alias_pool;
            a_set_alloc.descriptorSetCount = VK_FRAME_RING_DEPTH;
            a_set_alloc.pSetLayouts        = a_layouts;

            r = vk_fn.AllocateDescriptorSets(vk_device, &a_set_alloc, vk_alias_set);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: AllocateDescriptorSets (alias) failed (%d)\n",
                           (int)r);
                return false;
            }
        }

        /* Update sets: 2 image bindings + 4 SSBO bindings,
         * times N sets = 6*N writes.  Image bindings (0+1)
         * are shared across slots; SSBO bindings (2..5)
         * differ. */
        memset(&a_img_info, 0, sizeof(a_img_info));
        a_img_info[0].imageView   = vk_texture_view;
        a_img_info[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        a_img_info[1].imageView   = vk_zbuffer_view;
        a_img_info[1].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

        {
            VkDescriptorBufferInfo a_buf_infos[4 * VK_FRAME_RING_DEPTH];
            VkWriteDescriptorSet   a_all_writes[6 * VK_FRAME_RING_DEPTH];
            unsigned               asi;
            unsigned               wi = 0;

            memset(a_buf_infos, 0, sizeof(a_buf_infos));
            memset(a_all_writes, 0, sizeof(a_all_writes));

            for (asi = 0; asi < VK_FRAME_RING_DEPTH; asi++) {
                a_buf_infos[asi * 4 + 0].buffer = vk_alias_vertex_buffer[asi];
                a_buf_infos[asi * 4 + 0].offset = 0;
                a_buf_infos[asi * 4 + 0].range  = VK_WHOLE_SIZE;
                a_buf_infos[asi * 4 + 1].buffer = vk_alias_triangle_buffer[asi];
                a_buf_infos[asi * 4 + 1].offset = 0;
                a_buf_infos[asi * 4 + 1].range  = VK_WHOLE_SIZE;
                a_buf_infos[asi * 4 + 2].buffer = vk_alias_skin_buffer[asi];
                a_buf_infos[asi * 4 + 2].offset = 0;
                a_buf_infos[asi * 4 + 2].range  = VK_WHOLE_SIZE;
                a_buf_infos[asi * 4 + 3].buffer = vk_alias_colormap_buffer[asi];
                a_buf_infos[asi * 4 + 3].offset = 0;
                a_buf_infos[asi * 4 + 3].range  = VK_WHOLE_SIZE;

                /* Bindings 0+1: storage images, shared. */
                a_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                a_all_writes[wi].dstSet          = vk_alias_set[asi];
                a_all_writes[wi].dstBinding      = 0;
                a_all_writes[wi].descriptorCount = 1;
                a_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                a_all_writes[wi].pImageInfo      = &a_img_info[0];
                wi++;
                a_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                a_all_writes[wi].dstSet          = vk_alias_set[asi];
                a_all_writes[wi].dstBinding      = 1;
                a_all_writes[wi].descriptorCount = 1;
                a_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                a_all_writes[wi].pImageInfo      = &a_img_info[1];
                wi++;
                /* Bindings 2..5: SSBOs, per slot. */
                {
                    int bbi;
                    for (bbi = 0; bbi < 4; bbi++) {
                        a_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                        a_all_writes[wi].dstSet          = vk_alias_set[asi];
                        a_all_writes[wi].dstBinding      = (uint32_t)(2 + bbi);
                        a_all_writes[wi].descriptorCount = 1;
                        a_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                        a_all_writes[wi].pBufferInfo     = &a_buf_infos[asi * 4 + bbi];
                        wi++;
                    }
                }
            }

            vk_fn.UpdateDescriptorSets(vk_device, wi, a_all_writes, 0, NULL);
        }

        /* 5. Pipeline layout + pipeline. */
        memset(&a_push_range, 0, sizeof(a_push_range));
        a_push_range.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        a_push_range.offset     = 0;
        a_push_range.size       = sizeof(struct vk_alias_pc);

        memset(&a_pl_ci, 0, sizeof(a_pl_ci));
        a_pl_ci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        a_pl_ci.setLayoutCount         = 1;
        a_pl_ci.pSetLayouts            = &vk_alias_dsl;
        a_pl_ci.pushConstantRangeCount = 1;
        a_pl_ci.pPushConstantRanges    = &a_push_range;

        r = vk_fn.CreatePipelineLayout(vk_device, &a_pl_ci, NULL,
                                       &vk_alias_pipeline_layout);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreatePipelineLayout (alias) failed (%d)\n",
                       (int)r);
            return false;
        }

        memset(&a_cs_stage, 0, sizeof(a_cs_stage));
        a_cs_stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        a_cs_stage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
        a_cs_stage.module = vk_alias_cs_module;
        a_cs_stage.pName  = "main";

        memset(&a_cp_ci, 0, sizeof(a_cp_ci));
        a_cp_ci.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        a_cp_ci.stage  = a_cs_stage;
        a_cp_ci.layout = vk_alias_pipeline_layout;

        r = vk_fn.CreateComputePipelines(vk_device, VK_NULL_HANDLE,
                                         1, &a_cp_ci, NULL,
                                         &vk_alias_pipeline);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateComputePipelines (alias) failed (%d)\n",
                       (int)r);
            return false;
        }
    }

    /* ---------------------------------------------------------
     * Command pool + buffer.  RESET_COMMAND_BUFFER_BIT lets
     * vkBeginCommandBuffer implicitly reset the buffer at the
     * top of every per-frame recording (Phase 4e);
     * vkResetCommandBuffer is never called explicitly. */
    memset(&pool_ci, 0, sizeof(pool_ci));
    pool_ci.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    pool_ci.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    pool_ci.queueFamilyIndex = vk_queue_family;

    r = vk_fn.CreateCommandPool(vk_device, &pool_ci, NULL, &vk_cmd_pool);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR, "rhi-vk: CreateCommandPool failed (%d)\n", (int)r);
        return false;
    }

    /* Allocate VK_FRAME_RING_DEPTH command buffers from
     * the pool, one per frame-ring slot.  Single API call
     * is sufficient; commandBufferCount=N fills the array
     * contiguously.  Stash each into its vk_frame_ctx[i]
     * slot and leave vk_cmd_buffer pointing at slot 0 so
     * the very first frame (before begin_frame's first
     * rotation) has a valid handle. */
    {
        VkCommandBuffer cbs[VK_FRAME_RING_DEPTH];
        unsigned        si;

        memset(&cmd_alloc, 0, sizeof(cmd_alloc));
        cmd_alloc.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_alloc.commandPool        = vk_cmd_pool;
        cmd_alloc.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_alloc.commandBufferCount = VK_FRAME_RING_DEPTH;

        r = vk_fn.AllocateCommandBuffers(vk_device, &cmd_alloc, cbs);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: AllocateCommandBuffers (ring) failed (%d)\n", (int)r);
            return false;
        }

        for (si = 0; si < VK_FRAME_RING_DEPTH; si++) {
            VkFenceCreateInfo     fi;
            VkSemaphoreCreateInfo sem_ci;
            memset(&fi, 0, sizeof(fi));
            fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            /* SIGNALED so the first time begin_frame waits
             * on this slot's fence (before the slot has
             * ever been submitted into) the wait is
             * instant rather than deadlocking. */
            fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
            r = vk_fn.CreateFence(vk_device, &fi, NULL,
                                  &vk_frame_ctx[si].done_fence);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: CreateFence (ring slot %u) failed (%d)\n",
                           si, (int)r);
                return false;
            }
            /* Phase 5b-08 step 3b: render-done semaphore.
             * Created unsignaled; QueueSubmit in end_frame
             * signals it, set_image hands it to the
             * frontend, and the frontend resets it when it
             * waits.  Binary semaphore (the default; no
             * VkSemaphoreTypeCreateInfo pNext). */
            memset(&sem_ci, 0, sizeof(sem_ci));
            sem_ci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            r = vk_fn.CreateSemaphore(vk_device, &sem_ci, NULL,
                                      &vk_frame_ctx[si].render_done_semaphore);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: CreateSemaphore (ring slot %u) failed (%d)\n",
                           si, (int)r);
                return false;
            }
            vk_frame_ctx[si].cmd_buffer = cbs[si];
        }

        /* Bootstrap vk_cmd_buffer with slot 0's CB so any
         * pre-first-frame recording path (if any future
         * code goes that route) sees a valid handle.
         * begin_frame will reassign this each frame. */
        vk_cmd_buffer  = vk_frame_ctx[0].cmd_buffer;
        vk_active_slot = 0;

        /* Phase 5b-08 step 3b: dedicated upload-path fence.
         * Created UNSIGNALED -- end_uploads submits with it
         * as the signal target, waits on it, then resets.
         * Single fence is fine because end_uploads is
         * synchronous (caller waits before returning) so
         * only one upload submit is ever in flight. */
        {
            VkFenceCreateInfo ufi;
            memset(&ufi, 0, sizeof(ufi));
            ufi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            ufi.flags = 0;
            r = vk_fn.CreateFence(vk_device, &ufi, NULL, &vk_upload_fence);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: CreateFence (upload) failed (%d)\n", (int)r);
                return false;
            }
        }
    }

    /* No per-frame recording here -- backend_vk_record_frame
     * fills the buffer fresh each time end_frame runs.  The
     * buffer is in initial state at this point; the first
     * recording will transition it to executable via the
     * implicit reset that vkBeginCommandBuffer does when the
     * pool was created with RESET_COMMAND_BUFFER_BIT. */

    /* Phase 5b-03: one-shot upload of intsintable into
     * vk_warp_table_buffer (DEVICE_LOCAL).  Uses
     * vk_cmd_buffer (just allocated above) for a single-
     * use CopyBuffer + QueueWaitIdle, then frees the
     * staging buffer.  The table doesn't change after
     * R_InitTurb so this is a once-per-context-reset
     * cost. */
    if (vk_warp_table_staging != VK_NULL_HANDLE) {
        VkCommandBufferBeginInfo wt_begin;
        VkBufferCopy             wt_copy;
        VkSubmitInfo             wt_si;
        VkBufferMemoryBarrier    wt_bar;

        memset(&wt_begin, 0, sizeof(wt_begin));
        wt_begin.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        wt_begin.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        r = vk_fn.BeginCommandBuffer(vk_cmd_buffer, &wt_begin);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: warp-table upload BeginCommandBuffer failed (%d)\n",
                       (int)r);
            return false;
        }

        memset(&wt_copy, 0, sizeof(wt_copy));
        wt_copy.size = VK_WARP_TABLE_BYTES;

        vk_fn.CmdCopyBuffer(vk_cmd_buffer,
                            vk_warp_table_staging,
                            vk_warp_table_buffer,
                            1, &wt_copy);

        /* TRANSFER_WRITE -> UNIFORM_READ for the UBO so
         * the first frame's warp dispatch sees the data
         * coherently.  Buffer barrier so the table-
         * specific dependency is clear; alternative would
         * be a global memory barrier. */
        memset(&wt_bar, 0, sizeof(wt_bar));
        wt_bar.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        wt_bar.srcAccessMask       = VK_ACCESS_TRANSFER_WRITE_BIT;
        wt_bar.dstAccessMask       = VK_ACCESS_UNIFORM_READ_BIT;
        wt_bar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        wt_bar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        wt_bar.buffer              = vk_warp_table_buffer;
        wt_bar.offset              = 0;
        wt_bar.size                = VK_WARP_TABLE_BYTES;

        vk_fn.CmdPipelineBarrier(vk_cmd_buffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 0,
                                 0, NULL,
                                 1, &wt_bar,
                                 0, NULL);

        r = vk_fn.EndCommandBuffer(vk_cmd_buffer);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: warp-table upload EndCommandBuffer failed (%d)\n",
                       (int)r);
            return false;
        }

        memset(&wt_si, 0, sizeof(wt_si));
        wt_si.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        wt_si.commandBufferCount = 1;
        wt_si.pCommandBuffers    = &vk_cmd_buffer;

        r = vk_fn.QueueSubmit(vk_queue, 1, &wt_si, VK_NULL_HANDLE);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: warp-table upload QueueSubmit failed (%d)\n",
                       (int)r);
            return false;
        }

        vk_fn.QueueWaitIdle(vk_queue);

        /* Staging buffer's job is done; free it.  Mapped
         * memory is implicitly unmapped by FreeMemory. */
        vk_fn.UnmapMemory(vk_device, vk_warp_table_staging_memory);
        vk_fn.DestroyBuffer(vk_device, vk_warp_table_staging, NULL);
        vk_fn.FreeMemory(vk_device, vk_warp_table_staging_memory, NULL);
        vk_warp_table_staging         = VK_NULL_HANDLE;
        vk_warp_table_staging_memory  = VK_NULL_HANDLE;
    }

    /* Phase 5b-07a step 1: sky texture storage.  Stands up
     * two R8_UINT 128x128 storage images (front masked
     * overlay + back opaque underlay) and a 32 KiB
     * host-coherent staging buffer kept permanently mapped
     * for cheap re-upload on level change.
     *
     * No initial data upload here -- if R_InitSky already
     * fired (the common case: level loaded before context
     * stood up) vk_sky_cache_populated is true and we just
     * re-arm vk_sky_upload_pending so record_frame picks it
     * up on first run.  If R_InitSky hasn't fired yet
     * (rare; happens if Quake decides to switch sky textures
     * mid-game) the next R_InitSky call sets pending
     * directly.  Either way the upload itself lives in
     * record_frame, sharing the per-frame command buffer.
     *
     * Consumed by sky.comp in Phase 5b-07a commit 3; commits
     * 1 and 2 leave these images at GENERAL layout with valid
     * contents but no pipeline bound to them yet. */
    {
        VkImageCreateInfo       sk_ci;
        VkMemoryRequirements    sk_mem_req;
        VkMemoryAllocateInfo    sk_mem_ai;
        VkImageViewCreateInfo   sk_view_ci;
        VkBufferCreateInfo      ss_ci;
        VkMemoryRequirements    ss_mem_req;
        VkMemoryAllocateInfo    ss_mem_ai;
        uint32_t                sk_mem_type;
        uint32_t                ss_mem_type;
        int                     pass;

        /* Two iterations: pass 0 = front, pass 1 = back.
         * The two images are configurationally identical;
         * loop avoids 80 lines of paste. */
        for (pass = 0; pass < 2; pass++) {
            VkImage        *img_p   = (pass == 0) ? &vk_sky_front_image
                                                  : &vk_sky_back_image;
            VkDeviceMemory *mem_p   = (pass == 0) ? &vk_sky_front_memory
                                                  : &vk_sky_back_memory;
            VkImageView    *view_p  = (pass == 0) ? &vk_sky_front_view
                                                  : &vk_sky_back_view;
            const char     *label   = (pass == 0) ? "sky-front" : "sky-back";

            memset(&sk_ci, 0, sizeof(sk_ci));
            sk_ci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            sk_ci.imageType     = VK_IMAGE_TYPE_2D;
            sk_ci.format        = VK_FORMAT_R8_UINT;
            sk_ci.extent.width  = VK_SKY_TEXEL_W;
            sk_ci.extent.height = VK_SKY_TEXEL_H;
            sk_ci.extent.depth  = 1;
            sk_ci.mipLevels     = 1;
            sk_ci.arrayLayers   = 1;
            sk_ci.samples       = VK_SAMPLE_COUNT_1_BIT;
            sk_ci.tiling        = VK_IMAGE_TILING_OPTIMAL;
            /* STORAGE for imageLoad from sky.comp (commit 3),
             * TRANSFER_DST for the per-frame staging copy. */
            sk_ci.usage         = VK_IMAGE_USAGE_STORAGE_BIT
                                | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            sk_ci.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
            sk_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            r = vk_fn.CreateImage(vk_device, &sk_ci, NULL, img_p);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: CreateImage (%s) failed (%d)\n",
                           label, (int)r);
                return false;
            }

            vk_fn.GetImageMemoryRequirements(vk_device, *img_p, &sk_mem_req);
            sk_mem_type = backend_vk_find_memory_type(sk_mem_req.memoryTypeBits,
                                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            if (sk_mem_type == 0xFFFFFFFFu) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: no DEVICE_LOCAL memory type for %s\n",
                           label);
                return false;
            }

            memset(&sk_mem_ai, 0, sizeof(sk_mem_ai));
            sk_mem_ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            sk_mem_ai.allocationSize  = sk_mem_req.size;
            sk_mem_ai.memoryTypeIndex = sk_mem_type;

            r = vk_fn.AllocateMemory(vk_device, &sk_mem_ai, NULL, mem_p);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: AllocateMemory (%s) failed (%d)\n",
                           label, (int)r);
                return false;
            }

            r = vk_fn.BindImageMemory(vk_device, *img_p, *mem_p, 0);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: BindImageMemory (%s) failed (%d)\n",
                           label, (int)r);
                return false;
            }

            memset(&sk_view_ci, 0, sizeof(sk_view_ci));
            sk_view_ci.sType        = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            sk_view_ci.image        = *img_p;
            sk_view_ci.viewType     = VK_IMAGE_VIEW_TYPE_2D;
            sk_view_ci.format       = VK_FORMAT_R8_UINT;
            sk_view_ci.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            sk_view_ci.subresourceRange.baseMipLevel   = 0;
            sk_view_ci.subresourceRange.levelCount     = 1;
            sk_view_ci.subresourceRange.baseArrayLayer = 0;
            sk_view_ci.subresourceRange.layerCount     = 1;

            r = vk_fn.CreateImageView(vk_device, &sk_view_ci, NULL, view_p);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: CreateImageView (%s) failed (%d)\n",
                           label, (int)r);
                return false;
            }
        }

        /* Sky staging buffer: 32 KiB host-visible host-
         * coherent, permanently mapped.  Lives for the
         * duration of vk_resources_ready (unlike the warp
         * table staging which is a one-shot).  Re-used on
         * every sky upload (rare: per-level).  Phase 5b-08
         * step 2d: ring-buffered. */
        {
            unsigned skssi;
            for (skssi = 0; skssi < VK_FRAME_RING_DEPTH; skssi++) {
                memset(&ss_ci, 0, sizeof(ss_ci));
                ss_ci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
                ss_ci.size        = VK_SKY_STAGING_BYTES;
                ss_ci.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                ss_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

                r = vk_fn.CreateBuffer(vk_device, &ss_ci, NULL, &vk_sky_staging[skssi]);
                if (r != VK_SUCCESS) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: CreateBuffer (sky staging slot %u) failed (%d)\n",
                               skssi, (int)r);
                    return false;
                }

                vk_fn.GetBufferMemoryRequirements(vk_device, vk_sky_staging[skssi], &ss_mem_req);
                ss_mem_type = backend_vk_find_memory_type(ss_mem_req.memoryTypeBits,
                                                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                                          | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                if (ss_mem_type == 0xFFFFFFFFu) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: no HOST_VISIBLE|HOST_COHERENT memory type for sky staging slot %u\n",
                               skssi);
                    return false;
                }

                memset(&ss_mem_ai, 0, sizeof(ss_mem_ai));
                ss_mem_ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                ss_mem_ai.allocationSize  = ss_mem_req.size;
                ss_mem_ai.memoryTypeIndex = ss_mem_type;

                r = vk_fn.AllocateMemory(vk_device, &ss_mem_ai, NULL,
                                         &vk_sky_staging_memory[skssi]);
                if (r != VK_SUCCESS) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: AllocateMemory (sky staging slot %u) failed (%d)\n",
                               skssi, (int)r);
                    return false;
                }

                r = vk_fn.BindBufferMemory(vk_device, vk_sky_staging[skssi],
                                           vk_sky_staging_memory[skssi], 0);
                if (r != VK_SUCCESS) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: BindBufferMemory (sky staging slot %u) failed (%d)\n",
                               skssi, (int)r);
                    return false;
                }

                r = vk_fn.MapMemory(vk_device, vk_sky_staging_memory[skssi],
                                    0, VK_WHOLE_SIZE, 0, &vk_sky_staging_ptr[skssi]);
                if (r != VK_SUCCESS) {
                    if (log_cb)
                        log_cb(RETRO_LOG_ERROR,
                               "rhi-vk: MapMemory (sky staging slot %u) failed (%d)\n",
                               skssi, (int)r);
                    return false;
                }
            }
        }

        /* Re-arm pending if we already have data from a
         * pre-context-reset R_InitSky.  The first record_frame
         * after this point will perform the upload. */
        if (vk_sky_cache_populated)
            vk_sky_upload_pending = true;
    }

    /* Phase 5b-07a step 2 + 3: sky compute pipeline.
     *
     * Three host-visible host-coherent buffers (UBO, rows
     * SSBO, spans SSBO) permanently mapped for cheap per-
     * frame writes from the bucket-by-row pass in record_-
     * frame.  The shader (sky_cs) plus its DSL / pool / set
     * / pipeline_layout / pipeline mirror the alias-compute
     * shape from Phase 5b-06 but with 6 bindings (2 storage
     * images for the sky atlases on top of the output / UBO /
     * 2 SSBOs alias uses): u_output (vk_texture), u_sky_-
     * front, u_sky_back, SkyUbo, SkyRows, SkySpans.
     *
     * No push constants -- the bbox + all view-direction
     * parameters travel through the UBO since they change
     * each frame anyway.
     *
     * On any failure inside this block we return false; the
     * caller (rhi.c via backend_vk_context_reset) unwinds
     * via destroy_resources which is null-handle-safe. */
    {
        VkBufferCreateInfo            sb_ci;
        VkMemoryRequirements          sb_mem_req;
        VkMemoryAllocateInfo          sb_mem_ai;
        VkShaderModuleCreateInfo      sk_sm_ci;
        VkDescriptorSetLayoutBinding  sk_dsl_bindings[7];
        VkDescriptorSetLayoutCreateInfo sk_dsl_ci;
        VkDescriptorPoolSize          sk_pool_sizes[3];
        VkDescriptorPoolCreateInfo    sk_pool_ci;
        VkDescriptorSetAllocateInfo   sk_set_alloc;
        VkDescriptorImageInfo         sk_img_info[4];
        /* Phase 5b-08 step 2d: per-slot buf-infos + 7*N
         * writes are declared inside the per-slot update
         * scope further down. */
        VkPipelineLayoutCreateInfo    sk_pl_ci;
        VkComputePipelineCreateInfo   sk_cp_ci;
        VkPipelineShaderStageCreateInfo sk_cs_stage;
        uint32_t                      sb_mem_type;
        int                           bi;
        int                           pass;

        /* 1. Three host-visible storage / uniform buffers.
         *    Iteration order: UBO (binding 3), rows SSBO
         *    (binding 4), spans SSBO (binding 5).  All three
         *    are HOST_VISIBLE | HOST_COHERENT and stay mapped
         *    for the duration of vk_resources_ready, so each
         *    per-frame upload is just a memcpy into the
         *    mapped pointer (no Flush needed, no Begin /
         *    EndMappedRange calls). */
        /* Phase 5b-08 step 2d: outer per-slot loop, inner
         * 3-pass loop targets that slot's UBO / rows /
         * spans triple.  The pass selector chooses which
         * slot-array to write into via the active slot
         * index. */
        {
            unsigned sksi;
            for (sksi = 0; sksi < VK_FRAME_RING_DEPTH; sksi++) {
        for (pass = 0; pass < 3; pass++) {
            VkBuffer        *buf_p = (pass == 0) ? &vk_sky_ubo_buffer[sksi]
                                  : (pass == 1) ? &vk_sky_rows_buffer[sksi]
                                                : &vk_sky_spans_buffer[sksi];
            VkDeviceMemory  *mem_p = (pass == 0) ? &vk_sky_ubo_memory[sksi]
                                  : (pass == 1) ? &vk_sky_rows_memory[sksi]
                                                : &vk_sky_spans_memory[sksi];
            void           **ptr_p = (pass == 0) ? &vk_sky_ubo_ptr[sksi]
                                  : (pass == 1) ? &vk_sky_rows_ptr[sksi]
                                                : &vk_sky_spans_ptr[sksi];
            VkDeviceSize     sz    = (pass == 0) ? (VkDeviceSize)VK_SKY_UBO_BYTES
                                  : (pass == 1) ? (VkDeviceSize)VK_SKY_ROW_BYTES
                                                : (VkDeviceSize)VK_SKY_SPAN_BYTES;
            VkBufferUsageFlags usage = (pass == 0)
                                       ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
                                       : VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            const char      *label = (pass == 0) ? "sky-ubo"
                                  : (pass == 1) ? "sky-rows"
                                                : "sky-spans";

            memset(&sb_ci, 0, sizeof(sb_ci));
            sb_ci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            sb_ci.size        = sz;
            sb_ci.usage       = usage;
            sb_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            r = vk_fn.CreateBuffer(vk_device, &sb_ci, NULL, buf_p);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: CreateBuffer (%s slot %u) failed (%d)\n", label, sksi, (int)r);
                return false;
            }

            vk_fn.GetBufferMemoryRequirements(vk_device, *buf_p, &sb_mem_req);
            sb_mem_type = backend_vk_find_memory_type(sb_mem_req.memoryTypeBits,
                                                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                                     | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            if (sb_mem_type == 0xFFFFFFFFu) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: no HOST_VISIBLE|HOST_COHERENT memory type for %s slot %u\n",
                           label, sksi);
                return false;
            }

            memset(&sb_mem_ai, 0, sizeof(sb_mem_ai));
            sb_mem_ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            sb_mem_ai.allocationSize  = sb_mem_req.size;
            sb_mem_ai.memoryTypeIndex = sb_mem_type;

            r = vk_fn.AllocateMemory(vk_device, &sb_mem_ai, NULL, mem_p);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: AllocateMemory (%s slot %u) failed (%d)\n", label, sksi, (int)r);
                return false;
            }

            r = vk_fn.BindBufferMemory(vk_device, *buf_p, *mem_p, 0);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: BindBufferMemory (%s slot %u) failed (%d)\n", label, sksi, (int)r);
                return false;
            }

            r = vk_fn.MapMemory(vk_device, *mem_p, 0, VK_WHOLE_SIZE, 0, ptr_p);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: MapMemory (%s slot %u) failed (%d)\n", label, sksi, (int)r);
                return false;
            }
        }
            } /* end outer per-slot loop (Phase 5b-08 step 2d) */
        }

        /* 2. Shader module from precompiled SPIR-V. */
        memset(&sk_sm_ci, 0, sizeof(sk_sm_ci));
        sk_sm_ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        sk_sm_ci.codeSize = sizeof(spv_sky_cs);
        sk_sm_ci.pCode    = spv_sky_cs;

        r = vk_fn.CreateShaderModule(vk_device, &sk_sm_ci, NULL,
                                     &vk_sky_cs_module);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateShaderModule (sky.comp) failed (%d)\n", (int)r);
            return false;
        }

        /* 3. Descriptor set layout: 7 bindings, all COMPUTE. */
        memset(sk_dsl_bindings, 0, sizeof(sk_dsl_bindings));
        for (bi = 0; bi < 7; bi++) {
            sk_dsl_bindings[bi].binding         = (uint32_t)bi;
            sk_dsl_bindings[bi].descriptorCount = 1;
            sk_dsl_bindings[bi].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
        }
        sk_dsl_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; /* u_output */
        sk_dsl_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; /* u_sky_front */
        sk_dsl_bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; /* u_sky_back */
        sk_dsl_bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;/* SkyUbo */
        sk_dsl_bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;/* SkyRows */
        sk_dsl_bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;/* SkySpans */
        sk_dsl_bindings[6].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; /* u_zbuffer */

        memset(&sk_dsl_ci, 0, sizeof(sk_dsl_ci));
        sk_dsl_ci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        sk_dsl_ci.bindingCount = 7;
        sk_dsl_ci.pBindings    = sk_dsl_bindings;

        r = vk_fn.CreateDescriptorSetLayout(vk_device, &sk_dsl_ci, NULL,
                                            &vk_sky_dsl);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateDescriptorSetLayout (sky) failed (%d)\n", (int)r);
            return false;
        }

        /* 4. Descriptor pool sized for N sets.  Phase 5b-08
         *    step 2d: per-slot DS pattern.  Three pool
         *    sizes scaled by N: 4 storage images, 1 UBO, 2
         *    storage buffers per set. */
        memset(sk_pool_sizes, 0, sizeof(sk_pool_sizes));
        sk_pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        sk_pool_sizes[0].descriptorCount = 4u * VK_FRAME_RING_DEPTH;
        sk_pool_sizes[1].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        sk_pool_sizes[1].descriptorCount = 1u * VK_FRAME_RING_DEPTH;
        sk_pool_sizes[2].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        sk_pool_sizes[2].descriptorCount = 2u * VK_FRAME_RING_DEPTH;

        memset(&sk_pool_ci, 0, sizeof(sk_pool_ci));
        sk_pool_ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        sk_pool_ci.maxSets       = VK_FRAME_RING_DEPTH;
        sk_pool_ci.poolSizeCount = 3;
        sk_pool_ci.pPoolSizes    = sk_pool_sizes;

        r = vk_fn.CreateDescriptorPool(vk_device, &sk_pool_ci, NULL,
                                       &vk_sky_pool);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateDescriptorPool (sky) failed (%d)\n", (int)r);
            return false;
        }

        {
            VkDescriptorSetLayout sk_layouts[VK_FRAME_RING_DEPTH];
            unsigned              sksi;
            for (sksi = 0; sksi < VK_FRAME_RING_DEPTH; sksi++)
                sk_layouts[sksi] = vk_sky_dsl;

            memset(&sk_set_alloc, 0, sizeof(sk_set_alloc));
            sk_set_alloc.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            sk_set_alloc.descriptorPool     = vk_sky_pool;
            sk_set_alloc.descriptorSetCount = VK_FRAME_RING_DEPTH;
            sk_set_alloc.pSetLayouts        = sk_layouts;

            r = vk_fn.AllocateDescriptorSets(vk_device, &sk_set_alloc, vk_sky_set);
            if (r != VK_SUCCESS) {
                if (log_cb)
                    log_cb(RETRO_LOG_ERROR,
                           "rhi-vk: AllocateDescriptorSets (sky) failed (%d)\n", (int)r);
                return false;
            }
        }

        /* 5. Bind the 7 descriptors per set, N sets total
         *    = 7*N writes.  Shared image bindings (0/1/2/6)
         *    target the single-instance views; per-slot
         *    buffer bindings (3/4/5) target this slot's
         *    UBO + rows + spans. */
        memset(sk_img_info, 0, sizeof(sk_img_info));
        sk_img_info[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        sk_img_info[0].imageView   = vk_texture_view;
        sk_img_info[1].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        sk_img_info[1].imageView   = vk_sky_front_view;
        sk_img_info[2].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        sk_img_info[2].imageView   = vk_sky_back_view;
        sk_img_info[3].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        sk_img_info[3].imageView   = vk_zbuffer_view;

        {
            VkDescriptorBufferInfo sk_buf_infos[3 * VK_FRAME_RING_DEPTH];
            VkWriteDescriptorSet   sk_all_writes[7 * VK_FRAME_RING_DEPTH];
            unsigned               sksi;
            unsigned               wi = 0;

            memset(sk_buf_infos, 0, sizeof(sk_buf_infos));
            memset(sk_all_writes, 0, sizeof(sk_all_writes));

            for (sksi = 0; sksi < VK_FRAME_RING_DEPTH; sksi++) {
                sk_buf_infos[sksi * 3 + 0].buffer = vk_sky_ubo_buffer[sksi];
                sk_buf_infos[sksi * 3 + 0].offset = 0;
                sk_buf_infos[sksi * 3 + 0].range  = VK_WHOLE_SIZE;
                sk_buf_infos[sksi * 3 + 1].buffer = vk_sky_rows_buffer[sksi];
                sk_buf_infos[sksi * 3 + 1].offset = 0;
                sk_buf_infos[sksi * 3 + 1].range  = VK_WHOLE_SIZE;
                sk_buf_infos[sksi * 3 + 2].buffer = vk_sky_spans_buffer[sksi];
                sk_buf_infos[sksi * 3 + 2].offset = 0;
                sk_buf_infos[sksi * 3 + 2].range  = VK_WHOLE_SIZE;

                /* Binding 0: STORAGE_IMAGE (vk_texture_view). */
                sk_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                sk_all_writes[wi].dstSet          = vk_sky_set[sksi];
                sk_all_writes[wi].dstBinding      = 0;
                sk_all_writes[wi].descriptorCount = 1;
                sk_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                sk_all_writes[wi].pImageInfo      = &sk_img_info[0];
                wi++;
                /* Binding 1: STORAGE_IMAGE (sky_front). */
                sk_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                sk_all_writes[wi].dstSet          = vk_sky_set[sksi];
                sk_all_writes[wi].dstBinding      = 1;
                sk_all_writes[wi].descriptorCount = 1;
                sk_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                sk_all_writes[wi].pImageInfo      = &sk_img_info[1];
                wi++;
                /* Binding 2: STORAGE_IMAGE (sky_back). */
                sk_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                sk_all_writes[wi].dstSet          = vk_sky_set[sksi];
                sk_all_writes[wi].dstBinding      = 2;
                sk_all_writes[wi].descriptorCount = 1;
                sk_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                sk_all_writes[wi].pImageInfo      = &sk_img_info[2];
                wi++;
                /* Binding 3: UNIFORM_BUFFER (sky_ubo). */
                sk_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                sk_all_writes[wi].dstSet          = vk_sky_set[sksi];
                sk_all_writes[wi].dstBinding      = 3;
                sk_all_writes[wi].descriptorCount = 1;
                sk_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                sk_all_writes[wi].pBufferInfo     = &sk_buf_infos[sksi * 3 + 0];
                wi++;
                /* Binding 4: STORAGE_BUFFER (sky_rows). */
                sk_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                sk_all_writes[wi].dstSet          = vk_sky_set[sksi];
                sk_all_writes[wi].dstBinding      = 4;
                sk_all_writes[wi].descriptorCount = 1;
                sk_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                sk_all_writes[wi].pBufferInfo     = &sk_buf_infos[sksi * 3 + 1];
                wi++;
                /* Binding 5: STORAGE_BUFFER (sky_spans). */
                sk_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                sk_all_writes[wi].dstSet          = vk_sky_set[sksi];
                sk_all_writes[wi].dstBinding      = 5;
                sk_all_writes[wi].descriptorCount = 1;
                sk_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                sk_all_writes[wi].pBufferInfo     = &sk_buf_infos[sksi * 3 + 2];
                wi++;
                /* Binding 6: STORAGE_IMAGE (vk_zbuffer_view). */
                sk_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                sk_all_writes[wi].dstSet          = vk_sky_set[sksi];
                sk_all_writes[wi].dstBinding      = 6;
                sk_all_writes[wi].descriptorCount = 1;
                sk_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                sk_all_writes[wi].pImageInfo      = &sk_img_info[3];
                wi++;
            }

            vk_fn.UpdateDescriptorSets(vk_device, wi, sk_all_writes, 0, NULL);
        }

        /* 6. Pipeline layout (no push constants -- everything
         *    in the UBO) + compute pipeline. */
        memset(&sk_pl_ci, 0, sizeof(sk_pl_ci));
        sk_pl_ci.sType          = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        sk_pl_ci.setLayoutCount = 1;
        sk_pl_ci.pSetLayouts    = &vk_sky_dsl;

        r = vk_fn.CreatePipelineLayout(vk_device, &sk_pl_ci, NULL,
                                       &vk_sky_pipeline_layout);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreatePipelineLayout (sky) failed (%d)\n", (int)r);
            return false;
        }

        memset(&sk_cs_stage, 0, sizeof(sk_cs_stage));
        sk_cs_stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        sk_cs_stage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
        sk_cs_stage.module = vk_sky_cs_module;
        sk_cs_stage.pName  = "main";

        memset(&sk_cp_ci, 0, sizeof(sk_cp_ci));
        sk_cp_ci.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        sk_cp_ci.stage  = sk_cs_stage;
        sk_cp_ci.layout = vk_sky_pipeline_layout;

        r = vk_fn.CreateComputePipelines(vk_device, VK_NULL_HANDLE,
                                         1, &sk_cp_ci, NULL,
                                         &vk_sky_pipeline);
        if (r != VK_SUCCESS) {
            if (log_cb)
                log_cb(RETRO_LOG_ERROR,
                       "rhi-vk: CreateComputePipelines (sky) failed (%d)\n", (int)r);
            return false;
        }
    }

    /* Phase 5b-07b: GPU compute turb-surface raster resources.
     * Atlas image (per-frame texture dedup), three host-mapped
     * buffers (rows + spans + atlas staging), shader, DSL +
     * pool + set, push-constant-driven pipeline layout +
     * pipeline.  All conditional on this block running once at
     * create_resources -- record_frame just uses what's here. */
    {
        VkImageCreateInfo            tu_ci;
        VkMemoryRequirements         tu_mem_req;
        VkMemoryAllocateInfo         tu_mem_ai;
        VkImageViewCreateInfo        tu_view_ci;
        VkBufferCreateInfo           tu_buf_ci[4];   /* staging, rows, spans, plus 1 spare */
        VkMemoryRequirements         tu_buf_mem_req;
        VkMemoryAllocateInfo         tu_buf_mem_ai;
        VkShaderModuleCreateInfo     tu_sm_ci;
        VkDescriptorSetLayoutBinding tu_dsl_bindings[5];
        VkDescriptorSetLayoutCreateInfo tu_dsl_ci;
        VkDescriptorPoolSize         tu_pool_sizes[2];
        VkDescriptorPoolCreateInfo   tu_pool_ci;
        VkDescriptorSetAllocateInfo  tu_set_alloc;
        VkDescriptorImageInfo        tu_img_info[3];
        /* Phase 5b-08 step 2d: per-slot buf-infos + 5*N
         * writes are declared inside the per-slot update
         * scope further down. */
        VkPipelineLayoutCreateInfo   tu_pl_ci;
        VkPushConstantRange          tu_pcr;
        VkPipelineShaderStageCreateInfo tu_cs_stage;
        VkComputePipelineCreateInfo  tu_cp_ci;
        uint32_t                     tu_mem_type;
        int                          bi;

        /* Atlas storage image: 64 wide x 64*VK_TURB_TEX_SLOTS
         * tall (R8_UINT, palette-index storage), GENERAL
         * layout after the first record_frame transition. */
        memset(&tu_ci, 0, sizeof(tu_ci));
        tu_ci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        tu_ci.imageType     = VK_IMAGE_TYPE_2D;
        tu_ci.format        = VK_FORMAT_R8_UINT;
        tu_ci.extent.width  = VK_TURB_TEX_SIZE;
        tu_ci.extent.height = VK_TURB_TEX_SIZE * VK_TURB_TEX_SLOTS;
        tu_ci.extent.depth  = 1;
        tu_ci.mipLevels     = 1;
        tu_ci.arrayLayers   = 1;
        tu_ci.samples       = VK_SAMPLE_COUNT_1_BIT;
        tu_ci.tiling        = VK_IMAGE_TILING_OPTIMAL;
        tu_ci.usage         = VK_IMAGE_USAGE_STORAGE_BIT
                            | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        tu_ci.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        tu_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        r = vk_fn.CreateImage(vk_device, &tu_ci, NULL, &vk_turb_atlas_image);
        if (r != VK_SUCCESS) {
            if (log_cb) log_cb(RETRO_LOG_ERROR,
                "rhi-vk: CreateImage (turb atlas) failed (%d)\n", (int)r);
            return false;
        }

        vk_fn.GetImageMemoryRequirements(vk_device, vk_turb_atlas_image,
                                         &tu_mem_req);
        tu_mem_type = backend_vk_find_memory_type(tu_mem_req.memoryTypeBits,
                                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (tu_mem_type == UINT32_MAX) {
            if (log_cb) log_cb(RETRO_LOG_ERROR,
                "rhi-vk: no DEVICE_LOCAL memory type for turb atlas\n");
            return false;
        }
        memset(&tu_mem_ai, 0, sizeof(tu_mem_ai));
        tu_mem_ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        tu_mem_ai.allocationSize  = tu_mem_req.size;
        tu_mem_ai.memoryTypeIndex = tu_mem_type;
        r = vk_fn.AllocateMemory(vk_device, &tu_mem_ai, NULL,
                                 &vk_turb_atlas_memory);
        if (r != VK_SUCCESS) {
            if (log_cb) log_cb(RETRO_LOG_ERROR,
                "rhi-vk: AllocateMemory (turb atlas) failed (%d)\n", (int)r);
            return false;
        }
        vk_fn.BindImageMemory(vk_device, vk_turb_atlas_image,
                              vk_turb_atlas_memory, 0);

        memset(&tu_view_ci, 0, sizeof(tu_view_ci));
        tu_view_ci.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        tu_view_ci.image    = vk_turb_atlas_image;
        tu_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        tu_view_ci.format   = VK_FORMAT_R8_UINT;
        tu_view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        tu_view_ci.subresourceRange.levelCount = 1;
        tu_view_ci.subresourceRange.layerCount = 1;
        r = vk_fn.CreateImageView(vk_device, &tu_view_ci, NULL,
                                  &vk_turb_atlas_view);
        if (r != VK_SUCCESS) {
            if (log_cb) log_cb(RETRO_LOG_ERROR,
                "rhi-vk: CreateImageView (turb atlas) failed (%d)\n", (int)r);
            return false;
        }

        /* Three permanently-mapped HOST_VISIBLE | HOST_COHERENT
         * buffers: atlas staging (128 KiB), rows SSBO (128 KiB),
         * spans SSBO (64 KiB).  Created in a loop to share the
         * memory-type lookup and the bind/map sequence. */
        memset(tu_buf_ci, 0, sizeof(tu_buf_ci));
        tu_buf_ci[0].sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        tu_buf_ci[0].size  = VK_TURB_ATLAS_BYTES;
        tu_buf_ci[0].usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        tu_buf_ci[0].sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        tu_buf_ci[1].sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        tu_buf_ci[1].size  = VK_TURB_ROW_BYTES;
        tu_buf_ci[1].usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        tu_buf_ci[1].sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        tu_buf_ci[2].sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        tu_buf_ci[2].size  = VK_TURB_SPAN_BYTES;
        tu_buf_ci[2].usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        tu_buf_ci[2].sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        {
            unsigned tusi;
            for (tusi = 0; tusi < VK_FRAME_RING_DEPTH; tusi++) {
                VkBuffer       *buf_handles[3] = { &vk_turb_atlas_staging[tusi],
                                                    &vk_turb_rows_buffer[tusi],
                                                    &vk_turb_spans_buffer[tusi] };
                VkDeviceMemory *mem_handles[3] = { &vk_turb_atlas_staging_memory[tusi],
                                                    &vk_turb_rows_memory[tusi],
                                                    &vk_turb_spans_memory[tusi] };
                void          **ptr_handles[3] = { &vk_turb_atlas_staging_ptr[tusi],
                                                    &vk_turb_rows_ptr[tusi],
                                                    &vk_turb_spans_ptr[tusi] };
                int pass;
                for (pass = 0; pass < 3; pass++) {
                    r = vk_fn.CreateBuffer(vk_device, &tu_buf_ci[pass], NULL,
                                           buf_handles[pass]);
                    if (r != VK_SUCCESS) {
                        if (log_cb) log_cb(RETRO_LOG_ERROR,
                            "rhi-vk: CreateBuffer (turb pass=%d slot %u) failed (%d)\n",
                            pass, tusi, (int)r);
                        return false;
                    }
                    vk_fn.GetBufferMemoryRequirements(vk_device,
                                                      *buf_handles[pass],
                                                      &tu_buf_mem_req);
                    tu_mem_type = backend_vk_find_memory_type(
                        tu_buf_mem_req.memoryTypeBits,
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                        | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
                    if (tu_mem_type == UINT32_MAX) {
                        if (log_cb) log_cb(RETRO_LOG_ERROR,
                            "rhi-vk: no HOST_VISIBLE memory for turb pass=%d slot %u\n",
                            pass, tusi);
                        return false;
                    }
                    memset(&tu_buf_mem_ai, 0, sizeof(tu_buf_mem_ai));
                    tu_buf_mem_ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
                    tu_buf_mem_ai.allocationSize  = tu_buf_mem_req.size;
                    tu_buf_mem_ai.memoryTypeIndex = tu_mem_type;
                    r = vk_fn.AllocateMemory(vk_device, &tu_buf_mem_ai, NULL,
                                             mem_handles[pass]);
                    if (r != VK_SUCCESS) {
                        if (log_cb) log_cb(RETRO_LOG_ERROR,
                            "rhi-vk: AllocateMemory (turb pass=%d slot %u) failed (%d)\n",
                            pass, tusi, (int)r);
                        return false;
                    }
                    vk_fn.BindBufferMemory(vk_device, *buf_handles[pass],
                                           *mem_handles[pass], 0);
                    vk_fn.MapMemory(vk_device, *mem_handles[pass], 0,
                                    tu_buf_ci[pass].size, 0, ptr_handles[pass]);
                }
            }
        }

        /* Shader module. */
        memset(&tu_sm_ci, 0, sizeof(tu_sm_ci));
        tu_sm_ci.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        tu_sm_ci.codeSize = sizeof(spv_turb_cs);
        tu_sm_ci.pCode    = spv_turb_cs;
        r = vk_fn.CreateShaderModule(vk_device, &tu_sm_ci, NULL,
                                     &vk_turb_cs_module);
        if (r != VK_SUCCESS) {
            if (log_cb) log_cb(RETRO_LOG_ERROR,
                "rhi-vk: CreateShaderModule (turb) failed (%d)\n", (int)r);
            return false;
        }

        /* DSL: 5 bindings (u_output, u_turb_textures, TurbRows,
         * TurbSpans, u_zbuffer), all COMPUTE. */
        memset(tu_dsl_bindings, 0, sizeof(tu_dsl_bindings));
        for (bi = 0; bi < 5; bi++) {
            tu_dsl_bindings[bi].binding         = (uint32_t)bi;
            tu_dsl_bindings[bi].descriptorCount = 1;
            tu_dsl_bindings[bi].stageFlags      = VK_SHADER_STAGE_COMPUTE_BIT;
        }
        tu_dsl_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        tu_dsl_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        tu_dsl_bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        tu_dsl_bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        tu_dsl_bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        memset(&tu_dsl_ci, 0, sizeof(tu_dsl_ci));
        tu_dsl_ci.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        tu_dsl_ci.bindingCount = 5;
        tu_dsl_ci.pBindings    = tu_dsl_bindings;
        r = vk_fn.CreateDescriptorSetLayout(vk_device, &tu_dsl_ci, NULL,
                                            &vk_turb_dsl);
        if (r != VK_SUCCESS) {
            if (log_cb) log_cb(RETRO_LOG_ERROR,
                "rhi-vk: CreateDescriptorSetLayout (turb) failed (%d)\n", (int)r);
            return false;
        }

        /* Pool: 3 storage images + 2 storage buffers per
         * set; N sets total. */
        memset(tu_pool_sizes, 0, sizeof(tu_pool_sizes));
        tu_pool_sizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        tu_pool_sizes[0].descriptorCount = 3u * VK_FRAME_RING_DEPTH;
        tu_pool_sizes[1].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        tu_pool_sizes[1].descriptorCount = 2u * VK_FRAME_RING_DEPTH;
        memset(&tu_pool_ci, 0, sizeof(tu_pool_ci));
        tu_pool_ci.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        tu_pool_ci.maxSets       = VK_FRAME_RING_DEPTH;
        tu_pool_ci.poolSizeCount = 2;
        tu_pool_ci.pPoolSizes    = tu_pool_sizes;
        r = vk_fn.CreateDescriptorPool(vk_device, &tu_pool_ci, NULL,
                                       &vk_turb_pool);
        if (r != VK_SUCCESS) {
            if (log_cb) log_cb(RETRO_LOG_ERROR,
                "rhi-vk: CreateDescriptorPool (turb) failed (%d)\n", (int)r);
            return false;
        }

        {
            VkDescriptorSetLayout tu_layouts[VK_FRAME_RING_DEPTH];
            unsigned              tusi;
            for (tusi = 0; tusi < VK_FRAME_RING_DEPTH; tusi++)
                tu_layouts[tusi] = vk_turb_dsl;

            memset(&tu_set_alloc, 0, sizeof(tu_set_alloc));
            tu_set_alloc.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            tu_set_alloc.descriptorPool     = vk_turb_pool;
            tu_set_alloc.descriptorSetCount = VK_FRAME_RING_DEPTH;
            tu_set_alloc.pSetLayouts        = tu_layouts;
            r = vk_fn.AllocateDescriptorSets(vk_device, &tu_set_alloc, vk_turb_set);
            if (r != VK_SUCCESS) {
                if (log_cb) log_cb(RETRO_LOG_ERROR,
                    "rhi-vk: AllocateDescriptorSets (turb) failed (%d)\n", (int)r);
                return false;
            }
        }

        memset(tu_img_info, 0, sizeof(tu_img_info));
        tu_img_info[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        tu_img_info[0].imageView   = vk_texture_view;
        tu_img_info[1].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        tu_img_info[1].imageView   = vk_turb_atlas_view;
        tu_img_info[2].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        tu_img_info[2].imageView   = vk_zbuffer_view;

        {
            VkDescriptorBufferInfo tu_buf_infos[2 * VK_FRAME_RING_DEPTH];
            VkWriteDescriptorSet   tu_all_writes[5 * VK_FRAME_RING_DEPTH];
            unsigned               tusi;
            unsigned               wi = 0;

            memset(tu_buf_infos, 0, sizeof(tu_buf_infos));
            memset(tu_all_writes, 0, sizeof(tu_all_writes));

            for (tusi = 0; tusi < VK_FRAME_RING_DEPTH; tusi++) {
                tu_buf_infos[tusi * 2 + 0].buffer = vk_turb_rows_buffer[tusi];
                tu_buf_infos[tusi * 2 + 0].range  = VK_TURB_ROW_BYTES;
                tu_buf_infos[tusi * 2 + 1].buffer = vk_turb_spans_buffer[tusi];
                tu_buf_infos[tusi * 2 + 1].range  = VK_TURB_SPAN_BYTES;

                /* Binding 0: STORAGE_IMAGE (vk_texture_view). */
                tu_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                tu_all_writes[wi].dstSet          = vk_turb_set[tusi];
                tu_all_writes[wi].dstBinding      = 0;
                tu_all_writes[wi].descriptorCount = 1;
                tu_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                tu_all_writes[wi].pImageInfo      = &tu_img_info[0];
                wi++;
                /* Binding 1: STORAGE_IMAGE (turb_atlas). */
                tu_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                tu_all_writes[wi].dstSet          = vk_turb_set[tusi];
                tu_all_writes[wi].dstBinding      = 1;
                tu_all_writes[wi].descriptorCount = 1;
                tu_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                tu_all_writes[wi].pImageInfo      = &tu_img_info[1];
                wi++;
                /* Binding 2: STORAGE_BUFFER (turb_rows). */
                tu_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                tu_all_writes[wi].dstSet          = vk_turb_set[tusi];
                tu_all_writes[wi].dstBinding      = 2;
                tu_all_writes[wi].descriptorCount = 1;
                tu_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                tu_all_writes[wi].pBufferInfo     = &tu_buf_infos[tusi * 2 + 0];
                wi++;
                /* Binding 3: STORAGE_BUFFER (turb_spans). */
                tu_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                tu_all_writes[wi].dstSet          = vk_turb_set[tusi];
                tu_all_writes[wi].dstBinding      = 3;
                tu_all_writes[wi].descriptorCount = 1;
                tu_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                tu_all_writes[wi].pBufferInfo     = &tu_buf_infos[tusi * 2 + 1];
                wi++;
                /* Binding 4: STORAGE_IMAGE (zbuffer). */
                tu_all_writes[wi].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                tu_all_writes[wi].dstSet          = vk_turb_set[tusi];
                tu_all_writes[wi].dstBinding      = 4;
                tu_all_writes[wi].descriptorCount = 1;
                tu_all_writes[wi].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                tu_all_writes[wi].pImageInfo      = &tu_img_info[2];
                wi++;
            }

            vk_fn.UpdateDescriptorSets(vk_device, wi, tu_all_writes, 0, NULL);
        }

        /* Pipeline layout: 1 set + 84-byte push-constant block
         * (compute stage only).  See turb.comp for the field
         * layout. */
        memset(&tu_pcr, 0, sizeof(tu_pcr));
        tu_pcr.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        tu_pcr.offset     = 0;
        tu_pcr.size       = 112;  /* 6 x vec4 + 1 x uvec4 */
        memset(&tu_pl_ci, 0, sizeof(tu_pl_ci));
        tu_pl_ci.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        tu_pl_ci.setLayoutCount         = 1;
        tu_pl_ci.pSetLayouts            = &vk_turb_dsl;
        tu_pl_ci.pushConstantRangeCount = 1;
        tu_pl_ci.pPushConstantRanges    = &tu_pcr;
        r = vk_fn.CreatePipelineLayout(vk_device, &tu_pl_ci, NULL,
                                       &vk_turb_pipeline_layout);
        if (r != VK_SUCCESS) {
            if (log_cb) log_cb(RETRO_LOG_ERROR,
                "rhi-vk: CreatePipelineLayout (turb) failed (%d)\n", (int)r);
            return false;
        }

        memset(&tu_cs_stage, 0, sizeof(tu_cs_stage));
        tu_cs_stage.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        tu_cs_stage.stage  = VK_SHADER_STAGE_COMPUTE_BIT;
        tu_cs_stage.module = vk_turb_cs_module;
        tu_cs_stage.pName  = "main";
        memset(&tu_cp_ci, 0, sizeof(tu_cp_ci));
        tu_cp_ci.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        tu_cp_ci.stage  = tu_cs_stage;
        tu_cp_ci.layout = vk_turb_pipeline_layout;
        r = vk_fn.CreateComputePipelines(vk_device, VK_NULL_HANDLE, 1,
                                         &tu_cp_ci, NULL, &vk_turb_pipeline);
        if (r != VK_SUCCESS) {
            if (log_cb) log_cb(RETRO_LOG_ERROR,
                "rhi-vk: CreateComputePipelines (turb) failed (%d)\n", (int)r);
            return false;
        }
    }

    /* --------------------------------------------------------
     * Phase 5b-07 step 2: brush surface atlas resources.
     *
     * One VkImage of R8_UINT VK_BRUSH_ATLAS_W x VK_BRUSH_ATLAS_H
     * = 16 MiB.  Used as a storage image by the brush compute
     * dispatch (imageLoad via a uimage2D view, see brush.comp in
     * step 3) and as a transfer destination during the per-frame
     * staging upload.  Initial layout UNDEFINED; record_frame
     * transitions to TRANSFER_DST during the upload step then
     * back to GENERAL for the compute dispatch.
     *
     * One staging VkBuffer of VK_BRUSH_ATLAS_BYTES = 16 MiB,
     * host-visible coherent, permanently mapped.  The dispatch
     * site memcpys cacheblock contents into here at the rect
     * offset surf_atlas_get returned; record_frame batches all
     * dirty rects into one CmdCopyBufferToImage.  Note that
     * Phase 5b-07 step 2 lands the resources only -- without
     * the brush dispatch wired (step 3) the staging buffer is
     * never written and the CopyBufferToImage is never emitted.
     *
     * The surf_atlas_t instance carries the strip free-lists,
     * the (key -> rect) cache, and LRU-eviction state.  Strip
     * ladder is the recommended Quake distribution from
     * surf_atlas.h: 32 x 16h, 16 x 32h, 16 x 64h, 8 x 128h,
     * 2 x 256h, 1 x 512h -- summing to VK_BRUSH_ATLAS_H. */
    {
        VkImageCreateInfo            ba_ci;
        VkMemoryRequirements         ba_mem_req;
        VkMemoryAllocateInfo         ba_mem_ai;
        VkImageViewCreateInfo        ba_view_ci;
        VkBufferCreateInfo           ba_buf_ci;
        VkMemoryRequirements         ba_buf_mem_req;
        VkMemoryAllocateInfo         ba_buf_mem_ai;
        uint32_t                     ba_mem_type;
        uint32_t                     ba_buf_mem_type;
        surf_atlas_config_t          ba_cfg;
        static const surf_atlas_strip_desc_t k_brush_strip_desc[] = {
            {  16, 32 },  /* 512  px */
            {  32, 16 },  /* 512  px */
            {  64, 16 },  /* 1024 px */
            { 128,  8 },  /* 1024 px */
            { 256,  2 },  /* 512  px */
            { 512,  1 }   /* 512  px */
            /* total              4096 px = VK_BRUSH_ATLAS_H */
        };

        /* Atlas storage image. */
        memset(&ba_ci, 0, sizeof(ba_ci));
        ba_ci.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        ba_ci.imageType     = VK_IMAGE_TYPE_2D;
        ba_ci.format        = VK_FORMAT_R8_UINT;
        ba_ci.extent.width  = VK_BRUSH_ATLAS_W;
        ba_ci.extent.height = VK_BRUSH_ATLAS_H;
        ba_ci.extent.depth  = 1;
        ba_ci.mipLevels     = 1;
        ba_ci.arrayLayers   = 1;
        ba_ci.samples       = VK_SAMPLE_COUNT_1_BIT;
        ba_ci.tiling        = VK_IMAGE_TILING_OPTIMAL;
        ba_ci.usage         = VK_IMAGE_USAGE_STORAGE_BIT
                            | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        ba_ci.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
        ba_ci.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        r = vk_fn.CreateImage(vk_device, &ba_ci, NULL,
                              &vk_brush_atlas_image);
        if (r != VK_SUCCESS) {
            if (log_cb) log_cb(RETRO_LOG_ERROR,
                "rhi-vk: CreateImage (brush atlas) failed (%d)\n", (int)r);
            return false;
        }

        vk_fn.GetImageMemoryRequirements(vk_device, vk_brush_atlas_image,
                                         &ba_mem_req);
        ba_mem_type = backend_vk_find_memory_type(ba_mem_req.memoryTypeBits,
                                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (ba_mem_type == UINT32_MAX) {
            if (log_cb) log_cb(RETRO_LOG_ERROR,
                "rhi-vk: no DEVICE_LOCAL memory type for brush atlas\n");
            return false;
        }
        memset(&ba_mem_ai, 0, sizeof(ba_mem_ai));
        ba_mem_ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ba_mem_ai.allocationSize  = ba_mem_req.size;
        ba_mem_ai.memoryTypeIndex = ba_mem_type;
        r = vk_fn.AllocateMemory(vk_device, &ba_mem_ai, NULL,
                                 &vk_brush_atlas_memory);
        if (r != VK_SUCCESS) {
            if (log_cb) log_cb(RETRO_LOG_ERROR,
                "rhi-vk: AllocateMemory (brush atlas) failed (%d)\n", (int)r);
            return false;
        }
        vk_fn.BindImageMemory(vk_device, vk_brush_atlas_image,
                              vk_brush_atlas_memory, 0);

        memset(&ba_view_ci, 0, sizeof(ba_view_ci));
        ba_view_ci.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ba_view_ci.image    = vk_brush_atlas_image;
        ba_view_ci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ba_view_ci.format   = VK_FORMAT_R8_UINT;
        ba_view_ci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ba_view_ci.subresourceRange.levelCount = 1;
        ba_view_ci.subresourceRange.layerCount = 1;
        r = vk_fn.CreateImageView(vk_device, &ba_view_ci, NULL,
                                  &vk_brush_atlas_view);
        if (r != VK_SUCCESS) {
            if (log_cb) log_cb(RETRO_LOG_ERROR,
                "rhi-vk: CreateImageView (brush atlas) failed (%d)\n", (int)r);
            return false;
        }

        /* Staging buffer.  HOST_VISIBLE | HOST_COHERENT,
         * permanently mapped; the dispatch site writes
         * directly into the mapped pointer at the rect's
         * byte offset.  TRANSFER_SRC for the
         * CmdCopyBufferToImage that ships its contents to
         * vk_brush_atlas_image. */
        memset(&ba_buf_ci, 0, sizeof(ba_buf_ci));
        ba_buf_ci.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        ba_buf_ci.size        = VK_BRUSH_ATLAS_BYTES;
        ba_buf_ci.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        ba_buf_ci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        r = vk_fn.CreateBuffer(vk_device, &ba_buf_ci, NULL,
                               &vk_brush_atlas_staging);
        if (r != VK_SUCCESS) {
            if (log_cb) log_cb(RETRO_LOG_ERROR,
                "rhi-vk: CreateBuffer (brush atlas staging) failed (%d)\n",
                (int)r);
            return false;
        }
        vk_fn.GetBufferMemoryRequirements(vk_device,
                                          vk_brush_atlas_staging,
                                          &ba_buf_mem_req);
        ba_buf_mem_type = backend_vk_find_memory_type(
            ba_buf_mem_req.memoryTypeBits,
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
            | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        if (ba_buf_mem_type == UINT32_MAX) {
            if (log_cb) log_cb(RETRO_LOG_ERROR,
                "rhi-vk: no HOST_VISIBLE|COHERENT memory type "
                "for brush atlas staging\n");
            return false;
        }
        memset(&ba_buf_mem_ai, 0, sizeof(ba_buf_mem_ai));
        ba_buf_mem_ai.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        ba_buf_mem_ai.allocationSize  = ba_buf_mem_req.size;
        ba_buf_mem_ai.memoryTypeIndex = ba_buf_mem_type;
        r = vk_fn.AllocateMemory(vk_device, &ba_buf_mem_ai, NULL,
                                 &vk_brush_atlas_staging_memory);
        if (r != VK_SUCCESS) {
            if (log_cb) log_cb(RETRO_LOG_ERROR,
                "rhi-vk: AllocateMemory (brush atlas staging) failed (%d)\n",
                (int)r);
            return false;
        }
        vk_fn.BindBufferMemory(vk_device, vk_brush_atlas_staging,
                               vk_brush_atlas_staging_memory, 0);
        r = vk_fn.MapMemory(vk_device, vk_brush_atlas_staging_memory,
                            0, VK_WHOLE_SIZE, 0,
                            &vk_brush_atlas_staging_ptr);
        if (r != VK_SUCCESS) {
            if (log_cb) log_cb(RETRO_LOG_ERROR,
                "rhi-vk: MapMemory (brush atlas staging) failed (%d)\n",
                (int)r);
            return false;
        }

        /* RHI-agnostic allocator.  Lives at module scope; freed
         * in backend_vk_destroy_resources. */
        memset(&ba_cfg, 0, sizeof(ba_cfg));
        ba_cfg.width            = VK_BRUSH_ATLAS_W;
        ba_cfg.height           = VK_BRUSH_ATLAS_H;
        ba_cfg.max_entries      = VK_BRUSH_MAX_ENTRIES;
        ba_cfg.strip_desc_count = (uint16_t)(sizeof(k_brush_strip_desc)
                                  / sizeof(k_brush_strip_desc[0]));
        ba_cfg.strip_desc       = k_brush_strip_desc;
        vk_brush_atlas = surf_atlas_create(&ba_cfg);
        if (!vk_brush_atlas) {
            if (log_cb) log_cb(RETRO_LOG_ERROR,
                "rhi-vk: surf_atlas_create failed (brush atlas)\n");
            return false;
        }
    }

    vk_resources_ready = true;
    if (log_cb)
        log_cb(RETRO_LOG_INFO,
               "rhi-vk: render target %ux%u + R8 index + 256x1 palette + "
               "staging %llu bytes ready; compute palette LUT + Draw_Pic intercept active\n",
               width, height,
               (unsigned long long)vk_staging_size);
    return true;
}

/*
 * Tear down everything backend_vk_create_resources built.  Safe
 * to call when resources are partially or fully constructed --
 * each step is gated on its corresponding handle being non-null.
 * Always called from context_destroy (and from
 * create_resources's own rollback if a step failed).
 */
static void
backend_vk_destroy_resources(void)
{
    /* Drain the queue first so nothing is still referencing
     * the objects we're about to destroy.  DeviceWaitIdle is
     * a stronger barrier than QueueWaitIdle (covers every
     * queue), which is what we want at teardown. */
    if (vk_fn.DeviceWaitIdle && vk_device != VK_NULL_HANDLE)
        vk_fn.DeviceWaitIdle(vk_device);

    /* Phase 5b-06 alias pipeline / resources teardown.
     * Reverse-create order.  Alias was created after
     * sprite (which is below); destruction is the
     * mirror order. */
    if (vk_alias_pipeline != VK_NULL_HANDLE) {
        if (vk_fn.DestroyPipeline)
            vk_fn.DestroyPipeline(vk_device, vk_alias_pipeline, NULL);
        vk_alias_pipeline = VK_NULL_HANDLE;
    }
    if (vk_alias_pipeline_layout != VK_NULL_HANDLE) {
        if (vk_fn.DestroyPipelineLayout)
            vk_fn.DestroyPipelineLayout(vk_device,
                                        vk_alias_pipeline_layout, NULL);
        vk_alias_pipeline_layout = VK_NULL_HANDLE;
    }
    if (vk_alias_pool != VK_NULL_HANDLE) {
        if (vk_fn.DestroyDescriptorPool)
            vk_fn.DestroyDescriptorPool(vk_device, vk_alias_pool, NULL);
        vk_alias_pool = VK_NULL_HANDLE;
        {
            unsigned asi;
            for (asi = 0; asi < VK_FRAME_RING_DEPTH; asi++)
                vk_alias_set[asi] = VK_NULL_HANDLE;
        }
    }
    if (vk_alias_dsl != VK_NULL_HANDLE) {
        if (vk_fn.DestroyDescriptorSetLayout)
            vk_fn.DestroyDescriptorSetLayout(vk_device, vk_alias_dsl, NULL);
        vk_alias_dsl = VK_NULL_HANDLE;
    }
    if (vk_alias_cs_module != VK_NULL_HANDLE) {
        if (vk_fn.DestroyShaderModule)
            vk_fn.DestroyShaderModule(vk_device, vk_alias_cs_module, NULL);
        vk_alias_cs_module = VK_NULL_HANDLE;
    }
    /* Phase 5b-08 step 2c: ring-buffered alias SSBO
     * teardown.  Four kinds × N slots; loop inner pattern
     * matches the create loop. */
    {
        unsigned asi;
        for (asi = 0; asi < VK_FRAME_RING_DEPTH; asi++) {
            if (vk_alias_colormap_buffer[asi] != VK_NULL_HANDLE) {
                if (vk_fn.DestroyBuffer)
                    vk_fn.DestroyBuffer(vk_device, vk_alias_colormap_buffer[asi], NULL);
                vk_alias_colormap_buffer[asi] = VK_NULL_HANDLE;
            }
            if (vk_alias_colormap_memory[asi] != VK_NULL_HANDLE) {
                if (vk_fn.FreeMemory)
                    vk_fn.FreeMemory(vk_device, vk_alias_colormap_memory[asi], NULL);
                vk_alias_colormap_memory[asi] = VK_NULL_HANDLE;
            }
            vk_alias_colormap_ptr[asi] = NULL;

            if (vk_alias_skin_buffer[asi] != VK_NULL_HANDLE) {
                if (vk_fn.DestroyBuffer)
                    vk_fn.DestroyBuffer(vk_device, vk_alias_skin_buffer[asi], NULL);
                vk_alias_skin_buffer[asi] = VK_NULL_HANDLE;
            }
            if (vk_alias_skin_memory[asi] != VK_NULL_HANDLE) {
                if (vk_fn.FreeMemory)
                    vk_fn.FreeMemory(vk_device, vk_alias_skin_memory[asi], NULL);
                vk_alias_skin_memory[asi] = VK_NULL_HANDLE;
            }
            vk_alias_skin_ptr[asi] = NULL;

            if (vk_alias_triangle_buffer[asi] != VK_NULL_HANDLE) {
                if (vk_fn.DestroyBuffer)
                    vk_fn.DestroyBuffer(vk_device, vk_alias_triangle_buffer[asi], NULL);
                vk_alias_triangle_buffer[asi] = VK_NULL_HANDLE;
            }
            if (vk_alias_triangle_memory[asi] != VK_NULL_HANDLE) {
                if (vk_fn.FreeMemory)
                    vk_fn.FreeMemory(vk_device, vk_alias_triangle_memory[asi], NULL);
                vk_alias_triangle_memory[asi] = VK_NULL_HANDLE;
            }
            vk_alias_triangle_ptr[asi] = NULL;

            if (vk_alias_vertex_buffer[asi] != VK_NULL_HANDLE) {
                if (vk_fn.DestroyBuffer)
                    vk_fn.DestroyBuffer(vk_device, vk_alias_vertex_buffer[asi], NULL);
                vk_alias_vertex_buffer[asi] = VK_NULL_HANDLE;
            }
            if (vk_alias_vertex_memory[asi] != VK_NULL_HANDLE) {
                if (vk_fn.FreeMemory)
                    vk_fn.FreeMemory(vk_device, vk_alias_vertex_memory[asi], NULL);
                vk_alias_vertex_memory[asi] = VK_NULL_HANDLE;
            }
            vk_alias_vertex_ptr[asi] = NULL;
        }
    }
    vk_alias_count           = 0;
    vk_alias_vertex_cursor   = 0;
    vk_alias_triangle_cursor = 0;
    vk_alias_skin_count      = 0;
    vk_alias_cmap_count      = 0;

    /* Phase 5b-05 sprite pipeline / resources teardown.
     * Reverse-create order. */
    if (vk_sprite_pipeline != VK_NULL_HANDLE) {
        if (vk_fn.DestroyPipeline)
            vk_fn.DestroyPipeline(vk_device, vk_sprite_pipeline, NULL);
        vk_sprite_pipeline = VK_NULL_HANDLE;
    }
    if (vk_sprite_pipeline_layout != VK_NULL_HANDLE) {
        if (vk_fn.DestroyPipelineLayout)
            vk_fn.DestroyPipelineLayout(vk_device,
                                        vk_sprite_pipeline_layout, NULL);
        vk_sprite_pipeline_layout = VK_NULL_HANDLE;
    }
    if (vk_sprite_pool != VK_NULL_HANDLE) {
        if (vk_fn.DestroyDescriptorPool)
            vk_fn.DestroyDescriptorPool(vk_device, vk_sprite_pool, NULL);
        vk_sprite_pool = VK_NULL_HANDLE;
        {
            unsigned ssi;
            for (ssi = 0; ssi < VK_FRAME_RING_DEPTH; ssi++)
                vk_sprite_set[ssi] = VK_NULL_HANDLE;
        }
    }
    if (vk_sprite_dsl != VK_NULL_HANDLE) {
        if (vk_fn.DestroyDescriptorSetLayout)
            vk_fn.DestroyDescriptorSetLayout(vk_device, vk_sprite_dsl, NULL);
        vk_sprite_dsl = VK_NULL_HANDLE;
    }
    if (vk_sprite_cs_module != VK_NULL_HANDLE) {
        if (vk_fn.DestroyShaderModule)
            vk_fn.DestroyShaderModule(vk_device, vk_sprite_cs_module, NULL);
        vk_sprite_cs_module = VK_NULL_HANDLE;
    }
    /* Phase 5b-08 step 2c: ring-buffered sprite SSBO
     * teardown.  Vertex + texture pair × N slots. */
    {
        unsigned ssi;
        for (ssi = 0; ssi < VK_FRAME_RING_DEPTH; ssi++) {
            if (vk_sprite_texture_buffer[ssi] != VK_NULL_HANDLE) {
                if (vk_fn.DestroyBuffer)
                    vk_fn.DestroyBuffer(vk_device, vk_sprite_texture_buffer[ssi], NULL);
                vk_sprite_texture_buffer[ssi] = VK_NULL_HANDLE;
            }
            if (vk_sprite_texture_memory[ssi] != VK_NULL_HANDLE) {
                if (vk_fn.FreeMemory)
                    vk_fn.FreeMemory(vk_device, vk_sprite_texture_memory[ssi], NULL);
                vk_sprite_texture_memory[ssi] = VK_NULL_HANDLE;
            }
            vk_sprite_texture_ptr[ssi] = NULL;

            if (vk_sprite_vertex_buffer[ssi] != VK_NULL_HANDLE) {
                if (vk_fn.DestroyBuffer)
                    vk_fn.DestroyBuffer(vk_device, vk_sprite_vertex_buffer[ssi], NULL);
                vk_sprite_vertex_buffer[ssi] = VK_NULL_HANDLE;
            }
            if (vk_sprite_vertex_memory[ssi] != VK_NULL_HANDLE) {
                if (vk_fn.FreeMemory)
                    vk_fn.FreeMemory(vk_device, vk_sprite_vertex_memory[ssi], NULL);
                vk_sprite_vertex_memory[ssi] = VK_NULL_HANDLE;
            }
            vk_sprite_vertex_ptr[ssi] = NULL;
        }
    }
    vk_sprite_count         = 0;
    vk_sprite_vertex_cursor = 0;

    /* Phase 5b-03 warp pipeline / resources teardown.
     * Reverse-create order; pool destruction implicitly
     * frees vk_warp_set.  The staging buffer / memory
     * pair, if non-NULL, is leftover state from a failed
     * create_resources -- destroy them too. */
    if (vk_warp_pipeline != VK_NULL_HANDLE) {
        if (vk_fn.DestroyPipeline)
            vk_fn.DestroyPipeline(vk_device, vk_warp_pipeline, NULL);
        vk_warp_pipeline = VK_NULL_HANDLE;
    }
    if (vk_warp_pipeline_layout != VK_NULL_HANDLE) {
        if (vk_fn.DestroyPipelineLayout)
            vk_fn.DestroyPipelineLayout(vk_device,
                                        vk_warp_pipeline_layout, NULL);
        vk_warp_pipeline_layout = VK_NULL_HANDLE;
    }
    if (vk_warp_pool != VK_NULL_HANDLE) {
        if (vk_fn.DestroyDescriptorPool)
            vk_fn.DestroyDescriptorPool(vk_device, vk_warp_pool, NULL);
        vk_warp_pool = VK_NULL_HANDLE;
        {
            unsigned wsi;
            for (wsi = 0; wsi < VK_FRAME_RING_DEPTH; wsi++)
                vk_warp_set[wsi] = VK_NULL_HANDLE;
        }
    }
    if (vk_warp_dsl != VK_NULL_HANDLE) {
        if (vk_fn.DestroyDescriptorSetLayout)
            vk_fn.DestroyDescriptorSetLayout(vk_device, vk_warp_dsl, NULL);
        vk_warp_dsl = VK_NULL_HANDLE;
    }
    if (vk_warp_cs_module != VK_NULL_HANDLE) {
        if (vk_fn.DestroyShaderModule)
            vk_fn.DestroyShaderModule(vk_device, vk_warp_cs_module, NULL);
        vk_warp_cs_module = VK_NULL_HANDLE;
    }
    if (vk_warp_table_staging != VK_NULL_HANDLE) {
        if (vk_fn.DestroyBuffer)
            vk_fn.DestroyBuffer(vk_device, vk_warp_table_staging, NULL);
        vk_warp_table_staging = VK_NULL_HANDLE;
    }
    if (vk_warp_table_staging_memory != VK_NULL_HANDLE) {
        if (vk_fn.FreeMemory)
            vk_fn.FreeMemory(vk_device, vk_warp_table_staging_memory, NULL);
        vk_warp_table_staging_memory = VK_NULL_HANDLE;
    }
    if (vk_warp_table_buffer != VK_NULL_HANDLE) {
        if (vk_fn.DestroyBuffer)
            vk_fn.DestroyBuffer(vk_device, vk_warp_table_buffer, NULL);
        vk_warp_table_buffer = VK_NULL_HANDLE;
    }
    if (vk_warp_table_memory != VK_NULL_HANDLE) {
        if (vk_fn.FreeMemory)
            vk_fn.FreeMemory(vk_device, vk_warp_table_memory, NULL);
        vk_warp_table_memory = VK_NULL_HANDLE;
    }
    vk_warp_active = false;

    /* Phase 5b-07a step 1: sky textures + staging.
     *
     * Reverse-create order: unmap+destroy staging buffer
     * first, then images / memory / views.  Mapped memory
     * is implicitly unmapped by FreeMemory but we Unmap
     * explicitly for symmetry with the MapMemory in create
     * and to make the lifetime obvious.
     *
     * CPU caches (vk_sky_cache_front, vk_sky_cache_back,
     * vk_sky_cache_populated) deliberately survive
     * destroy_resources.  They live in BSS / static storage,
     * outliving every Vulkan handle; on the next
     * create_resources the cache is still populated and
     * vk_sky_upload_pending gets re-armed so record_frame
     * re-pushes the data to the freshly-created images
     * without R_InitSky needing to re-fire from the engine
     * side.  We do clear vk_sky_upload_pending here since
     * the staging buffer it would target is gone -- create
     * will set it again. */
    /* Phase 5b-08 step 2d: N× sky_staging teardown. */
    {
        unsigned skssi;
        for (skssi = 0; skssi < VK_FRAME_RING_DEPTH; skssi++) {
            if (vk_sky_staging_memory[skssi] != VK_NULL_HANDLE
                && vk_sky_staging_ptr[skssi]) {
                if (vk_fn.UnmapMemory)
                    vk_fn.UnmapMemory(vk_device, vk_sky_staging_memory[skssi]);
                vk_sky_staging_ptr[skssi] = NULL;
            }
            if (vk_sky_staging[skssi] != VK_NULL_HANDLE) {
                if (vk_fn.DestroyBuffer)
                    vk_fn.DestroyBuffer(vk_device, vk_sky_staging[skssi], NULL);
                vk_sky_staging[skssi] = VK_NULL_HANDLE;
            }
            if (vk_sky_staging_memory[skssi] != VK_NULL_HANDLE) {
                if (vk_fn.FreeMemory)
                    vk_fn.FreeMemory(vk_device, vk_sky_staging_memory[skssi], NULL);
                vk_sky_staging_memory[skssi] = VK_NULL_HANDLE;
            }
        }
    }
    if (vk_sky_back_view != VK_NULL_HANDLE) {
        if (vk_fn.DestroyImageView)
            vk_fn.DestroyImageView(vk_device, vk_sky_back_view, NULL);
        vk_sky_back_view = VK_NULL_HANDLE;
    }
    if (vk_sky_back_image != VK_NULL_HANDLE) {
        if (vk_fn.DestroyImage)
            vk_fn.DestroyImage(vk_device, vk_sky_back_image, NULL);
        vk_sky_back_image = VK_NULL_HANDLE;
    }
    if (vk_sky_back_memory != VK_NULL_HANDLE) {
        if (vk_fn.FreeMemory)
            vk_fn.FreeMemory(vk_device, vk_sky_back_memory, NULL);
        vk_sky_back_memory = VK_NULL_HANDLE;
    }
    if (vk_sky_front_view != VK_NULL_HANDLE) {
        if (vk_fn.DestroyImageView)
            vk_fn.DestroyImageView(vk_device, vk_sky_front_view, NULL);
        vk_sky_front_view = VK_NULL_HANDLE;
    }
    if (vk_sky_front_image != VK_NULL_HANDLE) {
        if (vk_fn.DestroyImage)
            vk_fn.DestroyImage(vk_device, vk_sky_front_image, NULL);
        vk_sky_front_image = VK_NULL_HANDLE;
    }
    if (vk_sky_front_memory != VK_NULL_HANDLE) {
        if (vk_fn.FreeMemory)
            vk_fn.FreeMemory(vk_device, vk_sky_front_memory, NULL);
        vk_sky_front_memory = VK_NULL_HANDLE;
    }
    vk_sky_upload_pending = false;

    /* Phase 5b-07a step 2 + 3: sky compute pipeline +
     * mapped buffers.  Reverse-create order.  UnmapMemory
     * before FreeMemory for symmetry with the MapMemory in
     * create (and to make the lifetime obvious; FreeMemory
     * implicitly unmaps anyway).  Pool destruction
     * implicitly frees vk_sky_set so we just null its
     * handle.  vk_sky_collected* state lives in BSS and
     * survives; it's reset at the start of each frame in
     * record_frame's sky-dispatch path anyway. */
    /* Phase 5b-07b turb teardown (reverse-create order).
     * Pool destruction implicitly frees vk_turb_set; just
     * null the handle.  Per-frame state in vk_turb_surfaces
     * etc. lives in BSS and is reset at begin_frame anyway. */
    if (vk_turb_pipeline != VK_NULL_HANDLE) {
        if (vk_fn.DestroyPipeline)
            vk_fn.DestroyPipeline(vk_device, vk_turb_pipeline, NULL);
        vk_turb_pipeline = VK_NULL_HANDLE;
    }
    if (vk_turb_pipeline_layout != VK_NULL_HANDLE) {
        if (vk_fn.DestroyPipelineLayout)
            vk_fn.DestroyPipelineLayout(vk_device,
                                        vk_turb_pipeline_layout, NULL);
        vk_turb_pipeline_layout = VK_NULL_HANDLE;
    }
    if (vk_turb_pool != VK_NULL_HANDLE) {
        if (vk_fn.DestroyDescriptorPool)
            vk_fn.DestroyDescriptorPool(vk_device, vk_turb_pool, NULL);
        vk_turb_pool = VK_NULL_HANDLE;
        {
            unsigned tusi;
            for (tusi = 0; tusi < VK_FRAME_RING_DEPTH; tusi++)
                vk_turb_set[tusi] = VK_NULL_HANDLE;
        }
    }
    if (vk_turb_dsl != VK_NULL_HANDLE) {
        if (vk_fn.DestroyDescriptorSetLayout)
            vk_fn.DestroyDescriptorSetLayout(vk_device, vk_turb_dsl, NULL);
        vk_turb_dsl = VK_NULL_HANDLE;
    }
    if (vk_turb_cs_module != VK_NULL_HANDLE) {
        if (vk_fn.DestroyShaderModule)
            vk_fn.DestroyShaderModule(vk_device, vk_turb_cs_module, NULL);
        vk_turb_cs_module = VK_NULL_HANDLE;
    }
    /* Phase 5b-08 step 2d: N× turb spans/rows/atlas-staging
     * teardown. */
    {
        unsigned tusi;
        for (tusi = 0; tusi < VK_FRAME_RING_DEPTH; tusi++) {
            if (vk_turb_spans_memory[tusi] != VK_NULL_HANDLE
                && vk_turb_spans_ptr[tusi]) {
                if (vk_fn.UnmapMemory)
                    vk_fn.UnmapMemory(vk_device, vk_turb_spans_memory[tusi]);
                vk_turb_spans_ptr[tusi] = NULL;
            }
            if (vk_turb_spans_buffer[tusi] != VK_NULL_HANDLE) {
                if (vk_fn.DestroyBuffer)
                    vk_fn.DestroyBuffer(vk_device, vk_turb_spans_buffer[tusi], NULL);
                vk_turb_spans_buffer[tusi] = VK_NULL_HANDLE;
            }
            if (vk_turb_spans_memory[tusi] != VK_NULL_HANDLE) {
                if (vk_fn.FreeMemory)
                    vk_fn.FreeMemory(vk_device, vk_turb_spans_memory[tusi], NULL);
                vk_turb_spans_memory[tusi] = VK_NULL_HANDLE;
            }

            if (vk_turb_rows_memory[tusi] != VK_NULL_HANDLE
                && vk_turb_rows_ptr[tusi]) {
                if (vk_fn.UnmapMemory)
                    vk_fn.UnmapMemory(vk_device, vk_turb_rows_memory[tusi]);
                vk_turb_rows_ptr[tusi] = NULL;
            }
            if (vk_turb_rows_buffer[tusi] != VK_NULL_HANDLE) {
                if (vk_fn.DestroyBuffer)
                    vk_fn.DestroyBuffer(vk_device, vk_turb_rows_buffer[tusi], NULL);
                vk_turb_rows_buffer[tusi] = VK_NULL_HANDLE;
            }
            if (vk_turb_rows_memory[tusi] != VK_NULL_HANDLE) {
                if (vk_fn.FreeMemory)
                    vk_fn.FreeMemory(vk_device, vk_turb_rows_memory[tusi], NULL);
                vk_turb_rows_memory[tusi] = VK_NULL_HANDLE;
            }

            if (vk_turb_atlas_staging_memory[tusi] != VK_NULL_HANDLE
                && vk_turb_atlas_staging_ptr[tusi]) {
                if (vk_fn.UnmapMemory)
                    vk_fn.UnmapMemory(vk_device, vk_turb_atlas_staging_memory[tusi]);
                vk_turb_atlas_staging_ptr[tusi] = NULL;
            }
            if (vk_turb_atlas_staging[tusi] != VK_NULL_HANDLE) {
                if (vk_fn.DestroyBuffer)
                    vk_fn.DestroyBuffer(vk_device, vk_turb_atlas_staging[tusi], NULL);
                vk_turb_atlas_staging[tusi] = VK_NULL_HANDLE;
            }
            if (vk_turb_atlas_staging_memory[tusi] != VK_NULL_HANDLE) {
                if (vk_fn.FreeMemory)
                    vk_fn.FreeMemory(vk_device, vk_turb_atlas_staging_memory[tusi], NULL);
                vk_turb_atlas_staging_memory[tusi] = VK_NULL_HANDLE;
            }
        }
    }
    if (vk_turb_atlas_view != VK_NULL_HANDLE) {
        if (vk_fn.DestroyImageView)
            vk_fn.DestroyImageView(vk_device, vk_turb_atlas_view, NULL);
        vk_turb_atlas_view = VK_NULL_HANDLE;
    }
    if (vk_turb_atlas_image != VK_NULL_HANDLE) {
        if (vk_fn.DestroyImage)
            vk_fn.DestroyImage(vk_device, vk_turb_atlas_image, NULL);
        vk_turb_atlas_image = VK_NULL_HANDLE;
    }
    if (vk_turb_atlas_memory != VK_NULL_HANDLE) {
        if (vk_fn.FreeMemory)
            vk_fn.FreeMemory(vk_device, vk_turb_atlas_memory, NULL);
        vk_turb_atlas_memory = VK_NULL_HANDLE;
    }
    vk_turb_surface_count   = 0;
    vk_turb_rows_used       = 0;
    vk_turb_spans_used      = 0;
    vk_turb_tex_count       = 0;
    vk_turb_tex_upload_first= 0;
    vk_turb_tex_upload_last = 0;

    /* Phase 5b-07 step 2: brush atlas teardown.  Allocator first
     * (frees its strip free-lists + entry pool); then map / buffer
     * / memory for staging; then view / image / memory for the
     * atlas storage image.  Mirrors the create-order in reverse
     * with the same null-guard pattern as the turb teardown
     * above. */
    if (vk_brush_atlas) {
        surf_atlas_destroy(vk_brush_atlas);
        vk_brush_atlas = NULL;
    }
    if (vk_brush_atlas_staging_memory != VK_NULL_HANDLE
        && vk_brush_atlas_staging_ptr) {
        if (vk_fn.UnmapMemory)
            vk_fn.UnmapMemory(vk_device, vk_brush_atlas_staging_memory);
        vk_brush_atlas_staging_ptr = NULL;
    }
    if (vk_brush_atlas_staging != VK_NULL_HANDLE) {
        if (vk_fn.DestroyBuffer)
            vk_fn.DestroyBuffer(vk_device, vk_brush_atlas_staging, NULL);
        vk_brush_atlas_staging = VK_NULL_HANDLE;
    }
    if (vk_brush_atlas_staging_memory != VK_NULL_HANDLE) {
        if (vk_fn.FreeMemory)
            vk_fn.FreeMemory(vk_device, vk_brush_atlas_staging_memory, NULL);
        vk_brush_atlas_staging_memory = VK_NULL_HANDLE;
    }
    if (vk_brush_atlas_view != VK_NULL_HANDLE) {
        if (vk_fn.DestroyImageView)
            vk_fn.DestroyImageView(vk_device, vk_brush_atlas_view, NULL);
        vk_brush_atlas_view = VK_NULL_HANDLE;
    }
    if (vk_brush_atlas_image != VK_NULL_HANDLE) {
        if (vk_fn.DestroyImage)
            vk_fn.DestroyImage(vk_device, vk_brush_atlas_image, NULL);
        vk_brush_atlas_image = VK_NULL_HANDLE;
    }
    if (vk_brush_atlas_memory != VK_NULL_HANDLE) {
        if (vk_fn.FreeMemory)
            vk_fn.FreeMemory(vk_device, vk_brush_atlas_memory, NULL);
        vk_brush_atlas_memory = VK_NULL_HANDLE;
    }

    if (vk_sky_pipeline != VK_NULL_HANDLE) {
        if (vk_fn.DestroyPipeline)
            vk_fn.DestroyPipeline(vk_device, vk_sky_pipeline, NULL);
        vk_sky_pipeline = VK_NULL_HANDLE;
    }
    if (vk_sky_pipeline_layout != VK_NULL_HANDLE) {
        if (vk_fn.DestroyPipelineLayout)
            vk_fn.DestroyPipelineLayout(vk_device,
                                        vk_sky_pipeline_layout, NULL);
        vk_sky_pipeline_layout = VK_NULL_HANDLE;
    }
    if (vk_sky_pool != VK_NULL_HANDLE) {
        if (vk_fn.DestroyDescriptorPool)
            vk_fn.DestroyDescriptorPool(vk_device, vk_sky_pool, NULL);
        vk_sky_pool = VK_NULL_HANDLE;
        {
            unsigned sksi;
            for (sksi = 0; sksi < VK_FRAME_RING_DEPTH; sksi++)
                vk_sky_set[sksi] = VK_NULL_HANDLE;
        }
    }
    if (vk_sky_dsl != VK_NULL_HANDLE) {
        if (vk_fn.DestroyDescriptorSetLayout)
            vk_fn.DestroyDescriptorSetLayout(vk_device, vk_sky_dsl, NULL);
        vk_sky_dsl = VK_NULL_HANDLE;
    }
    if (vk_sky_cs_module != VK_NULL_HANDLE) {
        if (vk_fn.DestroyShaderModule)
            vk_fn.DestroyShaderModule(vk_device, vk_sky_cs_module, NULL);
        vk_sky_cs_module = VK_NULL_HANDLE;
    }
    /* Phase 5b-08 step 2d: N× sky ubo/rows/spans teardown. */
    {
        unsigned sksi;
        for (sksi = 0; sksi < VK_FRAME_RING_DEPTH; sksi++) {
            if (vk_sky_spans_memory[sksi] != VK_NULL_HANDLE
                && vk_sky_spans_ptr[sksi]) {
                if (vk_fn.UnmapMemory)
                    vk_fn.UnmapMemory(vk_device, vk_sky_spans_memory[sksi]);
                vk_sky_spans_ptr[sksi] = NULL;
            }
            if (vk_sky_spans_buffer[sksi] != VK_NULL_HANDLE) {
                if (vk_fn.DestroyBuffer)
                    vk_fn.DestroyBuffer(vk_device, vk_sky_spans_buffer[sksi], NULL);
                vk_sky_spans_buffer[sksi] = VK_NULL_HANDLE;
            }
            if (vk_sky_spans_memory[sksi] != VK_NULL_HANDLE) {
                if (vk_fn.FreeMemory)
                    vk_fn.FreeMemory(vk_device, vk_sky_spans_memory[sksi], NULL);
                vk_sky_spans_memory[sksi] = VK_NULL_HANDLE;
            }

            if (vk_sky_rows_memory[sksi] != VK_NULL_HANDLE
                && vk_sky_rows_ptr[sksi]) {
                if (vk_fn.UnmapMemory)
                    vk_fn.UnmapMemory(vk_device, vk_sky_rows_memory[sksi]);
                vk_sky_rows_ptr[sksi] = NULL;
            }
            if (vk_sky_rows_buffer[sksi] != VK_NULL_HANDLE) {
                if (vk_fn.DestroyBuffer)
                    vk_fn.DestroyBuffer(vk_device, vk_sky_rows_buffer[sksi], NULL);
                vk_sky_rows_buffer[sksi] = VK_NULL_HANDLE;
            }
            if (vk_sky_rows_memory[sksi] != VK_NULL_HANDLE) {
                if (vk_fn.FreeMemory)
                    vk_fn.FreeMemory(vk_device, vk_sky_rows_memory[sksi], NULL);
                vk_sky_rows_memory[sksi] = VK_NULL_HANDLE;
            }

            if (vk_sky_ubo_memory[sksi] != VK_NULL_HANDLE
                && vk_sky_ubo_ptr[sksi]) {
                if (vk_fn.UnmapMemory)
                    vk_fn.UnmapMemory(vk_device, vk_sky_ubo_memory[sksi]);
                vk_sky_ubo_ptr[sksi] = NULL;
            }
            if (vk_sky_ubo_buffer[sksi] != VK_NULL_HANDLE) {
                if (vk_fn.DestroyBuffer)
                    vk_fn.DestroyBuffer(vk_device, vk_sky_ubo_buffer[sksi], NULL);
                vk_sky_ubo_buffer[sksi] = VK_NULL_HANDLE;
            }
            if (vk_sky_ubo_memory[sksi] != VK_NULL_HANDLE) {
                if (vk_fn.FreeMemory)
                    vk_fn.FreeMemory(vk_device, vk_sky_ubo_memory[sksi], NULL);
                vk_sky_ubo_memory[sksi] = VK_NULL_HANDLE;
            }
        }
    }
    vk_sky_collected_count = 0;

    /* Phase 5b-02 particle pipeline / resources teardown.
     * Reverse-create order; pool destruction implicitly
     * frees vk_particles_set.  Mapped memory is implicitly
     * unmapped by FreeMemory. */
    if (vk_particles_pipeline != VK_NULL_HANDLE) {
        if (vk_fn.DestroyPipeline)
            vk_fn.DestroyPipeline(vk_device, vk_particles_pipeline, NULL);
        vk_particles_pipeline = VK_NULL_HANDLE;
    }
    if (vk_particles_pipeline_layout != VK_NULL_HANDLE) {
        if (vk_fn.DestroyPipelineLayout)
            vk_fn.DestroyPipelineLayout(vk_device,
                                        vk_particles_pipeline_layout, NULL);
        vk_particles_pipeline_layout = VK_NULL_HANDLE;
    }
    if (vk_particles_pool != VK_NULL_HANDLE) {
        if (vk_fn.DestroyDescriptorPool)
            vk_fn.DestroyDescriptorPool(vk_device, vk_particles_pool, NULL);
        vk_particles_pool = VK_NULL_HANDLE;
        /* DestroyDescriptorPool implicitly frees every set
         * allocated from it.  Zero all N entries. */
        {
            unsigned psi;
            for (psi = 0; psi < VK_FRAME_RING_DEPTH; psi++)
                vk_particles_set[psi] = VK_NULL_HANDLE;
        }
    }
    if (vk_particles_dsl != VK_NULL_HANDLE) {
        if (vk_fn.DestroyDescriptorSetLayout)
            vk_fn.DestroyDescriptorSetLayout(vk_device,
                                             vk_particles_dsl, NULL);
        vk_particles_dsl = VK_NULL_HANDLE;
    }
    if (vk_particles_cs_module != VK_NULL_HANDLE) {
        if (vk_fn.DestroyShaderModule)
            vk_fn.DestroyShaderModule(vk_device,
                                      vk_particles_cs_module, NULL);
        vk_particles_cs_module = VK_NULL_HANDLE;
    }
    /* Phase 5b-08 step 2a: ring-buffered zbuf staging
     * teardown.  Same pattern as vid.buffer staging --
     * vkFreeMemory implicitly unmaps the persistent
     * mapping; zero each slot's ptr to NULL-deref a
     * stale dereference instead of use-after-freeing. */
    {
        unsigned zsi;
        for (zsi = 0; zsi < VK_FRAME_RING_DEPTH; zsi++) {
            if (vk_particles_zstaging[zsi] != VK_NULL_HANDLE) {
                if (vk_fn.DestroyBuffer)
                    vk_fn.DestroyBuffer(vk_device, vk_particles_zstaging[zsi], NULL);
                vk_particles_zstaging[zsi] = VK_NULL_HANDLE;
            }
            if (vk_particles_zstaging_memory[zsi] != VK_NULL_HANDLE) {
                if (vk_fn.FreeMemory)
                    vk_fn.FreeMemory(vk_device, vk_particles_zstaging_memory[zsi], NULL);
                vk_particles_zstaging_memory[zsi] = VK_NULL_HANDLE;
            }
            vk_particles_zstaging_ptr[zsi] = NULL;
        }
    }
    /* Phase 5b-08 step 2b: ring-buffered particles SSBO
     * teardown.  Same per-slot pattern. */
    {
        unsigned psi;
        for (psi = 0; psi < VK_FRAME_RING_DEPTH; psi++) {
            if (vk_particles_buffer[psi] != VK_NULL_HANDLE) {
                if (vk_fn.DestroyBuffer)
                    vk_fn.DestroyBuffer(vk_device, vk_particles_buffer[psi], NULL);
                vk_particles_buffer[psi] = VK_NULL_HANDLE;
            }
            if (vk_particles_memory[psi] != VK_NULL_HANDLE) {
                if (vk_fn.FreeMemory)
                    vk_fn.FreeMemory(vk_device, vk_particles_memory[psi], NULL);
                vk_particles_memory[psi] = VK_NULL_HANDLE;
            }
            vk_particles_ptr[psi] = NULL;
        }
    }
    if (vk_zbuffer_view != VK_NULL_HANDLE) {
        if (vk_fn.DestroyImageView)
            vk_fn.DestroyImageView(vk_device, vk_zbuffer_view, NULL);
        vk_zbuffer_view = VK_NULL_HANDLE;
    }
    if (vk_zbuffer != VK_NULL_HANDLE) {
        if (vk_fn.DestroyImage)
            vk_fn.DestroyImage(vk_device, vk_zbuffer, NULL);
        vk_zbuffer = VK_NULL_HANDLE;
    }
    if (vk_zbuffer_memory != VK_NULL_HANDLE) {
        if (vk_fn.FreeMemory)
            vk_fn.FreeMemory(vk_device, vk_zbuffer_memory, NULL);
        vk_zbuffer_memory = VK_NULL_HANDLE;
    }
    vk_pending_particle_count = 0;

    if (vk_cmd_pool != VK_NULL_HANDLE) {
        /* Ring-slot fences + render-done semaphores first.
         * Both live inside vk_frame_ctx[] and are
         * independent of the command pool's lifetime;
         * destroy them explicitly.  The CBs themselves are
         * freed implicitly by the DestroyCommandPool call
         * below (it frees every buffer allocated from the
         * pool). */
        {
            unsigned si;
            for (si = 0; si < VK_FRAME_RING_DEPTH; si++) {
                if (vk_frame_ctx[si].done_fence != VK_NULL_HANDLE
                    && vk_fn.DestroyFence)
                    vk_fn.DestroyFence(vk_device,
                                       vk_frame_ctx[si].done_fence, NULL);
                vk_frame_ctx[si].done_fence = VK_NULL_HANDLE;
                /* Phase 5b-08 step 3b: render-done
                 * semaphore teardown.  Semaphore must not
                 * be in flight (signaled but not yet
                 * waited on) at destroy time; in practice
                 * destroy_resources is only called at
                 * context-destroy or context-reset, both
                 * of which run after the device is idle
                 * via the surrounding teardown sequence. */
                if (vk_frame_ctx[si].render_done_semaphore != VK_NULL_HANDLE
                    && vk_fn.DestroySemaphore)
                    vk_fn.DestroySemaphore(vk_device,
                                           vk_frame_ctx[si].render_done_semaphore, NULL);
                vk_frame_ctx[si].render_done_semaphore = VK_NULL_HANDLE;
                vk_frame_ctx[si].cmd_buffer = VK_NULL_HANDLE;
            }
            vk_active_slot = 0;
        }

        /* Phase 5b-08 step 3b: upload fence teardown. */
        if (vk_upload_fence != VK_NULL_HANDLE && vk_fn.DestroyFence)
            vk_fn.DestroyFence(vk_device, vk_upload_fence, NULL);
        vk_upload_fence = VK_NULL_HANDLE;

        /* DestroyCommandPool frees every buffer allocated
         * from it; no separate FreeCommandBuffers call
         * needed.  This also frees the one-shot upload
         * command buffer if it was still allocated. */
        if (vk_fn.DestroyCommandPool)
            vk_fn.DestroyCommandPool(vk_device, vk_cmd_pool, NULL);
        vk_cmd_pool   = VK_NULL_HANDLE;
        vk_cmd_buffer = VK_NULL_HANDLE;
    }
    /* Phase 4c descriptor / sampler / staging / texture
     * teardown.  Reverse-create order, except that descriptor
     * pool destruction implicitly frees the allocated set, so
     * vk_descriptor_set just gets zeroed without a separate
     * Free call. */
    if (vk_descriptor_pool != VK_NULL_HANDLE) {
        if (vk_fn.DestroyDescriptorPool)
            vk_fn.DestroyDescriptorPool(vk_device, vk_descriptor_pool, NULL);
        vk_descriptor_pool = VK_NULL_HANDLE;
        {
            unsigned dsi;
            for (dsi = 0; dsi < VK_FRAME_RING_DEPTH; dsi++)
                vk_descriptor_set[dsi] = VK_NULL_HANDLE;
        }
    }
    if (vk_descriptor_set_layout != VK_NULL_HANDLE) {
        if (vk_fn.DestroyDescriptorSetLayout)
            vk_fn.DestroyDescriptorSetLayout(vk_device,
                                             vk_descriptor_set_layout, NULL);
        vk_descriptor_set_layout = VK_NULL_HANDLE;
    }
    /* Phase 5b-08 step 2a: ring-buffered staging teardown.
     * vkFreeMemory implicitly unmaps any persistent mapping
     * for that allocation; no UnmapMemory needed.  Zero
     * each slot's ptr so a stale-pointer dereference after
     * teardown would be a NULL deref rather than a
     * use-after-free on freed device memory. */
    {
        unsigned si;
        for (si = 0; si < VK_FRAME_RING_DEPTH; si++) {
            if (vk_staging_buffer[si] != VK_NULL_HANDLE) {
                if (vk_fn.DestroyBuffer)
                    vk_fn.DestroyBuffer(vk_device, vk_staging_buffer[si], NULL);
                vk_staging_buffer[si] = VK_NULL_HANDLE;
            }
            if (vk_staging_memory[si] != VK_NULL_HANDLE) {
                if (vk_fn.FreeMemory)
                    vk_fn.FreeMemory(vk_device, vk_staging_memory[si], NULL);
                vk_staging_memory[si] = VK_NULL_HANDLE;
            }
            vk_staging_ptr[si] = NULL;
        }
    }
    vk_staging_size = 0;
    if (vk_texture_view != VK_NULL_HANDLE) {
        if (vk_fn.DestroyImageView)
            vk_fn.DestroyImageView(vk_device, vk_texture_view, NULL);
        vk_texture_view = VK_NULL_HANDLE;
    }
    if (vk_texture != VK_NULL_HANDLE) {
        if (vk_fn.DestroyImage)
            vk_fn.DestroyImage(vk_device, vk_texture, NULL);
        vk_texture = VK_NULL_HANDLE;
    }
    if (vk_texture_memory != VK_NULL_HANDLE) {
        if (vk_fn.FreeMemory)
            vk_fn.FreeMemory(vk_device, vk_texture_memory, NULL);
        vk_texture_memory = VK_NULL_HANDLE;
    }
    if (vk_palette_texture_view != VK_NULL_HANDLE) {
        if (vk_fn.DestroyImageView)
            vk_fn.DestroyImageView(vk_device, vk_palette_texture_view, NULL);
        vk_palette_texture_view = VK_NULL_HANDLE;
    }
    if (vk_palette_texture != VK_NULL_HANDLE) {
        if (vk_fn.DestroyImage)
            vk_fn.DestroyImage(vk_device, vk_palette_texture, NULL);
        vk_palette_texture = VK_NULL_HANDLE;
    }
    if (vk_palette_texture_memory != VK_NULL_HANDLE) {
        if (vk_fn.FreeMemory)
            vk_fn.FreeMemory(vk_device, vk_palette_texture_memory, NULL);
        vk_palette_texture_memory = VK_NULL_HANDLE;
    }
    if (vk_sampler != VK_NULL_HANDLE) {
        if (vk_fn.DestroySampler)
            vk_fn.DestroySampler(vk_device, vk_sampler, NULL);
        vk_sampler = VK_NULL_HANDLE;
    }
    /* Phase 4h overlay objects: reverse-create order
     * within this group, and this whole group goes before
     * the Phase 4g compute objects.  The overlay pipeline
     * references the overlay layout (which has no
     * descriptors so isn't tied to the compute DSL), the
     * overlay render pass, and the overlay shader
     * modules; the framebuffer references the overlay
     * render pass + vk_image_view; the vertex buffer
     * stands alone. */
    /* Phase 5b-08 step 2e: N× overlay vertex teardown. */
    {
        unsigned ovsi;
        for (ovsi = 0; ovsi < VK_FRAME_RING_DEPTH; ovsi++) {
            if (vk_overlay_vertex_buffer[ovsi] != VK_NULL_HANDLE) {
                if (vk_fn.DestroyBuffer)
                    vk_fn.DestroyBuffer(vk_device, vk_overlay_vertex_buffer[ovsi], NULL);
                vk_overlay_vertex_buffer[ovsi] = VK_NULL_HANDLE;
            }
            if (vk_overlay_vertex_memory[ovsi] != VK_NULL_HANDLE) {
                if (vk_fn.FreeMemory)
                    vk_fn.FreeMemory(vk_device, vk_overlay_vertex_memory[ovsi], NULL);
                vk_overlay_vertex_memory[ovsi] = VK_NULL_HANDLE;
            }
            vk_overlay_vertex_ptr[ovsi] = NULL;
        }
    }
    if (vk_overlay_pipeline != VK_NULL_HANDLE) {
        if (vk_fn.DestroyPipeline)
            vk_fn.DestroyPipeline(vk_device, vk_overlay_pipeline, NULL);
        vk_overlay_pipeline = VK_NULL_HANDLE;
    }
    /* No vk_overlay_pipeline_layout to destroy at Phase 4i+;
     * the overlay pipeline shares vk_pipeline_layout with
     * the compute pipeline, which is torn down below. */
    {
        /* Phase 4j: loop the slot array tearing down every
         * populated entry.  An unused slot has image ==
         * VK_NULL_HANDLE (BSS zero-init guarantees this on
         * first create_resources; we re-zero after each
         * destroy so a second create_resources won't see
         * stale handles).  descriptor_set is freed
         * implicitly when vk_descriptor_pool is destroyed
         * below.
         *
         * Phase 5b-06 follow-up: now routed through
         * backend_vk_free_overlay_slot, which is also called
         * by the mid-flight LRU eviction in begin_frame; one
         * helper keeps the slot-teardown invariants in lock-
         * step. */
        unsigned si;
        for (si = 0; si < OVERLAY_SLOT_MAX; si++)
            backend_vk_free_overlay_slot(&vk_overlay_slots[si]);
        vk_overlay_draw_count    = 0;
        vk_overlay_upload_offset = 0;
    }
    /* Phase 5b-08 step 3a: N× overlay framebuffer
     * teardown.  Each slot's framebuffer wrapped its
     * slot's vk_image_view; destroy in matching order. */
    {
        unsigned ovfi;
        for (ovfi = 0; ovfi < VK_FRAME_RING_DEPTH; ovfi++) {
            if (vk_overlay_framebuffer[ovfi] != VK_NULL_HANDLE) {
                if (vk_fn.DestroyFramebuffer)
                    vk_fn.DestroyFramebuffer(vk_device, vk_overlay_framebuffer[ovfi], NULL);
                vk_overlay_framebuffer[ovfi] = VK_NULL_HANDLE;
            }
        }
    }
    if (vk_overlay_render_pass != VK_NULL_HANDLE) {
        if (vk_fn.DestroyRenderPass)
            vk_fn.DestroyRenderPass(vk_device, vk_overlay_render_pass, NULL);
        vk_overlay_render_pass = VK_NULL_HANDLE;
    }
    if (vk_overlay_fs_module != VK_NULL_HANDLE) {
        if (vk_fn.DestroyShaderModule)
            vk_fn.DestroyShaderModule(vk_device, vk_overlay_fs_module, NULL);
        vk_overlay_fs_module = VK_NULL_HANDLE;
    }
    if (vk_overlay_vs_module != VK_NULL_HANDLE) {
        if (vk_fn.DestroyShaderModule)
            vk_fn.DestroyShaderModule(vk_device, vk_overlay_vs_module, NULL);
        vk_overlay_vs_module = VK_NULL_HANDLE;
    }
    /* Phase 4g pipeline objects: reverse-create order.  The
     * compute pipeline references vk_pipeline_layout, which
     * references the DSL; the shader module can go any time
     * after pipeline destruction. */
    if (vk_compute_pipeline != VK_NULL_HANDLE) {
        if (vk_fn.DestroyPipeline)
            vk_fn.DestroyPipeline(vk_device, vk_compute_pipeline, NULL);
        vk_compute_pipeline = VK_NULL_HANDLE;
    }
    if (vk_pipeline_layout != VK_NULL_HANDLE) {
        if (vk_fn.DestroyPipelineLayout)
            vk_fn.DestroyPipelineLayout(vk_device, vk_pipeline_layout, NULL);
        vk_pipeline_layout = VK_NULL_HANDLE;
    }
    if (vk_cs_module != VK_NULL_HANDLE) {
        if (vk_fn.DestroyShaderModule)
            vk_fn.DestroyShaderModule(vk_device, vk_cs_module, NULL);
        vk_cs_module = VK_NULL_HANDLE;
    }
    /* Phase 5b-08 step 3a: N× vk_image / _view / _memory
     * teardown.  Reverse-create order per slot. */
    {
        unsigned imsi;
        for (imsi = 0; imsi < VK_FRAME_RING_DEPTH; imsi++) {
            if (vk_image_view[imsi] != VK_NULL_HANDLE) {
                if (vk_fn.DestroyImageView)
                    vk_fn.DestroyImageView(vk_device, vk_image_view[imsi], NULL);
                vk_image_view[imsi] = VK_NULL_HANDLE;
            }
            if (vk_image[imsi] != VK_NULL_HANDLE) {
                if (vk_fn.DestroyImage)
                    vk_fn.DestroyImage(vk_device, vk_image[imsi], NULL);
                vk_image[imsi] = VK_NULL_HANDLE;
            }
            if (vk_image_memory[imsi] != VK_NULL_HANDLE) {
                if (vk_fn.FreeMemory)
                    vk_fn.FreeMemory(vk_device, vk_image_memory[imsi], NULL);
                vk_image_memory[imsi] = VK_NULL_HANDLE;
            }
            memset(&vk_retro_image[imsi], 0, sizeof(vk_retro_image[imsi]));
        }
    }
    vk_resources_ready = false;
}

static void
backend_vk_load_fn(void)
{
    /* Instance-level loads. */
    if (vk_fn.GetInstanceProcAddr && vk_instance != VK_NULL_HANDLE) {
        vk_fn.GetPhysicalDeviceProperties = (PFN_vkGetPhysicalDeviceProperties)
            vk_fn.GetInstanceProcAddr(vk_instance, "vkGetPhysicalDeviceProperties");
        vk_fn.GetPhysicalDeviceMemoryProperties = (PFN_vkGetPhysicalDeviceMemoryProperties)
            vk_fn.GetInstanceProcAddr(vk_instance, "vkGetPhysicalDeviceMemoryProperties");
    }

    /* Device-level loads. */
    if (vk_fn.GetDeviceProcAddr && vk_device != VK_NULL_HANDLE) {
        #define LOAD_DEV(name) \
            vk_fn.name = (PFN_vk##name) \
                vk_fn.GetDeviceProcAddr(vk_device, "vk" #name)
        LOAD_DEV(DeviceWaitIdle);
        LOAD_DEV(CreateImage);
        LOAD_DEV(DestroyImage);
        LOAD_DEV(GetImageMemoryRequirements);
        LOAD_DEV(AllocateMemory);
        LOAD_DEV(FreeMemory);
        LOAD_DEV(BindImageMemory);
        LOAD_DEV(CreateImageView);
        LOAD_DEV(DestroyImageView);
        LOAD_DEV(CreateCommandPool);
        LOAD_DEV(DestroyCommandPool);
        LOAD_DEV(AllocateCommandBuffers);
        LOAD_DEV(FreeCommandBuffers);
        LOAD_DEV(BeginCommandBuffer);
        LOAD_DEV(EndCommandBuffer);
        LOAD_DEV(CmdPipelineBarrier);
        LOAD_DEV(CmdClearColorImage);
        LOAD_DEV(QueueSubmit);
        LOAD_DEV(QueueWaitIdle);
        LOAD_DEV(CreateFence);
        LOAD_DEV(DestroyFence);
        LOAD_DEV(WaitForFences);
        LOAD_DEV(ResetFences);
        /* Phase 5b-08 step 3b: render-done semaphores. */
        LOAD_DEV(CreateSemaphore);
        LOAD_DEV(DestroySemaphore);
        /* Phase 4b for shader/pipeline-layout, Phase 4g
         * for the compute pipeline, Phase 4h for the
         * overlay graphics pipeline + render pass +
         * framebuffer + vertex-buffer bind. */
        LOAD_DEV(CreateShaderModule);
        LOAD_DEV(DestroyShaderModule);
        LOAD_DEV(CreatePipelineLayout);
        LOAD_DEV(DestroyPipelineLayout);
        LOAD_DEV(CreateComputePipelines);
        LOAD_DEV(CreateGraphicsPipelines);
        LOAD_DEV(DestroyPipeline);
        LOAD_DEV(CreateRenderPass);
        LOAD_DEV(DestroyRenderPass);
        LOAD_DEV(CreateFramebuffer);
        LOAD_DEV(DestroyFramebuffer);
        LOAD_DEV(CmdBindPipeline);
        LOAD_DEV(CmdDispatch);
        LOAD_DEV(CmdBeginRenderPass);
        LOAD_DEV(CmdEndRenderPass);
        LOAD_DEV(CmdDraw);
        LOAD_DEV(CmdBindVertexBuffers);
        /* Phase 4c additions */
        LOAD_DEV(CreateSampler);
        LOAD_DEV(DestroySampler);
        LOAD_DEV(CreateBuffer);
        LOAD_DEV(DestroyBuffer);
        LOAD_DEV(GetBufferMemoryRequirements);
        LOAD_DEV(BindBufferMemory);
        LOAD_DEV(MapMemory);
        LOAD_DEV(UnmapMemory);
        LOAD_DEV(CreateDescriptorSetLayout);
        LOAD_DEV(DestroyDescriptorSetLayout);
        LOAD_DEV(CreateDescriptorPool);
        LOAD_DEV(DestroyDescriptorPool);
        LOAD_DEV(AllocateDescriptorSets);
        LOAD_DEV(UpdateDescriptorSets);
        LOAD_DEV(ResetCommandBuffer);
        LOAD_DEV(CmdCopyBufferToImage);
        LOAD_DEV(CmdBindDescriptorSets);
        LOAD_DEV(CmdPushConstants);
        LOAD_DEV(CmdCopyBuffer);
        #undef LOAD_DEV
    }
}

static void
backend_vk_context_reset(void)
{
    const struct retro_hw_render_interface *iface_base = NULL;
    const struct retro_hw_render_interface_vulkan *vki = NULL;
    VkPhysicalDeviceProperties props;

    if (!environ_cb)
        return;

    if (!environ_cb(RETRO_ENVIRONMENT_GET_HW_RENDER_INTERFACE,
                    (void *)&iface_base) || !iface_base) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: GET_HW_RENDER_INTERFACE failed in context_reset\n");
        return;
    }
    if (iface_base->interface_type != RETRO_HW_RENDER_INTERFACE_VULKAN) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: frontend returned wrong interface_type %d (expected %d)\n",
                   (int)iface_base->interface_type,
                   (int)RETRO_HW_RENDER_INTERFACE_VULKAN);
        return;
    }
    if (iface_base->interface_version != RETRO_HW_RENDER_INTERFACE_VULKAN_VERSION) {
        if (log_cb)
            log_cb(RETRO_LOG_WARN,
                   "rhi-vk: interface_version mismatch: frontend %u, expected %u\n",
                   iface_base->interface_version,
                   (unsigned)RETRO_HW_RENDER_INTERFACE_VULKAN_VERSION);
    }

    vki             = (const struct retro_hw_render_interface_vulkan *)iface_base;
    vk_iface        = vki;
    vk_instance     = vki->instance;
    vk_gpu          = vki->gpu;
    vk_device       = vki->device;
    vk_queue        = vki->queue;
    vk_queue_family = vki->queue_index;

    vk_fn.GetInstanceProcAddr = vki->get_instance_proc_addr;
    vk_fn.GetDeviceProcAddr   = vki->get_device_proc_addr;

    backend_vk_load_fn();

    if (vk_fn.GetPhysicalDeviceProperties && vk_gpu != VK_NULL_HANDLE) {
        memset(&props, 0, sizeof(props));
        vk_fn.GetPhysicalDeviceProperties(vk_gpu, &props);
        if (log_cb)
            log_cb(RETRO_LOG_INFO,
                   "rhi-vk: device='%s' apiVersion=%u.%u.%u driverVersion=0x%08x vendor=0x%04x device=0x%04x\n",
                   props.deviceName,
                   VK_VERSION_MAJOR(props.apiVersion),
                   VK_VERSION_MINOR(props.apiVersion),
                   VK_VERSION_PATCH(props.apiVersion),
                   props.driverVersion,
                   props.vendorID,
                   props.deviceID);
    } else if (log_cb) {
        log_cb(RETRO_LOG_WARN,
               "rhi-vk: could not load GetPhysicalDeviceProperties; device identification unavailable\n");
    }

    if (log_cb)
        log_cb(RETRO_LOG_INFO,
               "rhi-vk: context up; queue_family=%u\n", vk_queue_family);

    /* Stand up the per-context Vulkan resources.  On failure
     * roll back so context_destroy doesn't have to handle
     * a half-constructed state -- create_resources logged
     * the specific step that failed. */
    if (!backend_vk_create_resources()) {
        backend_vk_destroy_resources();
        if (log_cb)
            log_cb(RETRO_LOG_ERROR,
                   "rhi-vk: resource creation failed; end_frame will no-op\n");
    }
}

static void
backend_vk_context_destroy(void)
{
    backend_vk_destroy_resources();

    vk_iface        = NULL;
    vk_instance     = VK_NULL_HANDLE;
    vk_gpu          = VK_NULL_HANDLE;
    vk_device       = VK_NULL_HANDLE;
    vk_queue        = VK_NULL_HANDLE;
    vk_queue_family = 0;
    memset(&vk_fn, 0, sizeof(vk_fn));

    if (log_cb)
        log_cb(RETRO_LOG_INFO, "rhi-vk: context destroyed\n");
}

/*
 * Per-frame: walk vk_overlay_draws and write 4 vertices
 * per entry into vk_overlay_vertex_buffer.  Phase 4j used
 * to fill this once at create_resources; Phase 4k moves
 * the fill here so the draw list can change per frame
 * (which it will, now that backend_vk_queue_2d_pic
 * populates it from Draw_Pic intercepts during retro_run).
 *
 * Buffer is HOST_VISIBLE + HOST_COHERENT and persistently
 * mapped (vk_overlay_vertex_ptr), so the write is just
 * memory; no flush required.  Strip winding is the same
 * as in Phase 4i: v0 bottom-left, v1 bottom-right, v2
 * top-left, v3 top-right, four floats per vertex
 * (pos.x pos.y u v).
 */
static void
backend_vk_fill_overlay_vb(void)
{
    float   *dst = (float *)vk_overlay_vertex_ptr[vk_active_slot];
    unsigned di;

    for (di = 0; di < vk_overlay_draw_count; di++) {
        const struct overlay_draw *d = &vk_overlay_draws[di];
        /* bottom-left  */
        dst[ 0] = d->x0; dst[ 1] = d->y1; dst[ 2] = d->u0; dst[ 3] = d->v1;
        /* bottom-right */
        dst[ 4] = d->x1; dst[ 5] = d->y1; dst[ 6] = d->u1; dst[ 7] = d->v1;
        /* top-left     */
        dst[ 8] = d->x0; dst[ 9] = d->y0; dst[10] = d->u0; dst[11] = d->v0;
        /* top-right    */
        dst[12] = d->x1; dst[13] = d->y0; dst[14] = d->u1; dst[15] = d->v0;
        dst += 16;
    }
}

/*
 * backend_vk_queue_2d_pic -- the Phase 4k Draw_Pic
 * intercept body.
 *
 * Wired up to render_backend_t::queue_2d_pic and called
 * from common/draw.c::Draw_Pic before the SW
 * memcpy-into-vid.buffer path runs.  Caches the pic in
 * vk_overlay_slots (linear-scan lookup by qpic_t pointer)
 * and appends an entry to vk_overlay_draws for
 * record_frame to consume at end-of-frame.  Uploads on
 * cache miss are synchronous (begin_uploads /
 * upload_pic_slot / end_uploads -- the helper trio
 * QueueWaitIdles); a level load's worth of new pics will
 * cost one slow frame, then steady state hits the cache.
 *
 * Silently no-ops if resources aren't ready (the
 * intercept can fire before create_resources finishes if
 * a draw happens during init), if pic is NULL, or if the
 * cache / draw list is full (no eviction yet -- Phase 4l
 * adds invalidation hooks if it turns out to be needed).
 *
 * Coordinate conversion: Quake screen space goes
 * (0..vid.width, 0..vid.height) with y = 0 at the top of
 * the framebuffer.  Vulkan NDC goes (-1..+1) with y = -1
 * at the top of the framebuffer (because the compute
 * output samples vid.buffer with v = 0 at top and writes
 * to render-target y = 0 at top, then the swapchain
 * presents y = 0 as the top of the window).  So the
 * conversion is a plain rescale:
 *
 *   ndc = 2 * screen / extent - 1
 *
 * with no flips on either axis.
 */
static void
backend_vk_queue_2d_pic(int x, int y, const qpic_t *pic)
{
    unsigned             slot_idx;
    unsigned             si;
    struct overlay_draw *draw;
    int                  pw, ph;

    if (!vk_resources_ready || !pic)
        return;

    pw = pic->width;
    ph = pic->height;
    if (pw <= 0 || ph <= 0)
        return;

    /* Cache lookup: linear scan over populated slots.
     * The qpic_t pointer is the key.  Slots with
     * key == NULL are empty.  OVERLAY_SLOT_MAX is small
     * enough (128) that the scan is microseconds even
     * called many times per frame.
     *
     * Phase 5b-06 follow-up: also require width/height
     * to match.  After r_polysubdiv bumps, alias models
     * grow up to ~10 MB each in the Hunk cache, which
     * triggers Cache_FreeLow / Cache_Move on the menu
     * gfx LMPs sharing the cache.  When Draw_CachePic
     * reloads an evicted pic the Hunk allocator can
     * hand back the *same* address a previously evicted
     * (different) pic occupied, so vk_overlay_slots[].key
     * collides.  The stale slot was created at the
     * other pic's dimensions; reusing it draws the old
     * pixels stretched across the new pic's quad --
     * Lib screenshot 5b-06: qplaque area replaced by
     * vertical stretched-stripe garbage when subdiv
     * stepped 2 -> 3.  Requiring dims to match falls
     * through to "allocate new slot" on a collision.
     * The stale slot stays occupied (we don't destroy
     * its GPU resources inline; safer to let
     * destroy_resources clean it up at backend
     * teardown) but won't be matched again. */
    slot_idx = OVERLAY_SLOT_MAX;
    for (si = 0; si < OVERLAY_SLOT_MAX; si++) {
        if (vk_overlay_slots[si].key == (const void *)pic
            && vk_overlay_slots[si].width  == (unsigned)pw
            && vk_overlay_slots[si].height == (unsigned)ph) {
            slot_idx = si;
            break;
        }
    }

    if (slot_idx == OVERLAY_SLOT_MAX) {
        /* Cache miss: find the first empty slot and
         * upload pic->data there.  Empty == key NULL
         * AND image VK_NULL_HANDLE (defensive double-
         * check: a freshly-created slot has both, a
         * torn-down slot has both, and we never zero
         * one without the other). */
        for (si = 0; si < OVERLAY_SLOT_MAX; si++) {
            if (vk_overlay_slots[si].key == NULL &&
                vk_overlay_slots[si].image == VK_NULL_HANDLE) {
                slot_idx = si;
                break;
            }
        }
        if (slot_idx == OVERLAY_SLOT_MAX) {
            /* Cache full.  Drop the draw silently --
             * Phase 4k has no eviction policy; if Lib
             * hits this in practice, Phase 4l adds one
             * (LRU is the obvious choice).  The SW
             * path still runs in Draw_Pic, so the user
             * sees the pic via the compute upload --
             * just not the crisp Vulkan-overlay copy. */
            return;
        }

        if (!backend_vk_begin_uploads())
            return;
        if (!backend_vk_upload_pic_slot(slot_idx,
                                        (unsigned)pw, (unsigned)ph,
                                        pic->data)) {
            /* upload_pic_slot may have partially
             * populated the slot; clear the key so
             * the next lookup treats it as empty
             * (the partially-created image / memory
             * / view get torn down at
             * destroy_resources time via the slot
             * loop, gated on each handle being
             * non-null). */
            vk_overlay_slots[slot_idx].key = NULL;
            return;
        }
        if (!backend_vk_end_uploads()) {
            vk_overlay_slots[slot_idx].key = NULL;
            return;
        }
        vk_overlay_slots[slot_idx].key = (const void *)pic;
    }

    /* Append a draw entry.  Drop silently if the draw
     * list is full -- Phase 4k caps at OVERLAY_DRAW_MAX
     * (256), which is enough for the HUD + a typical
     * menu; busier scenes (multi-line console with
     * dozens of characters) will need a larger cap or
     * Phase 4l's character-glyph batching. */
    /* LRU stamp: both cache hits and fresh uploads converge
     * here with a final slot_idx; touching once just before
     * the draw-queue commit keeps backend_vk_begin_frame's
     * eviction pass honest -- a slot drawn this frame stays
     * pinned for at least LRU_EVICT_GRACE_FRAMES. */
    vk_overlay_slots[slot_idx].last_used_frame = vk_overlay_frame_counter;

    if (vk_overlay_draw_count >= OVERLAY_DRAW_MAX)
        return;

    draw           = &vk_overlay_draws[vk_overlay_draw_count++];
    draw->slot_idx = slot_idx;
    draw->x0       = (float)(2.0  * (double)x        / (double)width  - 1.0);
    draw->y0       = (float)(2.0  * (double)y        / (double)height - 1.0);
    draw->x1       = (float)(2.0  * (double)(x + pw) / (double)width  - 1.0);
    draw->y1       = (float)(2.0  * (double)(y + ph) / (double)height - 1.0);
    /* Pic draws sample the whole slot. */
    draw->u0       = 0.0f;
    draw->v0       = 0.0f;
    draw->u1       = 1.0f;
    draw->v1       = 1.0f;
}

/*
 * backend_vk_queue_2d_pic_scaled -- Phase 4q scale > 1
 * Draw_PicScaled / Draw_TransPicScaled intercept body.
 *
 * Identical cache / upload / queue-append logic as
 * backend_vk_queue_2d_pic above; the only difference is
 * that the on-screen rect uses (pw * scale) x (ph *
 * scale) so the scaled pic fills the right physical
 * pixels.  Sampling stays at full UV (0,0)-(1,1) -- the
 * overlay sampler's GPU-side filtering produces the
 * stretched result the SW `for (sy) for (sx) replicate
 * byte` loop produces in CPU.  (GL_NEAREST sampling
 * matches the SW byte-replicate exactly; if the slot
 * sampler ends up filtering linearly the edges will be
 * softer than the SW stretch, which the user would
 * notice on HUD digit-pic boundaries.  The existing
 * slot creation in backend_vk_upload_pic_slot uses
 * VK_FILTER_NEAREST -- see the sampler creation there
 * -- so we're aligned.)
 *
 * For Draw_TransPicScaled the overlay FS discards on
 * byte 255 (== TRANSPARENT_COLOR), which produces the
 * `if (b != TRANSPARENT_COLOR) ...` skip the SW path
 * performs per-pixel.  Same discard handles
 * Draw_PicScaled correctly too because opaque pics
 * (the gfx lmp set used at scale > 1: sb_sbar,
 * sb_ibar, qplaque, ttl_main, mainmenu, the face
 * pics) don't contain byte 255 in their palette
 * indices.
 */
static void
backend_vk_queue_2d_pic_scaled(int x, int y,
                               const qpic_t *pic, int scale)
{
    unsigned             slot_idx;
    unsigned             si;
    struct overlay_draw *draw;
    int                  pw, ph;
    int                  dw, dh;

    if (!vk_resources_ready || !pic)
        return;
    if (scale < 1)
        scale = 1;

    pw = pic->width;
    ph = pic->height;
    if (pw <= 0 || ph <= 0)
        return;

    dw = pw * scale;
    dh = ph * scale;

    /* Cache lookup keyed by pic pointer + dimensions.
     * Scale doesn't affect the cached pixels, only the
     * on-screen rect, so two calls with different
     * scales for the same pic share the same slot.
     *
     * Phase 5b-06 follow-up: dimensions are part of the
     * key to defeat Hunk-address reuse across cache
     * evictions.  See backend_vk_queue_2d_pic above for
     * the long comment; same fix applies here. */
    slot_idx = OVERLAY_SLOT_MAX;
    for (si = 0; si < OVERLAY_SLOT_MAX; si++) {
        if (vk_overlay_slots[si].key == (const void *)pic
            && vk_overlay_slots[si].width  == (unsigned)pw
            && vk_overlay_slots[si].height == (unsigned)ph) {
            slot_idx = si;
            break;
        }
    }

    if (slot_idx == OVERLAY_SLOT_MAX) {
        for (si = 0; si < OVERLAY_SLOT_MAX; si++) {
            if (vk_overlay_slots[si].key == NULL &&
                vk_overlay_slots[si].image == VK_NULL_HANDLE) {
                slot_idx = si;
                break;
            }
        }
        if (slot_idx == OVERLAY_SLOT_MAX)
            return;

        if (!backend_vk_begin_uploads())
            return;
        if (!backend_vk_upload_pic_slot(slot_idx,
                                        (unsigned)pw, (unsigned)ph,
                                        pic->data)) {
            vk_overlay_slots[slot_idx].key = NULL;
            return;
        }
        if (!backend_vk_end_uploads()) {
            vk_overlay_slots[slot_idx].key = NULL;
            return;
        }
        vk_overlay_slots[slot_idx].key = (const void *)pic;
    }

    /* LRU stamp: both cache hits and fresh uploads converge
     * here with a final slot_idx; touching once just before
     * the draw-queue commit keeps backend_vk_begin_frame's
     * eviction pass honest -- a slot drawn this frame stays
     * pinned for at least LRU_EVICT_GRACE_FRAMES. */
    vk_overlay_slots[slot_idx].last_used_frame = vk_overlay_frame_counter;

    if (vk_overlay_draw_count >= OVERLAY_DRAW_MAX)
        return;

    draw           = &vk_overlay_draws[vk_overlay_draw_count++];
    draw->slot_idx = slot_idx;
    draw->x0       = (float)(2.0  * (double)x        / (double)width  - 1.0);
    draw->y0       = (float)(2.0  * (double)y        / (double)height - 1.0);
    draw->x1       = (float)(2.0  * (double)(x + dw) / (double)width  - 1.0);
    draw->y1       = (float)(2.0  * (double)(y + dh) / (double)height - 1.0);
    draw->u0       = 0.0f;
    draw->v0       = 0.0f;
    draw->u1       = 1.0f;
    draw->v1       = 1.0f;
}

/*
 * backend_vk_queue_2d_pic_translate_scaled -- Phase 4s
 * Draw_TransPicTranslateScaled intercept body.
 *
 * Quake's multiplayer Setup menu has the user pick a
 * shirt and pants colour and previews the result by
 * rendering the gfx/menuplyr.lmp player sprite with a
 * 256-byte translation table applied per pixel (the
 * table re-maps the TOP_RANGE and BOTTOM_RANGE palette
 * windows to the user's chosen colours; everything
 * else passes through unchanged).  M_BuildTranslation
 * Table in menu.c builds the table from the current
 * setup_top / setup_bottom values; M_DrawTransPic
 * Translate calls Draw_TransPicTranslateScaled with
 * the table and pic each frame.
 *
 * Because the pic data and the translation are both
 * inputs, the cached slot pixels can't be keyed by
 * qpic_t pointer the way queue_2d_pic does -- two
 * frames in a row with different setup_top values
 * need different pixels in the slot.  This function
 * dedicates one slot to the translated pic via a
 * sentinel key (distinct from any qpic_t pointer and
 * from the conchars &draw_chars sentinel), allocates
 * it on the first call, and refreshes its pixels
 * in place on every subsequent call via
 * backend_vk_refresh_pic_slot.  Re-creating the slot
 * each frame would leak descriptor sets -- the pool
 * doesn't have VK_DESCRIPTOR_POOL_CREATE_FREE_
 * DESCRIPTOR_SET_BIT.
 *
 * Translation is done in CPU: walk pic->data, apply
 * translation[b] per byte, into a stack scratch buffer
 * sized to MAX_TRANSLATE_BYTES (large enough for any
 * plausible single qpic_t; menuplyr.lmp is ~64x80 =
 * 5120 bytes, the cap is much higher).  Byte 255 stays
 * 255 (M_BuildTranslationTable initialises the table
 * with identityTable, then only overwrites TOP_RANGE
 * and BOTTOM_RANGE -- both far from 255), so the
 * overlay FS discard-on-255 path handles transparency
 * the same way it does for Draw_TransPicScaled.
 */
#define MAX_TRANSLATE_BYTES 65536

static void
backend_vk_queue_2d_pic_translate_scaled(int x, int y,
                                         const qpic_t *pic,
                                         const byte *translation,
                                         int scale)
{
    /* Sentinel object: address is unique to this
     * function, used as the slot cache key.  Distinct
     * from any qpic_t pointer and from &draw_chars. */
    static const char    translate_slot_marker;
    const void          *key = &translate_slot_marker;

    unsigned             slot_idx;
    unsigned             si;
    struct overlay_draw *draw;
    int                  pw, ph, dw, dh;
    int                  i, n;
    uint8_t              translated[MAX_TRANSLATE_BYTES];

    if (!vk_resources_ready || !pic || !translation)
        return;
    if (scale < 1)
        scale = 1;

    pw = pic->width;
    ph = pic->height;
    if (pw <= 0 || ph <= 0)
        return;

    n = pw * ph;
    if (n > MAX_TRANSLATE_BYTES) {
        /* Defensive: never expected to fire (menuplyr is
         * tiny).  Caller's SW fallback would have been
         * dropped anyway; if a future caller pushes a
         * huge pic through here, expand the cap. */
        if (log_cb)
            log_cb(RETRO_LOG_WARN,
                   "rhi-vk: queue_2d_pic_translate_scaled: "
                   "pic %dx%d (%d bytes) exceeds scratch cap (%d)\n",
                   pw, ph, n, MAX_TRANSLATE_BYTES);
        return;
    }

    dw = pw * scale;
    dh = ph * scale;

    /* Translate */
    for (i = 0; i < n; i++)
        translated[i] = translation[pic->data[i]];

    /* Slot lookup by sentinel key. */
    slot_idx = OVERLAY_SLOT_MAX;
    for (si = 0; si < OVERLAY_SLOT_MAX; si++) {
        if (vk_overlay_slots[si].key == key) {
            slot_idx = si;
            break;
        }
    }

    if (slot_idx == OVERLAY_SLOT_MAX) {
        /* First call: allocate slot + upload via the
         * standard path. */
        for (si = 0; si < OVERLAY_SLOT_MAX; si++) {
            if (vk_overlay_slots[si].key == NULL &&
                vk_overlay_slots[si].image == VK_NULL_HANDLE) {
                slot_idx = si;
                break;
            }
        }
        if (slot_idx == OVERLAY_SLOT_MAX)
            return;

        if (!backend_vk_begin_uploads())
            return;
        if (!backend_vk_upload_pic_slot(slot_idx,
                                        (unsigned)pw, (unsigned)ph,
                                        translated)) {
            vk_overlay_slots[slot_idx].key = NULL;
            return;
        }
        if (!backend_vk_end_uploads()) {
            vk_overlay_slots[slot_idx].key = NULL;
            return;
        }
        vk_overlay_slots[slot_idx].key = key;
    } else {
        /* Subsequent call: refresh pixels in place.
         * Slot dimensions are guaranteed to match pic
         * dimensions because menuplyr.lmp is the only
         * caller and never changes size between calls. */
        if (!backend_vk_begin_uploads())
            return;
        if (!backend_vk_refresh_pic_slot(slot_idx, translated))
            return;
        if (!backend_vk_end_uploads())
            return;
    }

    /* LRU stamp: both cache hits and fresh uploads converge
     * here with a final slot_idx; touching once just before
     * the draw-queue commit keeps backend_vk_begin_frame's
     * eviction pass honest -- a slot drawn this frame stays
     * pinned for at least LRU_EVICT_GRACE_FRAMES. */
    vk_overlay_slots[slot_idx].last_used_frame = vk_overlay_frame_counter;

    if (vk_overlay_draw_count >= OVERLAY_DRAW_MAX)
        return;

    draw           = &vk_overlay_draws[vk_overlay_draw_count++];
    draw->slot_idx = slot_idx;
    draw->x0       = (float)(2.0  * (double)x        / (double)width  - 1.0);
    draw->y0       = (float)(2.0  * (double)y        / (double)height - 1.0);
    draw->x1       = (float)(2.0  * (double)(x + dw) / (double)width  - 1.0);
    draw->y1       = (float)(2.0  * (double)(y + dh) / (double)height - 1.0);
    draw->u0       = 0.0f;
    draw->v0       = 0.0f;
    draw->u1       = 1.0f;
    draw->v1       = 1.0f;
}

/*
 * backend_vk_queue_2d_fill -- Phase 4t Draw_Fill
 * intercept body.
 *
 * Caches one 1x1 R8 slot per distinct palette colour
 * seen.  Slot's single byte is the palette index `c`;
 * the existing overlay FS samples that index out of
 * the slot, looks it up in the shared palette
 * texture, and writes the resulting RGBA to the
 * swapchain.  That reproduces the SW path's
 * `dest[u] = c` byte-for-byte.
 *
 * Key is the address of color_slot_markers[c]; each
 * of the 256 array bytes has a distinct address so
 * the per-colour cache key is unique without
 * colliding with any qpic_t pointer or other sentinel
 * (e.g. &draw_chars for the conchars atlas).
 *
 * Defensive note: a fill with c == 255 would land in
 * a slot whose single byte is 255, which the overlay
 * FS would discard -- the rect would render fully
 * transparent.  Quake never calls Draw_Fill with c ==
 * 255 (255 is the palette transparency marker the
 * pic-blit paths use, never a fill colour), so this
 * is academic, but worth being aware of.
 */
static void
backend_vk_queue_2d_fill(int x, int y, int w, int h, int c)
{
    /* 256-byte sentinel array: each byte's address
     * uniquely identifies one palette colour. */
    static const char    color_slot_markers[256];
    const void          *key;
    uint8_t              cb;
    unsigned             slot_idx;
    unsigned             si;
    struct overlay_draw *draw;

    if (!vk_resources_ready || w <= 0 || h <= 0)
        return;
    if (c < 0 || c > 255)
        return;

    cb  = (uint8_t)c;
    key = (const void *)&color_slot_markers[cb];

    slot_idx = OVERLAY_SLOT_MAX;
    for (si = 0; si < OVERLAY_SLOT_MAX; si++) {
        if (vk_overlay_slots[si].key == key) {
            slot_idx = si;
            break;
        }
    }

    if (slot_idx == OVERLAY_SLOT_MAX) {
        for (si = 0; si < OVERLAY_SLOT_MAX; si++) {
            if (vk_overlay_slots[si].key == NULL &&
                vk_overlay_slots[si].image == VK_NULL_HANDLE) {
                slot_idx = si;
                break;
            }
        }
        if (slot_idx == OVERLAY_SLOT_MAX)
            return;

        if (!backend_vk_begin_uploads())
            return;
        if (!backend_vk_upload_pic_slot(slot_idx, 1, 1, &cb)) {
            vk_overlay_slots[slot_idx].key = NULL;
            return;
        }
        if (!backend_vk_end_uploads()) {
            vk_overlay_slots[slot_idx].key = NULL;
            return;
        }
        vk_overlay_slots[slot_idx].key = key;
    }

    /* LRU stamp: both cache hits and fresh uploads converge
     * here with a final slot_idx; touching once just before
     * the draw-queue commit keeps backend_vk_begin_frame's
     * eviction pass honest -- a slot drawn this frame stays
     * pinned for at least LRU_EVICT_GRACE_FRAMES. */
    vk_overlay_slots[slot_idx].last_used_frame = vk_overlay_frame_counter;

    if (vk_overlay_draw_count >= OVERLAY_DRAW_MAX)
        return;

    draw           = &vk_overlay_draws[vk_overlay_draw_count++];
    draw->slot_idx = slot_idx;
    draw->x0       = (float)(2.0  * (double)x       / (double)width  - 1.0);
    draw->y0       = (float)(2.0  * (double)y       / (double)height - 1.0);
    draw->x1       = (float)(2.0  * (double)(x + w) / (double)width  - 1.0);
    draw->y1       = (float)(2.0  * (double)(y + h) / (double)height - 1.0);
    draw->u0       = 0.0f;
    draw->v0       = 0.0f;
    draw->u1       = 1.0f;
    draw->v1       = 1.0f;
}

/*
 * backend_vk_queue_2d_fade_screen -- Phase 4t
 * Draw_FadeScreen intercept body.
 *
 * The SW path writes byte 0 (palette index 0 == black)
 * to 3 of every 4 pixels in a 4x2 checkerboard pattern,
 * leaving the 4th pixel (the world / HUD underneath)
 * unchanged.  The preserved pixels are at
 * (x, y) where (x & 3) == ((y & 1) << 1):
 *
 *   y=0 (t=0): x%4 == 0 preserved  ->  X . . . X . . .
 *   y=1 (t=2): x%4 == 2 preserved  ->  . . X . . . X .
 *
 * Implementation: stage a single full-screen mask slot
 * (vid.width x vid.height bytes) where:
 *
 *   mask[y][x] = ((x & 3) == ((y & 1) << 1)) ? 255 : 0
 *
 * 255 -> overlay FS discards -> compute-pass world /
 *        Sbar pic backgrounds underneath show through.
 * 0   -> palette[0] == black -> opaque black pixel.
 *
 * The full-screen mask is 1920*1080 = ~2 MiB at Lib's
 * setup, which fits within vk_staging_size
 * (vid.width * vid.height + VK_PALETTE_BYTES).  Mask
 * generation runs once, on the first Draw_FadeScreen
 * call, and the slot survives for the run lifetime.
 *
 * A REPEAT-sampler 4x2 tile would be 2 MiB smaller but
 * needs a separate sampler -- not worth the complexity
 * for a one-shot upload.
 */
static void
backend_vk_queue_2d_fade_screen(void)
{
    static const char    fade_screen_marker;
    const void          *key = &fade_screen_marker;
    unsigned             slot_idx;
    unsigned             si;
    struct overlay_draw *draw;

    if (!vk_resources_ready || width == 0 || height == 0)
        return;

    slot_idx = OVERLAY_SLOT_MAX;
    for (si = 0; si < OVERLAY_SLOT_MAX; si++) {
        if (vk_overlay_slots[si].key == key) {
            slot_idx = si;
            break;
        }
    }

    if (slot_idx == OVERLAY_SLOT_MAX) {
        uint8_t  *mask;
        unsigned  x, y;
        size_t    n;

        for (si = 0; si < OVERLAY_SLOT_MAX; si++) {
            if (vk_overlay_slots[si].key == NULL &&
                vk_overlay_slots[si].image == VK_NULL_HANDLE) {
                slot_idx = si;
                break;
            }
        }
        if (slot_idx == OVERLAY_SLOT_MAX)
            return;

        n    = (size_t)width * (size_t)height;
        mask = (uint8_t *)malloc(n);
        if (!mask)
            return;

        for (y = 0; y < height; y++) {
            unsigned t = (y & 1u) << 1u;
            uint8_t *row = mask + (size_t)y * (size_t)width;
            for (x = 0; x < width; x++)
                row[x] = ((x & 3u) == t) ? (uint8_t)255 : (uint8_t)0;
        }

        if (!backend_vk_begin_uploads()) {
            free(mask);
            return;
        }
        if (!backend_vk_upload_pic_slot(slot_idx, width, height, mask)) {
            free(mask);
            vk_overlay_slots[slot_idx].key = NULL;
            return;
        }
        if (!backend_vk_end_uploads()) {
            free(mask);
            vk_overlay_slots[slot_idx].key = NULL;
            return;
        }
        free(mask);
        vk_overlay_slots[slot_idx].key = key;
    }

    /* LRU stamp: both cache hits and fresh uploads converge
     * here with a final slot_idx; touching once just before
     * the draw-queue commit keeps backend_vk_begin_frame's
     * eviction pass honest -- a slot drawn this frame stays
     * pinned for at least LRU_EVICT_GRACE_FRAMES. */
    vk_overlay_slots[slot_idx].last_used_frame = vk_overlay_frame_counter;

    if (vk_overlay_draw_count >= OVERLAY_DRAW_MAX)
        return;

    draw           = &vk_overlay_draws[vk_overlay_draw_count++];
    draw->slot_idx = slot_idx;
    draw->x0       = -1.0f;
    draw->y0       = -1.0f;
    draw->x1       =  1.0f;
    draw->y1       =  1.0f;
    draw->u0       = 0.0f;
    draw->v0       = 0.0f;
    draw->u1       = 1.0f;
    draw->v1       = 1.0f;
}

/*
 * backend_vk_queue_2d_char -- Phase 4o Draw_Character /
 * Draw_String intercept body.
 *
 * The conchars atlas is one 128x128 image (16x16 cells of
 * 8x8 chars) that backs every visible character in
 * console / menu / scoreboard / centerstring / finale
 * text.  We cache it in exactly one overlay slot keyed by
 * the address of the global draw_chars pointer (stable
 * for the process lifetime, distinct from any qpic_t
 * pointer so it never collides with pic-slot keys).
 *
 * Transparency handling
 * =====================
 * The conchars atlas uses palette byte 0 as its
 * transparency marker (Draw_Character only writes
 * source[i] when source[i] != 0).  That conflicts with
 * the pic / TransPic intercept, which uses byte 255
 * (TRANSPARENT_COLOR) as transparency and whose
 * overlay_quad.frag discards on idx > 254.5 / 255.
 *
 * Rather than introduce a second pipeline / shader or a
 * push-constant, we remap byte 0 to byte 255 during the
 * one-time upload of draw_chars into the slot.  After
 * remap, the existing discard catches the formerly-zero
 * pixels.  This is safe iff conchars does not contain
 * byte 255 as a legitimate (non-transparent) palette
 * index -- if it did, those pixels would be incorrectly
 * discarded.  We scan during upload and log a warning if
 * we observe any byte-255 in the source data; if Lib
 * ever sees that warning fire in practice, the proper
 * fix is a push-constant transparency key or a second
 * pipeline.  In the standard Quake conchars the count is
 * zero.
 */
static void
backend_vk_queue_2d_char(int x, int y, int num, int scale)
{
    /* Sentinel key: address of the global draw_chars
     * pointer.  Stable for process lifetime; distinct
     * from every qpic_t pointer (it lives in .data /
     * .bss rather than the hunk allocator that hands
     * out qpic_t storage). */
    static const void *const conchars_key =
        (const void *)&draw_chars;

    unsigned             slot_idx;
    unsigned             si;
    struct overlay_draw *draw;
    int                  row, col;
    int                  w, h;

    if (!vk_resources_ready || !draw_chars)
        return;
    if (scale < 1)
        scale = 1;

    num &= 255;

    /* Cache lookup by the conchars sentinel. */
    slot_idx = OVERLAY_SLOT_MAX;
    for (si = 0; si < OVERLAY_SLOT_MAX; si++) {
        if (vk_overlay_slots[si].key == conchars_key) {
            slot_idx = si;
            break;
        }
    }

    if (slot_idx == OVERLAY_SLOT_MAX) {
        /* First call: find a free slot, remap byte 0
         * -> byte 255 into a scratch buffer, upload. */
        uint8_t *remapped;
        size_t   i;
        size_t   b255_count = 0;
        qboolean ok;

        for (si = 0; si < OVERLAY_SLOT_MAX; si++) {
            if (vk_overlay_slots[si].key == NULL &&
                vk_overlay_slots[si].image == VK_NULL_HANDLE) {
                slot_idx = si;
                break;
            }
        }
        if (slot_idx == OVERLAY_SLOT_MAX)
            return;

        remapped = (uint8_t *)malloc(128u * 128u);
        if (!remapped)
            return;
        for (i = 0; i < 128u * 128u; i++) {
            byte b = draw_chars[i];
            if (b == 0)
                remapped[i] = 255;
            else
                remapped[i] = b;
            if (b == 255)
                b255_count++;
        }
        if (b255_count && log_cb)
            log_cb(RETRO_LOG_WARN,
                   "rhi-vk: conchars contains %u byte-255 pixels; "
                   "those will be incorrectly discarded as transparent. "
                   "Expect visual artefacts in console / menu text. "
                   "(If you see this, push-constant transparency is the "
                   "right next step.)\n",
                   (unsigned)b255_count);

        ok = backend_vk_begin_uploads();
        if (ok)
            ok = backend_vk_upload_pic_slot(slot_idx,
                                            128u, 128u, remapped);
        if (ok)
            ok = backend_vk_end_uploads();
        free(remapped);
        if (!ok) {
            /* Partial upload may have created some
             * resources; clear key so the slot is
             * treated as empty next time and the
             * destroy_resources loop tears down what's
             * there. */
            vk_overlay_slots[slot_idx].key = NULL;
            return;
        }
        vk_overlay_slots[slot_idx].key = conchars_key;
    }

    /* LRU stamp: both cache hits and fresh uploads converge
     * here with a final slot_idx; touching once just before
     * the draw-queue commit keeps backend_vk_begin_frame's
     * eviction pass honest -- a slot drawn this frame stays
     * pinned for at least LRU_EVICT_GRACE_FRAMES. */
    vk_overlay_slots[slot_idx].last_used_frame = vk_overlay_frame_counter;

    if (vk_overlay_draw_count >= OVERLAY_DRAW_MAX)
        return;

    w   = 8 * scale;
    h   = 8 * scale;
    row = num >> 4;
    col = num & 15;

    draw           = &vk_overlay_draws[vk_overlay_draw_count++];
    draw->slot_idx = slot_idx;
    draw->x0       = (float)(2.0  * (double)x       / (double)width  - 1.0);
    draw->y0       = (float)(2.0  * (double)y       / (double)height - 1.0);
    draw->x1       = (float)(2.0  * (double)(x + w) / (double)width  - 1.0);
    draw->y1       = (float)(2.0  * (double)(y + h) / (double)height - 1.0);
    /* Sub-UV: pick one 8x8 cell out of the 16x16 atlas. */
    draw->u0       = (float)col         / 16.0f;
    draw->v0       = (float)row         / 16.0f;
    draw->u1       = (float)(col + 1)   / 16.0f;
    draw->v1       = (float)(row + 1)   / 16.0f;
}

/*
 * backend_vk_queue_2d_console_background -- Phase 4r
 * (re-attempt of 67c8f47 / Phase 4p) Draw_Console
 * Background intercept body.
 *
 * The original Phase 4p attempt landed and was reverted
 * (02c3181) because at that time M_DrawPic / M_Draw
 * TransPic at scale > 1 still SW-wrote menu pics into
 * vid.buffer; queuing the conback as an overlay quad
 * then covered the menu pics in vid.buffer instead of
 * sitting underneath them in queue order.  Phase 4q
 * (8a9268d) routes scale > 1 pics through the overlay
 * queue, which fixes the ordering: M_Draw now queues
 * conback first then the menu pics, and the menu pics
 * correctly draw on top of the conback as overlay
 * entries.  The implementation here is otherwise
 * unchanged from 67c8f47.
 *
 * The SW Draw_ConsoleBackground stretches the gfx/conback
 * .lmp pic vertically to fit (vid.width, lines), sampling
 * the bottom `lines / vid.height` fraction of the source.
 * The relevant SW math:
 *
 *   for (y = 0; y < lines; y++)
 *     v = (vid.conheight - lines + y) * conback->height
 *         / vid.conheight;
 *
 * so as y sweeps [0, lines) the source v sweeps
 * [(vid.conheight - lines) * conback->height / vid.conheight,
 *  ~conback->height).  Normalised to [0, 1] UV that's
 *
 *   v0 = (vid.height - lines) / vid.height
 *   v1 = 1.0
 *
 * which is what we set on the overlay quad below.  At
 * lines == vid.height (full-screen console, the
 * con_forcedup case in screen.c::SCR_SetUpToDrawConsole)
 * v0 collapses to 0 and the whole pic is sampled.
 *
 * Caching is keyed by the pic pointer (Draw_CachePic
 * returns a stable hunk pointer; same convention queue_2d
 * _pic uses for HUD / menu pics).  Caller already ran
 * Draw_ConbackString before the intercept, so the cached
 * upload captures the TYR_VERSION text baked into the
 * pic.  TYR_VERSION is constant per run, so the cached
 * version stays accurate across all subsequent frames.
 */
static void
backend_vk_queue_2d_console_background(int lines, const qpic_t *pic)
{
    unsigned             slot_idx;
    unsigned             si;
    struct overlay_draw *draw;
    int                  pw, ph;
    float                v0;

    if (!vk_resources_ready || !pic || lines <= 0)
        return;

    pw = pic->width;
    ph = pic->height;
    if (pw <= 0 || ph <= 0)
        return;

    /* Cache lookup -- same linear scan keyed by pic
     * pointer + dimensions that queue_2d_pic uses (see
     * Phase 5b-06 long comment there for the rationale). */
    slot_idx = OVERLAY_SLOT_MAX;
    for (si = 0; si < OVERLAY_SLOT_MAX; si++) {
        if (vk_overlay_slots[si].key == (const void *)pic
            && vk_overlay_slots[si].width  == (unsigned)pw
            && vk_overlay_slots[si].height == (unsigned)ph) {
            slot_idx = si;
            break;
        }
    }

    if (slot_idx == OVERLAY_SLOT_MAX) {
        for (si = 0; si < OVERLAY_SLOT_MAX; si++) {
            if (vk_overlay_slots[si].key == NULL &&
                vk_overlay_slots[si].image == VK_NULL_HANDLE) {
                slot_idx = si;
                break;
            }
        }
        if (slot_idx == OVERLAY_SLOT_MAX)
            return;

        if (!backend_vk_begin_uploads())
            return;
        if (!backend_vk_upload_pic_slot(slot_idx,
                                        (unsigned)pw, (unsigned)ph,
                                        pic->data)) {
            vk_overlay_slots[slot_idx].key = NULL;
            return;
        }
        if (!backend_vk_end_uploads()) {
            vk_overlay_slots[slot_idx].key = NULL;
            return;
        }
        vk_overlay_slots[slot_idx].key = (const void *)pic;
    }

    /* LRU stamp: both cache hits and fresh uploads converge
     * here with a final slot_idx; touching once just before
     * the draw-queue commit keeps backend_vk_begin_frame's
     * eviction pass honest -- a slot drawn this frame stays
     * pinned for at least LRU_EVICT_GRACE_FRAMES. */
    vk_overlay_slots[slot_idx].last_used_frame = vk_overlay_frame_counter;

    if (vk_overlay_draw_count >= OVERLAY_DRAW_MAX)
        return;

    if (lines > (int)height)
        lines = (int)height;
    v0 = (float)((int)height - lines) / (float)height;

    draw           = &vk_overlay_draws[vk_overlay_draw_count++];
    draw->slot_idx = slot_idx;
    draw->x0       = -1.0f;
    draw->y0       = -1.0f;
    draw->x1       =  1.0f;
    draw->y1       = (float)(2.0 * (double)lines / (double)height - 1.0);
    draw->u0       = 0.0f;
    draw->v0       = v0;
    draw->u1       = 1.0f;
    draw->v1       = 1.0f;
}
#endif /* RHI_HAVE_VULKAN */

/*
 * Vtable entry point for queue_2d_pic.  Lives outside the
 * RHI_HAVE_VULKAN block so the symbol is always defined
 * (the vtable references it unconditionally; the body
 * is a no-op in the SW-only build).
 */
static void
backend_vk_queue_2d_pic_entry(int x, int y, const qpic_t *pic)
{
#ifdef RHI_HAVE_VULKAN
    backend_vk_queue_2d_pic(x, y, pic);
#else
    (void)x; (void)y; (void)pic;
#endif
}

/*
 * Vtable entry point for queue_2d_char.  Same #ifdef
 * pattern as the pic entry above: always defined so the
 * vtable links, body no-ops in SW-only builds.
 */
static void
backend_vk_queue_2d_char_entry(int x, int y, int num, int scale)
{
#ifdef RHI_HAVE_VULKAN
    backend_vk_queue_2d_char(x, y, num, scale);
#else
    (void)x; (void)y; (void)num; (void)scale;
#endif
}

/*
 * Vtable entry point for queue_2d_pic_scaled.  Same
 * #ifdef pattern.
 */
static void
backend_vk_queue_2d_pic_scaled_entry(int x, int y,
                                     const qpic_t *pic, int scale)
{
#ifdef RHI_HAVE_VULKAN
    backend_vk_queue_2d_pic_scaled(x, y, pic, scale);
#else
    (void)x; (void)y; (void)pic; (void)scale;
#endif
}

/*
 * Vtable entry point for queue_2d_console_background.
 * Same #ifdef pattern as the entries above.
 */
static void
backend_vk_queue_2d_console_background_entry(int lines, const qpic_t *pic)
{
#ifdef RHI_HAVE_VULKAN
    backend_vk_queue_2d_console_background(lines, pic);
#else
    (void)lines; (void)pic;
#endif
}

/*
 * Vtable entry point for queue_2d_pic_translate_scaled.
 * Same #ifdef pattern.
 */
static void
backend_vk_queue_2d_pic_translate_scaled_entry(int x, int y,
                                               const qpic_t *pic,
                                               const byte *translation,
                                               int scale)
{
#ifdef RHI_HAVE_VULKAN
    backend_vk_queue_2d_pic_translate_scaled(x, y, pic, translation, scale);
#else
    (void)x; (void)y; (void)pic; (void)translation; (void)scale;
#endif
}

/*
 * Vtable entry point for queue_2d_fill.  Same #ifdef
 * pattern.
 */
static void
backend_vk_queue_2d_fill_entry(int x, int y, int w, int h, int c)
{
#ifdef RHI_HAVE_VULKAN
    backend_vk_queue_2d_fill(x, y, w, h, c);
#else
    (void)x; (void)y; (void)w; (void)h; (void)c;
#endif
}

/*
 * Vtable entry point for queue_2d_fade_screen.  Same
 * #ifdef pattern.
 */
static void
backend_vk_queue_2d_fade_screen_entry(void)
{
#ifdef RHI_HAVE_VULKAN
    backend_vk_queue_2d_fade_screen();
#endif
}

/*
 * Phase 5b-02: stage the active particle list for the
 * frame's GPU dispatch.
 *
 * Vtable entry called by R_DrawParticles when
 * g_rhi_compute_rendering is true and this entry is non-
 * NULL.  Walks the linked list, builds a flat array of
 * struct vk_gpu_particle records in the host-mapped
 * vk_particles_buffer, snapshots the SW raster state into
 * vk_particles_push, and sets vk_pending_particle_count.
 * record_frame later sees the non-zero count and emits
 * the dispatch + barrier sequence.
 *
 * Caps at VK_PARTICLES_MAX records.  Overflow particles
 * (beyond the cap) are silently dropped -- preferable to
 * a runtime allocation or a hard failure mode.  Quake's
 * default MAX_PARTICLES is 2048; the cap of 8192 leaves
 * 4x headroom even in pathological cases.
 *
 * The push-constants snapshot has to happen here (not in
 * record_frame) because record_frame runs in end_frame
 * after retro_run's input handling, possibly after the
 * world / camera have moved on -- the CPU SW raster
 * state at dispatch-stage time is what the shader's
 * arithmetic must match, so we capture it at the moment
 * R_DrawParticles called us.
 *
 * Defined at file scope outside the big #ifdef
 * RHI_HAVE_VULKAN body block (same #ifdef pattern as the
 * queue_2d_*_entry functions above): the vtable
 * references this name unconditionally so it must
 * resolve in the SW-only build too.  The body is a no-op
 * in that build.
 */
static void
backend_vk_dispatch_3d_particles_impl(struct particle_s *head)
{
#ifdef RHI_HAVE_VULKAN
    struct vk_gpu_particle *dst;
    struct particle_s      *p;
    uint32_t                count;

    if (!vk_resources_ready || !vk_particles_ptr[vk_active_slot])
        return;

    dst   = (struct vk_gpu_particle *)vk_particles_ptr[vk_active_slot];
    count = 0;
    for (p = head; p && count < VK_PARTICLES_MAX; p = p->next) {
        dst[count].origin[0] = p->org[0];
        dst[count].origin[1] = p->org[1];
        dst[count].origin[2] = p->org[2];
        /* particle_t::color is a float for legacy reasons --
         * the SW path casts to byte at draw time
         * (pdest[0] = pparticle->color).  Cast here to
         * preserve the same truncation semantics. */
        dst[count].color = (uint32_t)(uint8_t)p->color;
        count++;
    }

    if (count == 0)
        return;

    /* Snapshot push constants.  These globals can change
     * between now (mid-R_RenderView on the CPU) and the
     * command-buffer record in end_frame, so we capture
     * them by value. */
    memset(&vk_particles_push, 0, sizeof(vk_particles_push));
    vk_particles_push.r_origin[0] = r_origin[0];
    vk_particles_push.r_origin[1] = r_origin[1];
    vk_particles_push.r_origin[2] = r_origin[2];
    vk_particles_push.xcenter     = xcenter;
    vk_particles_push.r_pright[0] = r_pright[0];
    vk_particles_push.r_pright[1] = r_pright[1];
    vk_particles_push.r_pright[2] = r_pright[2];
    vk_particles_push.ycenter     = ycenter;
    vk_particles_push.r_pup[0]    = r_pup[0];
    vk_particles_push.r_pup[1]    = r_pup[1];
    vk_particles_push.r_pup[2]    = r_pup[2];
    vk_particles_push.d_y_aspect_shift = d_y_aspect_shift;
    vk_particles_push.r_ppn[0]    = r_ppn[0];
    vk_particles_push.r_ppn[1]    = r_ppn[1];
    vk_particles_push.r_ppn[2]    = r_ppn[2];
    vk_particles_push.d_pix_shift = d_pix_shift;
    vk_particles_push.d_vrectx               = d_vrectx;
    vk_particles_push.d_vrecty               = d_vrecty;
    vk_particles_push.d_vrectright_particle  = d_vrectright_particle;
    vk_particles_push.d_vrectbottom_particle = d_vrectbottom_particle;
    vk_particles_push.d_pix_min              = d_pix_min;
    vk_particles_push.d_pix_max              = d_pix_max;
    vk_particles_push.particle_count         = count;

    vk_pending_particle_count = count;
#else
    (void)head;
#endif
}

/*
 * Phase 5b-03: stage a water-warp dispatch for the
 * frame.
 *
 * Vtable entry called by R_RenderView at the point it
 * would have called D_WarpScreen (replacing the CPU
 * warp).  Snapshots the current warp phase ((cl.time *
 * TURB_SPEED) & 127) and the scr_vrect bounds into
 * vk_warp_push, sets vk_warp_active true.  record_frame
 * sees the flag and dispatches the fused warp+palette
 * compute shader instead of the regular palette compute.
 *
 * No GPU work happens here -- this just records "we want
 * a warp this frame" -- the actual compute dispatch
 * appears later in the per-frame command buffer.
 *
 * Defined outside the big #ifdef RHI_HAVE_VULKAN body
 * block because the vtable references this name
 * unconditionally; body is a no-op in the SW-only build.
 */
static void
backend_vk_dispatch_3d_warp_screen_impl(void)
{
#ifdef RHI_HAVE_VULKAN
    int phase;

    if (!vk_resources_ready)
        return;

    /* TURB_SPEED == 20, TURB_CYCLE == 128.  phase wraps
     * at TURB_CYCLE; turb[i] index ranges 0..127, which
     * combined with the +i offset (also 0..127) keeps
     * the intsintable[] access in [0, 255]. */
    phase = (int)(cl.time * 20.0) & 127;

    vk_warp_push.phase = phase;
    vk_warp_push.scr_x = scr_vrect.x;
    vk_warp_push.scr_y = scr_vrect.y;
    vk_warp_push.scr_w = scr_vrect.width;
    vk_warp_push.scr_h = scr_vrect.height;

    vk_warp_active = true;
#endif
}

/*
 * Phase 5b-05: stage a sprite dispatch for the frame.
 *
 * Vtable entry called by D_DrawSprite (replacing its
 * span generation + D_SpriteDrawSpans).  Takes the
 * already-clipped + already-projected screen-space
 * vertices, the sprite's pixel data + dimensions, and
 * the transparent palette index (255 in stock Quake).
 *
 * Side-effects (host-only -- the GPU compute dispatch is
 * emitted later in record_frame):
 *
 *   - Append nump verts to vk_sprite_vertex_pool (host-
 *     mapped SSBO).
 *   - Append the sprite's pixel bytes to the next free
 *     slot of vk_sprite_texture_pool.
 *   - Compute the screen-space bounding box from the
 *     vertex positions; the compute dispatch dimensions
 *     itself off the bbox to avoid wasting workgroup
 *     invocations on pixels outside the polygon.
 *   - Append the push-constants block to
 *     vk_sprite_calls[vk_sprite_count++].
 *
 * Pool exhaustion is silent: if either the vertex pool
 * or the sprite-count cap is hit, the sprite is dropped.
 * Quake's typical scene never gets close to either cap
 * so this is an acceptable failure mode -- silent drop
 * is preferable to assertion in a libretro core.
 *
 * Texture size cap: VK_SPRITE_TEX_POOL_SLOT_BYTES
 * (16 KiB, fits 128x128 sprites).  Sprites larger than
 * the slot are dropped.  Stock Quake's largest sprites
 * are 64x64; the cap is generous headroom.
 */
static void
backend_vk_dispatch_3d_sprite_impl(const rhi_sprite_vert_t *verts,
                                   int                      nump,
                                   const byte              *texdata,
                                   int                      tex_width,
                                   int                      tex_height,
                                   int                      transparent_idx)
{
#ifdef RHI_HAVE_VULKAN
    struct vk_sprite_vert *dst_verts;
    uint32_t  v_off;
    uint32_t  t_off;
    int       i;
    float     u_min, v_min, u_max, v_max;
    int       bb_x_min, bb_y_min, bb_x_max, bb_y_max;
    size_t    tex_bytes;

    if (!vk_resources_ready
     || !vk_sprite_vertex_ptr[vk_active_slot]
     || !vk_sprite_texture_ptr[vk_active_slot])
        return;

    /* Cap on number of sprites per frame. */
    if (vk_sprite_count >= VK_SPRITE_MAX_PER_FRAME)
        return;

    /* Bounded vertex count.  Frustum clipping of a 4-
     * vertex quad against the 4 view planes produces at
     * most 4 + 4 = 8 vertices in the worst case, which
     * is exactly what VK_SPRITE_MAX_VERTS_PER allows.
     * Degenerate-low (< 3) means a zero-area polygon
     * that R_SetupAndDrawSprite would have rejected;
     * skip defensively. */
    if (nump < 3 || (uint32_t)nump > VK_SPRITE_MAX_VERTS_PER)
        return;

    /* Texture-pool slot check. */
    tex_bytes = (size_t)tex_width * (size_t)tex_height;
    if (tex_bytes > VK_SPRITE_TEX_POOL_SLOT_BYTES)
        return;

    /* Vertex-pool capacity check (defensive; with the
     * per-frame cap above this can't actually overflow). */
    if (vk_sprite_vertex_cursor + (uint32_t)nump
            > VK_SPRITE_VERT_POOL_VERTS)
        return;

    /* Stage vertices.  rhi_sprite_vert_t layout matches
     * emitpoint_t (5 floats: u, v, s, t, zi); we reorder
     * into the shader's std430 SpriteVert struct (u, v,
     * inv_z, s_over_z, t_over_z) and compute s/z, t/z on
     * the fly.  The reorder + multiplication are cheaper
     * than a struct cast plus an extra shader pass to
     * normalise. */
    v_off     = vk_sprite_vertex_cursor;
    dst_verts = (struct vk_sprite_vert *)vk_sprite_vertex_ptr[vk_active_slot];

    u_min = u_max = verts[0].u;
    v_min = v_max = verts[0].v;
    for (i = 0; i < nump; i++) {
        struct vk_sprite_vert *d = &dst_verts[v_off + (uint32_t)i];

        d->u        = verts[i].u;
        d->v        = verts[i].v;
        d->inv_z    = verts[i].zi;
        d->s_over_z = verts[i].s * verts[i].zi;
        d->t_over_z = verts[i].t * verts[i].zi;
        d->_pad0    = 0.0f;
        d->_pad1    = 0.0f;
        d->_pad2    = 0.0f;

        if (verts[i].u < u_min) u_min = verts[i].u;
        if (verts[i].u > u_max) u_max = verts[i].u;
        if (verts[i].v < v_min) v_min = verts[i].v;
        if (verts[i].v > v_max) v_max = verts[i].v;
    }

    /* Inclusive-min, exclusive-max integer bbox.  Clamp
     * to the rendered framebuffer; the shader does its
     * own bbox check against this clamped rect. */
    bb_x_min = (int)u_min;
    bb_y_min = (int)v_min;
    bb_x_max = (int)u_max + 1;
    bb_y_max = (int)v_max + 1;

    if (bb_x_min < 0)            bb_x_min = 0;
    if (bb_y_min < 0)            bb_y_min = 0;
    if (bb_x_max > (int)width)   bb_x_max = (int)width;
    if (bb_y_max > (int)height)  bb_y_max = (int)height;

    if (bb_x_max <= bb_x_min || bb_y_max <= bb_y_min)
        return;  /* Off-screen / degenerate. */

    /* Stage texture into the next pool slot.  Round-
     * robin slot allocation within the frame: slot
     * index == sprite index.  Caller-side texdata pointer
     * caching could share slots across frames for
     * identical sprite frames, but the per-frame cost of
     * a memcpy(<= 16 KiB) is microseconds; not worth
     * complicating the bookkeeping for. */
    t_off = vk_sprite_count * VK_SPRITE_TEX_POOL_SLOT_BYTES;
    memcpy((byte *)vk_sprite_texture_ptr[vk_active_slot] + t_off, texdata, tex_bytes);

    /* Build the push-constant block. */
    {
        struct vk_sprite_pc *pc = &vk_sprite_calls[vk_sprite_count];

        pc->bbox[0]          = bb_x_min;
        pc->bbox[1]          = bb_y_min;
        pc->bbox[2]          = bb_x_max;
        pc->bbox[3]          = bb_y_max;

        pc->dispatch_info[0] = (int32_t)v_off;
        pc->dispatch_info[1] = nump;
        pc->dispatch_info[2] = (int32_t)t_off;
        pc->dispatch_info[3] = tex_width;

        pc->tex_info[0]      = tex_height;
        pc->tex_info[1]      = transparent_idx;
        pc->tex_info[2]      = 0;
        pc->tex_info[3]      = 0;
    }

    vk_sprite_vertex_cursor += (uint32_t)nump;
    vk_sprite_count++;
#else
    (void)verts;
    (void)nump;
    (void)texdata;
    (void)tex_width;
    (void)tex_height;
    (void)transparent_idx;
#endif
}

/*
 * Phase 5b-06: stage an alias-model dispatch for the
 * frame.
 *
 * Vtable entry called by D_PolysetDraw (replacing its
 * D_DrawNonSubdiv / D_DrawSubdiv span generation).
 *
 * Caller passes:
 *   - verts: a finalvert_t-shaped per-vertex array
 *     (rhi_alias_vert_t has the exact same layout, so
 *     the caller passes (const rhi_alias_vert_t *)
 *     r_affinetridesc.pfinalverts).  Only v[0..5] is
 *     read; flags / reserved / n[] are unused on this
 *     side (the SW raster needs them for the Phong path,
 *     not the flat one we GPU-port).
 *   - tris: an mtriangle_t-shaped triangle-index array
 *     (same layout-match story as verts).
 *   - skin / skin_w / skin_h: skin pixel data + dims
 *     (pointer is used as the skin-pool cache key).
 *   - colormap: 64 x 256 LUT pointer.  Almost always
 *     host_colormap unmodified; cached by pointer.
 *
 * The host side stages the data, resolves cache lookups,
 * computes the screen-space bbox from the participating
 * vertices, and appends to vk_alias_calls[].  The actual
 * compute dispatch is emitted later in record_frame.
 *
 * Pool exhaustion is silent: an over-cap call drops
 * (whole entity-or-triangle goes unrendered for the
 * frame).  Skin / colormap pool exhaustion likewise.
 * Quake's typical scenes never get close to any of these
 * caps; the caps are sized to accommodate a few extreme
 * cases (Shambler vs full party) without hitting them.
 */
/* The two slot-allocator helpers below live in the
 * Vulkan-only conditional block: they're called only
 * from backend_vk_dispatch_3d_alias_impl's
 * RHI_HAVE_VULKAN branch.  Wrapping them in the same
 * #ifdef avoids `unused function` warnings in the no-
 * Vulkan build (which compiles this file to provide
 * only the SW-mode-equivalent stubs). */
#ifdef RHI_HAVE_VULKAN
static uint32_t
backend_vk_alias_skin_slot(const void *skin_ptr, int skin_w, int skin_h,
                           size_t skin_bytes)
{
    uint32_t i, slot;

    /* Cache lookup -- linear probe.  16-slot cache;
     * lookup cost negligible vs the dispatch overhead. */
    for (i = 0; i < vk_alias_skin_count; i++) {
        if (vk_alias_skin_cache[i].ptr == skin_ptr)
            return vk_alias_skin_cache[i].offset;
    }

    /* Miss: allocate new slot. */
    if (vk_alias_skin_count >= VK_ALIAS_SKIN_SLOTS)
        return UINT32_MAX;
    if (skin_bytes > VK_ALIAS_SKIN_SLOT_BYTES)
        return UINT32_MAX;

    slot = vk_alias_skin_count * VK_ALIAS_SKIN_SLOT_BYTES;
    memcpy((byte *)vk_alias_skin_ptr[vk_active_slot] + slot, skin_ptr, skin_bytes);

    vk_alias_skin_cache[vk_alias_skin_count].ptr    = skin_ptr;
    vk_alias_skin_cache[vk_alias_skin_count].offset = slot;
    vk_alias_skin_cache[vk_alias_skin_count].width  = (uint32_t)skin_w;
    vk_alias_skin_cache[vk_alias_skin_count].height = (uint32_t)skin_h;
    vk_alias_skin_count++;
    return slot;
}

static uint32_t
backend_vk_alias_cmap_slot(const void *cmap_ptr)
{
    uint32_t i, slot;

    for (i = 0; i < vk_alias_cmap_count; i++) {
        if (vk_alias_cmap_cache[i].ptr == cmap_ptr)
            return vk_alias_cmap_cache[i].offset;
    }

    if (vk_alias_cmap_count >= VK_ALIAS_CMAP_SLOTS)
        return UINT32_MAX;

    slot = vk_alias_cmap_count * VK_ALIAS_CMAP_SLOT_BYTES;
    memcpy((byte *)vk_alias_colormap_ptr[vk_active_slot] + slot, cmap_ptr,
           VK_ALIAS_CMAP_SLOT_BYTES);

    vk_alias_cmap_cache[vk_alias_cmap_count].ptr    = cmap_ptr;
    vk_alias_cmap_cache[vk_alias_cmap_count].offset = slot;
    vk_alias_cmap_count++;
    return slot;
}
#endif /* RHI_HAVE_VULKAN */

static void
backend_vk_dispatch_3d_alias_impl(const rhi_alias_vert_t *verts,
                                  int                      num_verts,
                                  const rhi_alias_tri_t   *tris,
                                  int                      num_tris,
                                  const byte              *skin,
                                  int                      skin_w,
                                  int                      skin_h,
                                  const byte              *colormap)
{
#ifdef RHI_HAVE_VULKAN
    struct vk_alias_vert *dst_v;
    struct vk_alias_tri  *dst_t;
    uint32_t  v_off, t_off, skin_off, cmap_off;
    int       i;
    int       bb_x_min, bb_y_min, bb_x_max, bb_y_max;
    int       all_min_x, all_min_y, all_max_x, all_max_y;
    size_t    skin_bytes;

    if (!vk_resources_ready
     || !vk_alias_vertex_ptr[vk_active_slot]
     || !vk_alias_triangle_ptr[vk_active_slot]
     || !vk_alias_skin_ptr[vk_active_slot]
     || !vk_alias_colormap_ptr[vk_active_slot])
        return;

    if (vk_alias_count >= VK_ALIAS_MAX_PER_FRAME)
        return;
    if (num_verts <= 0 || num_tris <= 0)
        return;
    if (vk_alias_vertex_cursor + (uint32_t)num_verts
            > VK_ALIAS_VERT_POOL_VERTS)
        return;
    if (vk_alias_triangle_cursor + (uint32_t)num_tris
            > VK_ALIAS_TRI_POOL_TRIS)
        return;

    /* Resolve / upload skin. */
    skin_bytes = (size_t)skin_w * (size_t)skin_h;
    skin_off   = backend_vk_alias_skin_slot(skin, skin_w, skin_h, skin_bytes);
    if (skin_off == UINT32_MAX)
        return;

    /* Resolve / upload colormap. */
    cmap_off = backend_vk_alias_cmap_slot(colormap);
    if (cmap_off == UINT32_MAX)
        return;

    /* Stage vertices: copy just the v[0..5] portion of
     * each finalvert_t into the 24-byte packed pool
     * entry.  This is the inverse of finalvert_t's
     * layout: caller passes the whole 44-byte struct, we
     * extract the leading 6 ints. */
    v_off = vk_alias_vertex_cursor;
    dst_v = (struct vk_alias_vert *)vk_alias_vertex_ptr[vk_active_slot] + v_off;
    for (i = 0; i < num_verts; i++) {
        dst_v[i].v[0] = verts[i].v[0];
        dst_v[i].v[1] = verts[i].v[1];
        dst_v[i].v[2] = verts[i].v[2];
        dst_v[i].v[3] = verts[i].v[3];
        dst_v[i].v[4] = verts[i].v[4];
        dst_v[i].v[5] = verts[i].v[5];
    }

    /* Stage triangles: just the vertindex[] portion. */
    t_off = vk_alias_triangle_cursor;
    dst_t = (struct vk_alias_tri *)vk_alias_triangle_ptr[vk_active_slot] + t_off;
    for (i = 0; i < num_tris; i++) {
        dst_t[i].a = tris[i].vertindex[0];
        dst_t[i].b = tris[i].vertindex[1];
        dst_t[i].c = tris[i].vertindex[2];
    }

    /* Compute bbox over the vertices actually referenced
     * by this call's triangles.  An entity's finalvert
     * array can contain off-screen verts that aren't
     * touched by this triangle subset (e.g. the partially-
     * clipped path passes the whole entity's verts but
     * only one triangle's indices); using only the
     * referenced ones gives a tighter dispatch. */
    all_min_x = all_min_y =  0x7fffffff;
    all_max_x = all_max_y = -0x7fffffff;
    for (i = 0; i < num_tris; i++) {
        int idx;
        int j;

        for (j = 0; j < 3; j++) {
            idx = tris[i].vertindex[j];
            if (idx < 0 || idx >= num_verts)
                continue;
            if (verts[idx].v[0] < all_min_x) all_min_x = verts[idx].v[0];
            if (verts[idx].v[0] > all_max_x) all_max_x = verts[idx].v[0];
            if (verts[idx].v[1] < all_min_y) all_min_y = verts[idx].v[1];
            if (verts[idx].v[1] > all_max_y) all_max_y = verts[idx].v[1];
        }
    }

    if (all_min_x > all_max_x || all_min_y > all_max_y)
        return;  /* No valid triangles. */

    bb_x_min = all_min_x;
    bb_y_min = all_min_y;
    bb_x_max = all_max_x + 1;
    bb_y_max = all_max_y + 1;

    if (bb_x_min < 0)            bb_x_min = 0;
    if (bb_y_min < 0)            bb_y_min = 0;
    if (bb_x_max > (int)width)   bb_x_max = (int)width;
    if (bb_y_max > (int)height)  bb_y_max = (int)height;

    if (bb_x_max <= bb_x_min || bb_y_max <= bb_y_min)
        return;

    /* Build push constants. */
    {
        struct vk_alias_pc *pc = &vk_alias_calls[vk_alias_count];

        pc->bbox[0]          = bb_x_min;
        pc->bbox[1]          = bb_y_min;
        pc->bbox[2]          = bb_x_max;
        pc->bbox[3]          = bb_y_max;

        pc->dispatch_info[0] = (int32_t)v_off;
        pc->dispatch_info[1] = (int32_t)t_off;
        pc->dispatch_info[2] = num_tris;
        pc->dispatch_info[3] = (int32_t)skin_off;

        pc->skin_info[0]     = skin_w;
        pc->skin_info[1]     = skin_h;
        pc->skin_info[2]     = (int32_t)cmap_off;
        pc->skin_info[3]     = 0;
    }

    vk_alias_vertex_cursor   += (uint32_t)num_verts;
    vk_alias_triangle_cursor += (uint32_t)num_tris;
    vk_alias_count++;
#else
    (void)verts; (void)num_verts; (void)tris; (void)num_tris;
    (void)skin; (void)skin_w; (void)skin_h; (void)colormap;
#endif
}

static void
backend_vk_shutdown(void)
{
#ifdef RHI_HAVE_VULKAN
    memset(&vk_hwrender, 0, sizeof(vk_hwrender));
#endif
}

static void
backend_vk_begin_frame(void)
{
#ifdef RHI_HAVE_VULKAN
    /* Phase 5b-08 step 1: rotate to the next ring slot.
     * vk_active_slot advances (mod ring depth), and
     * vk_cmd_buffer is reassigned to that slot's CB so
     * record_frame + every one-shot recording path
     * unconditionally targets the active slot's command
     * buffer.
     *
     * Wait+reset on the slot's fence is the per-slot sync:
     * the fence was signaled when the previous use of
     * this slot completed on the GPU.  In THIS commit
     * QueueWaitIdle in end_frame has already drained
     * everything, so the wait is instant on the second
     * frame and beyond.  Step 3 removes the QueueWaitIdle
     * and makes this wait the actual cross-frame sync.
     *
     * The fences are created SIGNALED so the very first
     * begin_frame after create_resources doesn't deadlock
     * waiting on a never-submitted-into slot.
     *
     * Gated on vk_resources_ready -- if the backend's
     * resources aren't up (e.g. very first begin_frame
     * before create_resources has run, which the existing
     * code already supports), skip the rotation entirely.
     * That same code path will also skip end_frame, so the
     * ring stays consistent. */
    if (vk_resources_ready) {
        vk_active_slot = (vk_active_slot + 1) % VK_FRAME_RING_DEPTH;
        /* Cross-frame backpressure wait.  After 3b removed
         * end_frame's QueueWaitIdle, this fence wait is the
         * only host-side sync the pipeline does.  In a
         * properly pipelined N=2 ring with CPU outrunning
         * GPU, this returns instantly because the slot's
         * prior submission completed two frames ago; if GPU
         * outruns CPU, this is where CPU yields.  Logged as
         * "bw" so a regression to the pre-3b 3.71 ms cost is
         * immediately visible. */
        perf_timing_section_begin(PERF_SECTION_BEGIN_FRAME_WAIT);
        vk_fn.WaitForFences(vk_device, 1,
                            &vk_frame_ctx[vk_active_slot].done_fence,
                            VK_TRUE, UINT64_MAX);
        perf_timing_section_end(PERF_SECTION_BEGIN_FRAME_WAIT);
        vk_fn.ResetFences(vk_device, 1,
                          &vk_frame_ctx[vk_active_slot].done_fence);
        vk_cmd_buffer = vk_frame_ctx[vk_active_slot].cmd_buffer;
    }

    /* Phase 3c: nothing to do.  The clear command buffer is
     * pre-recorded once and re-submitted every frame, so
     * no per-frame setup is needed.  Future phases that
     * record fresh commands per frame will reset the
     * command buffer here.
     *
     * Phase 5b-06 follow-up: advance the overlay slot LRU
     * counter and reclaim slots that have aged past the
     * grace window.  The previous frame's command buffer
     * has been submitted (and, for libretro frame pacing
     * with the standard 2-3 in-flight maximum, completed)
     * by the time we re-enter retro_run, so destroying an
     * image / memory / view that hasn't been touched for
     * LRU_EVICT_GRACE_FRAMES is safe without an explicit
     * vkDeviceWaitIdle.  This is the bounded-pool side of
     * the Phase 5b-06 fix; combined with the Cache_Free
     * notify hook below it means a long playthrough with
     * heavy r_polysubdiv-driven cache churn can't deadlock
     * the slot table at 128 stale entries.
     *
     * Eviction is gated on vk_resources_ready so the pass
     * is a no-op before backend_vk_create_resources stands
     * the pool up (the slot array is BSS-zeroed at that
     * point and would self-skip anyway, but the explicit
     * gate keeps the intent obvious). */
    vk_overlay_frame_counter++;
    if (vk_resources_ready && vk_overlay_frame_counter > LRU_EVICT_GRACE_FRAMES) {
        uint64_t cutoff = vk_overlay_frame_counter - LRU_EVICT_GRACE_FRAMES;
        unsigned si;
        for (si = 0; si < OVERLAY_SLOT_MAX; si++) {
            struct overlay_slot *slot = &vk_overlay_slots[si];
            if (slot->image != VK_NULL_HANDLE
                && slot->last_used_frame > 0
                && slot->last_used_frame < cutoff)
                backend_vk_free_overlay_slot(slot);
        }
    }

    /* Phase 5b-07 step 2: advance the brush atlas's LRU clock.
     * Entries last touched on previous frames become eviction-
     * eligible for this frame's allocations.  Safe before
     * resources are ready -- the guard checks the allocator
     * was constructed (gated on vk_brush_atlas != NULL rather
     * than vk_resources_ready, since the atlas is created in
     * the same create_resources path and stays alive until
     * destroy_resources). */
    if (vk_brush_atlas)
        surf_atlas_begin_frame(vk_brush_atlas);
#endif
}

static void
backend_vk_draw_view(const refdef_t *rd)
{
    (void)rd;
#ifdef RHI_HAVE_VULKAN
    /* Phase 5 scaffolding: branch on the user's compute-
     * vs-graphics preference.  Both branches currently
     * fall through to the existing CPU SW rasterizer
     * (R_RenderView fills vid.buffer; the per-frame
     * compute palette-upload path in backend_vk_end_frame
     * turns that into the swapchain image).  Subsequent
     * commits replace each branch with its real
     * implementation:
     *
     *   compute branch  (Phase 5b+):
     *     GPU compute dispatches that port the SW
     *     rasterizer line-for-line (per-span affine
     *     texturing, surface cache, alias edge stepping
     *     -- the d_*.c / r_*.c hot loops translated into
     *     GLSL).  Writes directly into the same
     *     vid.buffer GPU image the compute palette path
     *     reads, so end_frame's downstream stages don't
     *     need to know which branch produced the pixels.
     *
     *   graphics branch (Phase 5a):
     *     Vulkan graphics pipelines for world surfaces,
     *     alias / brush / sprite models, sky, liquids,
     *     particles, shadows.  Renders to a colour +
     *     depth attachment pair sized to the user's
     *     `tyrquake_resolution`; the compute palette
     *     stage either reads from a different source or
     *     becomes a no-op when this branch owns the
     *     output (tbd, depending on how the colour
     *     attachment's format ends up plumbed). */
    if (g_rhi_compute_rendering) {
        /* Phase 5b: compute rasterizer.  Placeholder --
         * still runs the CPU rasterizer until the GLSL
         * port lands. */
        R_RenderView();
    } else {
        /* Phase 5a: graphics pipelines.  Placeholder --
         * still runs the CPU rasterizer until the
         * pipelines land. */
        R_RenderView();
    }
#endif
}

#ifdef RHI_HAVE_VULKAN
/*
 * Per-frame upload (Phase 4f).  Copies the raw 8bpp vid.buffer
 * into the index region of the staging buffer (bytes
 * [0, width*height)) and d_8to24table_shifted into the palette
 * region (bytes [width*height, width*height + VK_PALETTE_BYTES)).
 * No per-pixel arithmetic on the host -- the FS does the
 * index-into-palette lookup at sample time.
 *
 * d_8to24table_shifted (not d_8to24table) is the source for
 * the palette upload: it's the table that VID_SetPalette
 * keeps in sync with d_8to16table, so it tracks damage
 * flashes, bonus flashes, underwater shifts, and quad-damage
 * tinting just like the SW present path does.  d_8to24table
 * is the base palette (set once at startup, never updated),
 * needed by the SW renderer's alias/surface RGB-light math
 * but not by us.
 *
 * Walks the index region row by row to honour vid.rowbytes
 * (currently == width, but the SW renderer is allowed to pad
 * rows; defending against a future change there costs
 * nothing).  HOST_COHERENT memory means no Flush call needed;
 * the next QueueSubmit's CmdCopyBufferToImage commands see
 * the new bytes.
 *
 * The palette is uploaded every frame even though it changes
 * only on damage flashes / underwater tint / level load: at
 * 1 KiB it's invisible against the index data and avoiding
 * change-detection logic keeps record_frame's command stream
 * uniform across frames.
 */
static void
backend_vk_upload_vid_buffer(void)
{
    uint8_t *idx_dst;
    uint8_t *pal_dst;
    uint32_t y;

    if (!vk_staging_ptr[vk_active_slot] || !vid.buffer)
        return;

    idx_dst = (uint8_t *)vk_staging_ptr[vk_active_slot];
    for (y = 0; y < height; y++) {
        const uint8_t *srow = (const uint8_t *)vid.buffer
                            + (size_t)y * (size_t)vid.rowbytes;
        memcpy(idx_dst + (size_t)y * (size_t)width, srow, (size_t)width);
    }

    pal_dst = idx_dst + (size_t)width * (size_t)height;
    memcpy(pal_dst, d_8to24table_shifted, VK_PALETTE_BYTES);
}

/*
 * Phase 5b-02: stage d_pzbuffer into the host-visible
 * vk_particles_zstaging buffer so the per-frame command
 * recording can CopyBufferToImage it into vk_zbuffer.
 *
 * Called from backend_vk_end_frame only when
 * vk_pending_particle_count > 0 (i.e. there's a particle
 * dispatch waiting that needs the Z values to test
 * against).  At end_frame time, R_RenderView has finished
 * and d_pzbuffer holds the world / alias / brush Z
 * values the CPU SW raster wrote -- exactly what the GPU
 * particles need.
 *
 * Phase 5b-04 widened the GPU format to R32_UINT (to make
 * imageAtomicMax legal -- see particles.comp's prologue),
 * so the upload widens each signed-short d_pzbuffer
 * entry into a uint32 here on the host before the GPU
 * CopyBuffer.  Values stored in d_pzbuffer are always
 * non-negative izi (zi << 15) under normal Quake
 * camera positions, so the sign extension is irrelevant;
 * `(uint32_t)(uint16_t)d_pzbuffer[i]` preserves the bit
 * pattern.
 *
 * The widen loop runs over width * height pixels each
 * frame that has particles -- 2M ops at 1080p, ~2 ms on
 * a modern CPU.  Cost is acceptable for the race-free
 * Z-test it enables.
 */
static void
backend_vk_upload_zbuffer(void)
{
    uint32_t       *dst;
    const int16_t  *src;
    size_t          i;
    size_t          count;

    if (!vk_particles_zstaging_ptr[vk_active_slot] || !d_pzbuffer)
        return;

    dst   = (uint32_t *)vk_particles_zstaging_ptr[vk_active_slot];
    src   = (const int16_t *)d_pzbuffer;
    count = (size_t)width * (size_t)height;
    for (i = 0; i < count; i++)
        dst[i] = (uint32_t)(uint16_t)src[i];
}

/*
 * Record the per-frame command buffer.  Called by end_frame
 * after the host-side staging upload.  Sequence (Phase 4g):
 *
 *   Begin (ONE_TIME_SUBMIT)
 *     Barrier UNDEFINED -> TRANSFER_DST_OPTIMAL on
 *       vk_texture + vk_palette_texture (batched).
 *       UNDEFINED-as-discard handles both the first frame
 *       (texture genuinely UNDEFINED out of create_resources)
 *       and subsequent frames (texture in
 *       SHADER_READ_ONLY_OPTIMAL from the previous frame's
 *       record).  Wrapping the previous content as garbage
 *       is correct: we overwrite the entire texture in the
 *       copy that follows.
 *     CmdCopyBufferToImage staging -> vk_texture.
 *     CmdCopyBufferToImage staging -> vk_palette_texture.
 *     Barrier TRANSFER_DST -> SHADER_READ_ONLY_OPTIMAL on
 *       both textures (batched) with dstStage =
 *       COMPUTE_SHADER (Phase 4f had FRAGMENT_SHADER here;
 *       the consumer is now the compute dispatch, not a
 *       graphics pipeline's FS).
 *     Barrier UNDEFINED -> GENERAL on vk_image with dstStage
 *       = COMPUTE_SHADER so the storage-image write the
 *       compute shader does is ordered after the layout
 *       transition.  Same UNDEFINED-as-discard pattern as
 *       the textures -- previous frame's content is going
 *       to be entirely overwritten.
 *     CmdBindPipeline (COMPUTE) + CmdBindDescriptorSets
 *       (COMPUTE) + CmdDispatch.  Dispatch dimensions are
 *       ceil(width / 16) x ceil(height / 16) x 1; the in-
 *       shader bounds check catches the right/bottom edges
 *       when width or height isn't a multiple of 16.
 *     CmdBeginRenderPass (overlay) + CmdBindPipeline
 *       (GRAPHICS, overlay) + CmdBindVertexBuffers (binding
 *       0 = vk_overlay_vertex_buffer) + CmdDraw(4, 1, 0, 0)
 *       + CmdEndRenderPass.  The render pass's
 *       EXTERNAL -> 0 subpass dependency carries the
 *       compute-write -> color-attachment-access memory
 *       dependency that Phase 4g used to issue as an
 *       explicit pipeline barrier; the attachment's
 *       initialLayout = GENERAL takes the image straight
 *       from the dispatch, finalLayout =
 *       SHADER_READ_ONLY_OPTIMAL hands it to the frontend.
 *       Phase 4h replaces the explicit compute-exit barrier
 *       Phase 4g used to issue.
 *   End
 *
 * The ONE_TIME_SUBMIT flag tells the driver this recorded
 * stream will be submitted exactly once -- legal because
 * the buffer is reset on the next Begin, which is what
 * VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT enables.
 * Drivers can apply optimisations (eliding state caching
 * that a reusable buffer would need to preserve) under
 * this contract.
 *
 * Returns false on any record-time failure so end_frame can
 * skip the submit and let the frontend dupe-frame instead.
 */
static qboolean
backend_vk_record_frame(void)
{
    VkCommandBufferBeginInfo  begin_info;
    VkImageMemoryBarrier      barriers[3];
    VkImageMemoryBarrier      image_barrier;
    VkBufferImageCopy         regions[2];
    VkBufferImageCopy         zb_region;
    VkRenderPassBeginInfo     rpbi;
    VkDeviceSize              vb_offset;
    uint32_t                  group_count_x;
    uint32_t                  group_count_y;
    qboolean                  particles_active;
    qboolean                  sprites_active;
    qboolean                  alias_active;
    qboolean                  turb_active;
    qboolean                  zbuf_active;
    VkResult                  r;

    /* Phase 5b-02 + 5b-05 + 5b-06 + 5b-07b: latch all per-
     * frame compute-3D activity flags.  The implementation
     * reads them in multiple places below; pinning to
     * locals avoids mid-record mutation if a future
     * change adds a path that touches the globals.
     *
     * zbuf_active = particles || sprites || alias || turb --
     * all four subsystems interact with vk_zbuffer (the
     * first three Z-test, single-pass turb also writes Z,
     * pass-2 turb Z-tests), so the d_pzbuffer upload +
     * layout transition + GENERAL transition fire when
     * any is pending.  Missing turb here was the cause of
     * pass-2 turb reading stale Z (slipgate posts /
     * crucified-zombie sprites bleeding through walls in
     * the Quake start map when no monsters or particles
     * were in view to set the other flags). */
    particles_active = vk_pending_particle_count > 0;
    sprites_active   = vk_sprite_count > 0;
    alias_active     = vk_alias_count > 0;
    turb_active      = vk_turb_surface_count > 0;
    zbuf_active      = particles_active || sprites_active
                    || alias_active     || turb_active;

    memset(&begin_info, 0, sizeof(begin_info));
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    r = vk_fn.BeginCommandBuffer(vk_cmd_buffer, &begin_info);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR, "rhi-vk: BeginCommandBuffer failed (%d)\n", (int)r);
        return false;
    }

    /* Phase 5b-07a step 1: sky texture upload.  Conditional
     * on vk_sky_upload_pending: notify_sky_texture set it on
     * the most recent R_InitSky call (level load, rare) and
     * the upload itself is deferred here so it shares the
     * per-frame command buffer.  CPU cache is filled at notify
     * time; we memcpy it into the persistent host-coherent
     * vk_sky_staging buffer, then issue UNDEFINED -> TRANSFER_
     * DST for both images, copy staging -> image x2, then
     * transition both to GENERAL so the (Phase 5b-07a commit
     * 3) sky.comp dispatch can imageLoad from them later in
     * the same command buffer.
     *
     * The two images can share a single CmdPipelineBarrier
     * (same source/dest stage masks, same layout transition);
     * the two CmdCopyBufferToImage calls have to be separate
     * because each targets a different image. */
    if (vk_sky_upload_pending && vk_sky_staging_ptr[vk_active_slot]) {
        VkImageMemoryBarrier sky_barriers[2];
        VkBufferImageCopy    sky_copy;

        /* Stage the cached bytes.  vk_sky_cache_front sits at
         * [0, VK_SKY_LAYER_BYTES); vk_sky_cache_back at
         * [VK_SKY_LAYER_BYTES, 2 * VK_SKY_LAYER_BYTES).
         * HOST_COHERENT memory means no Flush is needed; the
         * next CmdCopyBufferToImage's TRANSFER stage will see
         * the new bytes once submit happens. */
        memcpy((byte *)vk_sky_staging_ptr[vk_active_slot],
               vk_sky_cache_front,
               VK_SKY_LAYER_BYTES);
        memcpy((byte *)vk_sky_staging_ptr[vk_active_slot] + VK_SKY_LAYER_BYTES,
               vk_sky_cache_back,
               VK_SKY_LAYER_BYTES);

        memset(sky_barriers, 0, sizeof(sky_barriers));
        sky_barriers[0].sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        sky_barriers[0].srcAccessMask                   = 0;
        sky_barriers[0].dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
        sky_barriers[0].oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
        sky_barriers[0].newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        sky_barriers[0].srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        sky_barriers[0].dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        sky_barriers[0].image                           = vk_sky_front_image;
        sky_barriers[0].subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        sky_barriers[0].subresourceRange.baseMipLevel   = 0;
        sky_barriers[0].subresourceRange.levelCount     = 1;
        sky_barriers[0].subresourceRange.baseArrayLayer = 0;
        sky_barriers[0].subresourceRange.layerCount     = 1;
        sky_barriers[1]       = sky_barriers[0];
        sky_barriers[1].image = vk_sky_back_image;

        vk_fn.CmdPipelineBarrier(vk_cmd_buffer,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 0, NULL,
                                 0, NULL,
                                 2, sky_barriers);

        memset(&sky_copy, 0, sizeof(sky_copy));
        sky_copy.bufferOffset                    = 0;
        sky_copy.bufferRowLength                 = 0;  /* tightly packed */
        sky_copy.bufferImageHeight               = 0;
        sky_copy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        sky_copy.imageSubresource.mipLevel       = 0;
        sky_copy.imageSubresource.baseArrayLayer = 0;
        sky_copy.imageSubresource.layerCount     = 1;
        sky_copy.imageExtent.width               = VK_SKY_TEXEL_W;
        sky_copy.imageExtent.height              = VK_SKY_TEXEL_H;
        sky_copy.imageExtent.depth               = 1;

        vk_fn.CmdCopyBufferToImage(vk_cmd_buffer,
                                   vk_sky_staging[vk_active_slot],
                                   vk_sky_front_image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   1, &sky_copy);

        sky_copy.bufferOffset = VK_SKY_LAYER_BYTES;
        vk_fn.CmdCopyBufferToImage(vk_cmd_buffer,
                                   vk_sky_staging[vk_active_slot],
                                   vk_sky_back_image,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   1, &sky_copy);

        /* TRANSFER_DST -> GENERAL.  STORAGE images consumed
         * by sky.comp via imageLoad must be GENERAL (UNIFORM_
         * READ would only allow read; GENERAL allows
         * imageLoad which is the path the shader uses). */
        sky_barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        sky_barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        sky_barriers[0].oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        sky_barriers[0].newLayout     = VK_IMAGE_LAYOUT_GENERAL;
        sky_barriers[1].srcAccessMask = sky_barriers[0].srcAccessMask;
        sky_barriers[1].dstAccessMask = sky_barriers[0].dstAccessMask;
        sky_barriers[1].oldLayout     = sky_barriers[0].oldLayout;
        sky_barriers[1].newLayout     = sky_barriers[0].newLayout;

        vk_fn.CmdPipelineBarrier(vk_cmd_buffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 0,
                                 0, NULL,
                                 0, NULL,
                                 2, sky_barriers);

        vk_sky_upload_pending = false;
    }

    /* Textures + (when particles active) zbuffer:
     * UNDEFINED -> TRANSFER_DST_OPTIMAL in a single
     * CmdPipelineBarrier.  UNDEFINED-as-discard is the
     * right old layout for the textures (whole content
     * overwritten by the copies below) and for the
     * zbuffer (whole content overwritten by the zbuffer
     * staging copy). */
    memset(&barriers, 0, sizeof(barriers));
    barriers[0].sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barriers[0].srcAccessMask                   = 0;
    barriers[0].dstAccessMask                   = VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[0].oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
    barriers[0].newLayout                       = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[0].srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barriers[0].image                           = vk_texture;
    barriers[0].subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barriers[0].subresourceRange.baseMipLevel   = 0;
    barriers[0].subresourceRange.levelCount     = 1;
    barriers[0].subresourceRange.baseArrayLayer = 0;
    barriers[0].subresourceRange.layerCount     = 1;
    barriers[1]       = barriers[0];
    barriers[1].image = vk_palette_texture;
    barriers[2]       = barriers[0];
    barriers[2].image = vk_zbuffer;

    vk_fn.CmdPipelineBarrier(vk_cmd_buffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             0, NULL,
                             0, NULL,
                             zbuf_active ? 3 : 2, barriers);

    /* Index texture: bytes [0, width*height) of staging at
     * the start of vk_texture.  Palette texture: bytes
     * [width*height, +VK_PALETTE_BYTES) of staging at the
     * start of vk_palette_texture. */
    memset(&regions, 0, sizeof(regions));
    regions[0].bufferOffset                    = 0;
    regions[0].bufferRowLength                 = 0;  /* tightly packed */
    regions[0].bufferImageHeight               = 0;
    regions[0].imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    regions[0].imageSubresource.mipLevel       = 0;
    regions[0].imageSubresource.baseArrayLayer = 0;
    regions[0].imageSubresource.layerCount     = 1;
    regions[0].imageExtent.width               = width;
    regions[0].imageExtent.height              = height;
    regions[0].imageExtent.depth               = 1;

    vk_fn.CmdCopyBufferToImage(vk_cmd_buffer,
                               vk_staging_buffer[vk_active_slot],
                               vk_texture,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &regions[0]);

    regions[1].bufferOffset                    = (VkDeviceSize)width
                                               * (VkDeviceSize)height;
    regions[1].bufferRowLength                 = 0;
    regions[1].bufferImageHeight               = 0;
    regions[1].imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    regions[1].imageSubresource.mipLevel       = 0;
    regions[1].imageSubresource.baseArrayLayer = 0;
    regions[1].imageSubresource.layerCount     = 1;
    regions[1].imageExtent.width               = VK_PALETTE_TEXELS;
    regions[1].imageExtent.height              = 1;
    regions[1].imageExtent.depth               = 1;

    vk_fn.CmdCopyBufferToImage(vk_cmd_buffer,
                               vk_staging_buffer[vk_active_slot],
                               vk_palette_texture,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               1, &regions[1]);

    /* Phase 5b-02: when particles are active, also copy
     * d_pzbuffer (staged earlier in end_frame) into the
     * GPU vk_zbuffer.  Same TRANSFER_DST_OPTIMAL layout
     * the barrier above prepared. */
    /* Phase 5b-02: when particles are active, also copy
     * d_pzbuffer (staged earlier in end_frame) into the
     * GPU vk_zbuffer.  Same TRANSFER_DST_OPTIMAL layout
     * the barrier above prepared.  Phase 5b-05 widens
     * the gate to zbuf_active so sprites trigger the
     * upload too (the d_pzbuffer staging is shared
     * between the two subsystems). */
    if (zbuf_active) {
        memset(&zb_region, 0, sizeof(zb_region));
        zb_region.bufferOffset                    = 0;
        zb_region.bufferRowLength                 = 0;
        zb_region.bufferImageHeight               = 0;
        zb_region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        zb_region.imageSubresource.mipLevel       = 0;
        zb_region.imageSubresource.baseArrayLayer = 0;
        zb_region.imageSubresource.layerCount     = 1;
        zb_region.imageExtent.width               = width;
        zb_region.imageExtent.height              = height;
        zb_region.imageExtent.depth               = 1;

        vk_fn.CmdCopyBufferToImage(vk_cmd_buffer,
                                   vk_particles_zstaging[vk_active_slot],
                                   vk_zbuffer,
                                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                   1, &zb_region);
    }

    if (zbuf_active) {
        /* Phase 5b-02 + 5b-05 compute-3D dispatch path.
         *
         * Post-copy layout transitions:
         *   vk_texture        : TRANSFER_DST -> GENERAL
         *                       (compute shaders will
         *                       imageStore into it)
         *   vk_palette_texture: TRANSFER_DST -> SHADER_
         *                       READ_ONLY (read by the
         *                       downstream palette
         *                       compute dispatch
         *                       unchanged)
         *   vk_zbuffer        : TRANSFER_DST -> GENERAL
         *                       (compute shaders will
         *                       imageLoad + imageStore /
         *                       imageAtomicMax for the
         *                       Z-test)
         *
         * All three batched in one CmdPipelineBarrier
         * with dstStage = COMPUTE_SHADER (the next
         * consumer for all three is the particle and / or
         * sprite dispatches). */
        barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT
                                  | VK_ACCESS_SHADER_WRITE_BIT;
        barriers[0].oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barriers[0].newLayout     = VK_IMAGE_LAYOUT_GENERAL;
        barriers[0].image         = vk_texture;

        barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barriers[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barriers[1].oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barriers[1].newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barriers[1].image         = vk_palette_texture;

        barriers[2].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barriers[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT
                                  | VK_ACCESS_SHADER_WRITE_BIT;
        barriers[2].oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barriers[2].newLayout     = VK_IMAGE_LAYOUT_GENERAL;
        barriers[2].image         = vk_zbuffer;

        vk_fn.CmdPipelineBarrier(vk_cmd_buffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 0,
                                 0, NULL,
                                 0, NULL,
                                 3, barriers);

        /* Phase 5b-07a step 3: sky compute dispatch.
         *
         * Runs BEFORE the alias / particle / sprite dispatches
         * so when those later passes Z-test against vk_zbuffer
         * (which holds sky's far Z at sky pixels, deposited by
         * the SW D_DrawZSpans path that still runs in d_edge.c
         * for SURF_DRAWSKY) and atomicMax wins (alias is
         * closer), their imageStore correctly overwrites sky's
         * color at those pixels.
         *
         * A SHADER_WRITE -> SHADER_WRITE memory barrier
         * follows the dispatch to serialise vk_texture writes
         * against the subsequent compute passes -- without it
         * the GPU is free to schedule sky and (e.g.) alias
         * concurrently, racing on the shared output image. */
        if (vk_sky_collected_count > 0 && vk_sky_ubo_ptr[vk_active_slot]) {
            struct vk_sky_ubo *ubo  = (struct vk_sky_ubo *)vk_sky_ubo_ptr[vk_active_slot];
            uint32_t          *rows = (uint32_t *)vk_sky_rows_ptr[vk_active_slot];
            uint32_t          *sp   = (uint32_t *)vk_sky_spans_ptr[vk_active_slot];
            VkMemoryBarrier    sky_mem_bar;
            unsigned           si;
            int                v;
            uint32_t           running;
            uint32_t           bbox_w, bbox_h;

            /* Bucket spans by scanline.  Two passes.  Pass 1
             * counts per row (storing in the .y slot of each
             * (first, count) pair).  Pass 2 prefix-sums the
             * counts into .x and resets .y for placement.
             * Pass 3 places each span at rows[v].x + rows[v].y
             * and increments .y so it doubles as a write
             * cursor; .y ends up holding the final count, as
             * the shader expects. */
            for (v = vk_sky_bbox_min_y; v < vk_sky_bbox_max_y; v++) {
                rows[2 * v]     = 0;
                rows[2 * v + 1] = 0;
            }
            for (si = 0; si < vk_sky_collected_count; si++) {
                int row = vk_sky_collected[si].v;
                rows[2 * row + 1]++;
            }
            running = 0;
            for (v = vk_sky_bbox_min_y; v < vk_sky_bbox_max_y; v++) {
                uint32_t cnt = rows[2 * v + 1];
                rows[2 * v]     = running;
                rows[2 * v + 1] = 0;
                running += cnt;
            }
            for (si = 0; si < vk_sky_collected_count; si++) {
                int      row    = vk_sky_collected[si].v;
                uint32_t pos    = rows[2 * row] + rows[2 * row + 1];
                sp[2 * pos]     = vk_sky_collected[si].u;
                sp[2 * pos + 1] = vk_sky_collected[si].count;
                rows[2 * row + 1]++;
            }

            /* Fill the UBO.  std140 layout: vec3 + pad fp32,
             * 3 times; then 8 fp32 fields; then 6 u32 fields.
             * struct vk_sky_ubo's C layout matches. */
            memset(ubo, 0, sizeof(*ubo));
            ubo->vpn[0]      = vpn[0];
            ubo->vpn[1]      = vpn[1];
            ubo->vpn[2]      = vpn[2];
            ubo->vright[0]   = vright[0];
            ubo->vright[1]   = vright[1];
            ubo->vright[2]   = vright[2];
            ubo->vup[0]      = vup[0];
            ubo->vup[1]      = vup[1];
            ubo->vup[2]      = vup[2];
            ubo->xcenter    = xcenter;
            ubo->ycenter    = ycenter;
            ubo->xscale     = xscale;
            ubo->yscale     = yscale;
            ubo->timespeed1 = skytime * skyspeed;
            ubo->timespeed2 = ubo->timespeed1 * 2.0f;
            bbox_w = (uint32_t)(vk_sky_bbox_max_x - vk_sky_bbox_min_x);
            bbox_h = (uint32_t)(vk_sky_bbox_max_y - vk_sky_bbox_min_y);
            ubo->bbox_min_x = (uint32_t)vk_sky_bbox_min_x;
            ubo->bbox_min_y = (uint32_t)vk_sky_bbox_min_y;
            ubo->bbox_w     = bbox_w;
            ubo->bbox_h     = bbox_h;

            /* Dispatch one workgroup per 8x8 tile of the
             * bbox.  Per-row span list lookup inside the
             * shader filters pixels not claimed by any sky
             * span; the heavy per-pixel sphere-projection
             * runs only at hits. */
            vk_fn.CmdBindPipeline(vk_cmd_buffer,
                                  VK_PIPELINE_BIND_POINT_COMPUTE,
                                  vk_sky_pipeline);
            vk_fn.CmdBindDescriptorSets(vk_cmd_buffer,
                                        VK_PIPELINE_BIND_POINT_COMPUTE,
                                        vk_sky_pipeline_layout,
                                        0,
                                        1, &vk_sky_set[vk_active_slot],
                                        0, NULL);
            vk_fn.CmdDispatch(vk_cmd_buffer,
                              (bbox_w + 7u) / 8u,
                              (bbox_h + 7u) / 8u,
                              1);

            /* Serialise sky's vk_texture writes against the
             * downstream alias / particle / sprite dispatches.
             * Without this barrier the GPU is free to
             * schedule those passes concurrently with sky;
             * since they all write vk_texture (sky
             * unconditionally, the others atomicMax-gated)
             * and the closer-geometry write must land AFTER
             * sky's at any overlapping pixel, the
             * SHADER_WRITE -> SHADER_WRITE barrier is
             * required for correctness. */
            memset(&sky_mem_bar, 0, sizeof(sky_mem_bar));
            sky_mem_bar.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            sky_mem_bar.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            sky_mem_bar.dstAccessMask = VK_ACCESS_SHADER_READ_BIT
                                      | VK_ACCESS_SHADER_WRITE_BIT;

            vk_fn.CmdPipelineBarrier(vk_cmd_buffer,
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     0,
                                     1, &sky_mem_bar,
                                     0, NULL,
                                     0, NULL);
        }

        /* Phase 5b-07b + followup: turb compute dispatches.
         * Two dispatch positions in record_frame, picked by
         * the per-frame is_pass2 flag carried on each surface
         * entry (uniform across all surfaces in one frame
         * because r_renderpass is per-frame):
         *
         *   - is_pass2 == 0 (single-pass mode):  dispatched
         *     HERE (after sky, before particles / sprites /
         *     alias).  Writes color + per-pixel 1/z; closer-
         *     geometry atomicMax-Z races downstream see
         *     turb's Z and correctly win where they're in
         *     front of water.
         *
         *   - is_pass2 == 1 (two-pass mode pass 2):  atlas
         *     upload still happens HERE so the GENERAL
         *     layout is ready by the time the after-alias
         *     dispatch slot fires; the actual CmdDispatch
         *     loop is deferred to a second `if` block past
         *     the alias dispatch.  Pass-2 turb writes color
         *     only (no Z) and z-tests against vk_zbuffer
         *     (= pass-1 opaque-world Z plus alias / particle
         *     / sprite atomicMax updates from the preceding
         *     dispatches).
         *
         * Atlas image upload (one CmdCopyBufferToImage cover-
         * ing [0, vk_turb_tex_count) slots) runs once per
         * frame regardless of mode -- the cache resets every
         * frame so all in-use slots are fresh and need re-
         * upload, but the staging buffer is permanently
         * mapped so the actual byte-copy already happened on
         * the CPU side during dispatch_3d_turb_surface. */
        if (vk_turb_surface_count > 0) {
            VkImageMemoryBarrier ta_bar0, ta_bar1;
            VkBufferImageCopy    ta_copy;
            VkMemoryBarrier      tu_mem_bar;
            unsigned             si;
            int                  turb_is_pass2;

            turb_is_pass2 = (vk_turb_surfaces[0].is_pass2 != 0);

            /* Atlas image: UNDEFINED (first frame) or GENERAL
             * (subsequent frames) -> TRANSFER_DST.  Use
             * UNDEFINED as oldLayout: Vulkan permits this
             * "discard previous contents" transition from any
             * actual current layout, and that's exactly what
             * we want -- the staging buffer carries fresh data
             * for every slot in [0, vk_turb_tex_count). */
            memset(&ta_bar0, 0, sizeof(ta_bar0));
            ta_bar0.sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            ta_bar0.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            ta_bar0.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            ta_bar0.oldLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
            ta_bar0.newLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            ta_bar0.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            ta_bar0.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            ta_bar0.image         = vk_turb_atlas_image;
            ta_bar0.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            ta_bar0.subresourceRange.levelCount = 1;
            ta_bar0.subresourceRange.layerCount = 1;
            vk_fn.CmdPipelineBarrier(vk_cmd_buffer,
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     0,
                                     0, NULL,
                                     0, NULL,
                                     1, &ta_bar0);

            memset(&ta_copy, 0, sizeof(ta_copy));
            ta_copy.bufferOffset      = 0;
            ta_copy.bufferRowLength   = 0;  /* tightly packed */
            ta_copy.bufferImageHeight = 0;
            ta_copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            ta_copy.imageSubresource.layerCount = 1;
            ta_copy.imageOffset.x = 0;
            ta_copy.imageOffset.y = 0;
            ta_copy.imageOffset.z = 0;
            ta_copy.imageExtent.width  = VK_TURB_TEX_SIZE;
            ta_copy.imageExtent.height = VK_TURB_TEX_SIZE * vk_turb_tex_count;
            ta_copy.imageExtent.depth  = 1;
            vk_fn.CmdCopyBufferToImage(vk_cmd_buffer,
                                       vk_turb_atlas_staging[vk_active_slot],
                                       vk_turb_atlas_image,
                                       VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                       1, &ta_copy);

            memset(&ta_bar1, 0, sizeof(ta_bar1));
            ta_bar1.sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            ta_bar1.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            ta_bar1.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            ta_bar1.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            ta_bar1.newLayout     = VK_IMAGE_LAYOUT_GENERAL;
            ta_bar1.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            ta_bar1.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            ta_bar1.image         = vk_turb_atlas_image;
            ta_bar1.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            ta_bar1.subresourceRange.levelCount = 1;
            ta_bar1.subresourceRange.layerCount = 1;
            vk_fn.CmdPipelineBarrier(vk_cmd_buffer,
                                     VK_PIPELINE_STAGE_TRANSFER_BIT,
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     0,
                                     0, NULL,
                                     0, NULL,
                                     1, &ta_bar1);

            if (!turb_is_pass2) {
                /* Single-pass mode: dispatch the whole batch
                 * here, between sky and particles / sprites /
                 * alias.  Each dispatch writes color + Z. */
                vk_fn.CmdBindPipeline(vk_cmd_buffer,
                                      VK_PIPELINE_BIND_POINT_COMPUTE,
                                      vk_turb_pipeline);
                vk_fn.CmdBindDescriptorSets(vk_cmd_buffer,
                                            VK_PIPELINE_BIND_POINT_COMPUTE,
                                            vk_turb_pipeline_layout,
                                            0,
                                            1, &vk_turb_set[vk_active_slot],
                                            0, NULL);
                for (si = 0; si < vk_turb_surface_count; si++) {
                    const struct vk_turb_surface *surf = &vk_turb_surfaces[si];
                    struct {
                        uint32_t bbox[4];
                        uint32_t range[4];
                        float    grad0[4];
                        float    grad1[4];
                        float    grad2[4];
                        int32_t  adj[4];
                        uint32_t mode[4];
                    } push;
                    push.bbox[0]  = surf->bbox_min_x;
                    push.bbox[1]  = surf->bbox_min_y;
                    push.bbox[2]  = surf->bbox_w;
                    push.bbox[3]  = surf->bbox_h;
                    push.range[0] = surf->rows_first;
                    push.range[1] = (uint32_t)surf->phase;
                    push.range[2] = surf->spans_first;
                    push.range[3] = surf->tex_slot;
                    push.grad0[0] = surf->grad.sdivzorigin;
                    push.grad0[1] = surf->grad.sdivzstepu;
                    push.grad0[2] = surf->grad.sdivzstepv;
                    push.grad0[3] = 0.0f;
                    push.grad1[0] = surf->grad.tdivzorigin;
                    push.grad1[1] = surf->grad.tdivzstepu;
                    push.grad1[2] = surf->grad.tdivzstepv;
                    push.grad1[3] = 0.0f;
                    push.grad2[0] = surf->grad.ziorigin;
                    push.grad2[1] = surf->grad.zistepu;
                    push.grad2[2] = surf->grad.zistepv;
                    push.grad2[3] = 0.0f;
                    push.adj[0]   = surf->grad.sadjust;
                    push.adj[1]   = surf->grad.tadjust;
                    push.adj[2]   = surf->grad.bbextents;
                    push.adj[3]   = surf->grad.bbextentt;
                    push.mode[0]  = surf->alpha;
                    push.mode[1]  = surf->is_pass2;
                    push.mode[2]  = 0;
                    push.mode[3]  = 0;
                    vk_fn.CmdPushConstants(vk_cmd_buffer,
                                           vk_turb_pipeline_layout,
                                           VK_SHADER_STAGE_COMPUTE_BIT,
                                           0, sizeof(push), &push);
                    vk_fn.CmdDispatch(vk_cmd_buffer,
                                      (surf->bbox_w + 7u) / 8u,
                                      (surf->bbox_h + 7u) / 8u,
                                      1);
                }

                /* Serialise turb's vk_texture / vk_zbuffer
                 * writes against the downstream alias /
                 * particle / sprite dispatches, same pattern
                 * as sky's barrier above. */
                memset(&tu_mem_bar, 0, sizeof(tu_mem_bar));
                tu_mem_bar.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
                tu_mem_bar.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                tu_mem_bar.dstAccessMask = VK_ACCESS_SHADER_READ_BIT
                                         | VK_ACCESS_SHADER_WRITE_BIT;
                vk_fn.CmdPipelineBarrier(vk_cmd_buffer,
                                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                         0,
                                         1, &tu_mem_bar,
                                         0, NULL,
                                         0, NULL);
            }
            /* Pass-2 case: atlas is in GENERAL layout; the
             * second `if` block past the alias dispatch
             * does the CmdDispatch loop. */
        }

        if (particles_active) {
            /* Particles dispatch (Phase 5b-02).  One
             * workgroup invocation per particle; local_
             * size_x = 64 (declared in particles.comp),
             * so dispatch ceil(count/64) workgroups.  The
             * in-shader pid >= count early-out handles
             * the tail. */
            vk_fn.CmdBindPipeline(vk_cmd_buffer,
                                  VK_PIPELINE_BIND_POINT_COMPUTE,
                                  vk_particles_pipeline);
            vk_fn.CmdBindDescriptorSets(vk_cmd_buffer,
                                        VK_PIPELINE_BIND_POINT_COMPUTE,
                                        vk_particles_pipeline_layout,
                                        0,
                                        1, &vk_particles_set[vk_active_slot],
                                        0, NULL);
            vk_fn.CmdPushConstants(vk_cmd_buffer,
                                   vk_particles_pipeline_layout,
                                   VK_SHADER_STAGE_COMPUTE_BIT,
                                   0, sizeof(vk_particles_push),
                                   &vk_particles_push);
            vk_fn.CmdDispatch(vk_cmd_buffer,
                              (vk_pending_particle_count + 63u) / 64u,
                              1, 1);
        }

        if (sprites_active) {
            /* Sprite dispatches (Phase 5b-05).  Bind the
             * pipeline + set once, then per-sprite: push
             * its constants and dispatch with workgroup
             * dimensions derived from the bbox.  No
             * barrier between sprites -- they operate on
             * possibly-overlapping bbox rects of
             * vk_texture + vk_zbuffer, and the
             * imageAtomicMax Z-test correctly handles
             * inter-dispatch overlap the same way it does
             * intra-dispatch overlap (with the residual
             * colour race documented in particles.comp's
             * prologue, applicable here too).
             *
             * Particles before sprites is arbitrary --
             * the atomicMax-based Z-test means dispatch
             * order doesn't change visual outcome when
             * both touch the same pixel; the closer Z
             * wins regardless. */
            uint32_t si;

            vk_fn.CmdBindPipeline(vk_cmd_buffer,
                                  VK_PIPELINE_BIND_POINT_COMPUTE,
                                  vk_sprite_pipeline);
            vk_fn.CmdBindDescriptorSets(vk_cmd_buffer,
                                        VK_PIPELINE_BIND_POINT_COMPUTE,
                                        vk_sprite_pipeline_layout,
                                        0,
                                        1, &vk_sprite_set[vk_active_slot],
                                        0, NULL);

            for (si = 0; si < vk_sprite_count; si++) {
                const struct vk_sprite_pc *spc = &vk_sprite_calls[si];
                uint32_t bb_w, bb_h;

                vk_fn.CmdPushConstants(vk_cmd_buffer,
                                       vk_sprite_pipeline_layout,
                                       VK_SHADER_STAGE_COMPUTE_BIT,
                                       0, sizeof(*spc),
                                       spc);

                /* Workgroup size 8x8 (declared in sprite.
                 * comp); dispatch ceil(bbox_w/8) x
                 * ceil(bbox_h/8) workgroups.  In-shader
                 * bbox check culls the tail and any
                 * pixels outside the polygon. */
                bb_w = (uint32_t)(spc->bbox[2] - spc->bbox[0]);
                bb_h = (uint32_t)(spc->bbox[3] - spc->bbox[1]);

                vk_fn.CmdDispatch(vk_cmd_buffer,
                                  (bb_w + 7u) / 8u,
                                  (bb_h + 7u) / 8u,
                                  1);
            }
        }

        if (alias_active) {
            /* Alias-model dispatches (Phase 5b-06).  Each
             * dispatch runs imageAtomicMax(z) then a
             * conditional imageStore(color) -- the
             * atomicMax is globally atomic but the store
             * that follows isn't.  Without a barrier
             * between consecutive dispatches, the GPU
             * schedules them concurrently and two
             * invocations at the same pixel can both
             * decide to write (each having read its
             * old-Z before the other's atomicMax was
             * visible) -- the later store then wins
             * regardless of Z order.  In a typical scene
             * this surfaces when a distant alias entity
             * (e.g. a torch flame attached to a wall)
             * overlaps a nearer one (e.g. a monster
             * standing in front of it): the flame's
             * imageStore can win and bleed in front of
             * the monster.
             *
             * Insert a SHADER_WRITE -> SHADER_READ |
             * SHADER_WRITE memory barrier between each
             * pair of consecutive dispatches.  Cost is
             * one VkMemoryBarrier per dispatch; with the
             * batching change in 795c2c5 the per-frame
             * dispatch count is small (one batched call
             * per entity plus a handful for clipped
             * fans), so the barrier overhead is
             * negligible against the dispatch work
             * itself.
             *
             * Order vs particles + sprites: those
             * subsystems have the same store-after-
             * atomicMax pattern but typically issue ONE
             * combined dispatch each, so they can't race
             * with themselves; cross-subsystem races
             * (particle vs alias at the same pixel) are
             * possible but rare in practice -- the
             * particle and sprite footprints are tiny
             * and they're additive bright effects where
             * a one-frame Z hiccup is hard to see.
             * Cross-subsystem barriers can be added if
             * the artefact ever shows up. */
            uint32_t ai;

            vk_fn.CmdBindPipeline(vk_cmd_buffer,
                                  VK_PIPELINE_BIND_POINT_COMPUTE,
                                  vk_alias_pipeline);
            vk_fn.CmdBindDescriptorSets(vk_cmd_buffer,
                                        VK_PIPELINE_BIND_POINT_COMPUTE,
                                        vk_alias_pipeline_layout,
                                        0,
                                        1, &vk_alias_set[vk_active_slot],
                                        0, NULL);

            for (ai = 0; ai < vk_alias_count; ai++) {
                const struct vk_alias_pc *apc = &vk_alias_calls[ai];
                uint32_t bb_w, bb_h;

                vk_fn.CmdPushConstants(vk_cmd_buffer,
                                       vk_alias_pipeline_layout,
                                       VK_SHADER_STAGE_COMPUTE_BIT,
                                       0, sizeof(*apc),
                                       apc);

                bb_w = (uint32_t)(apc->bbox[2] - apc->bbox[0]);
                bb_h = (uint32_t)(apc->bbox[3] - apc->bbox[1]);

                vk_fn.CmdDispatch(vk_cmd_buffer,
                                  (bb_w + 7u) / 8u,
                                  (bb_h + 7u) / 8u,
                                  1);

                if (ai + 1u < vk_alias_count) {
                    VkMemoryBarrier mb;

                    memset(&mb, 0, sizeof(mb));
                    mb.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
                    mb.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
                    mb.dstAccessMask = VK_ACCESS_SHADER_READ_BIT
                                     | VK_ACCESS_SHADER_WRITE_BIT;

                    vk_fn.CmdPipelineBarrier(vk_cmd_buffer,
                                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                             0,
                                             1, &mb,
                                             0, NULL,
                                             0, NULL);
                }
            }
        }

        /* Phase 5b-07b followup: pass-2 turb dispatch slot.
         * Mirror of the pass-1 dispatch loop above (which
         * runs between sky and particles when surfaces are
         * single-pass), positioned here so:
         *
         *   - Particles / sprites / alias have already
         *     atomicMaxed against vk_zbuffer.  Pass-2 turb's
         *     z-test sees the same Z state SW pass-2 would
         *     have seen if alias had run inline against
         *     d_pzbuffer (which it doesn't, but the visual
         *     intent is "water z-tests against everything
         *     drawn before the pass-2 stipple").  This
         *     diverges from strict SW pass-2 semantics
         *     (which z-test against pass-1 world Z only,
         *     ignoring alias) but produces a strictly more-
         *     correct result: a player model in front of
         *     water no longer gets water-smeared in the
         *     stipple-pass pixels.  See the commit message
         *     for the trade-off.
         *
         *   - The pass-2 turb writes color only and never
         *     touches vk_zbuffer, so no atomicMax race with
         *     the just-finished alias dispatches.
         *
         * Atlas image upload already happened in the pre-
         * alias `if (vk_turb_surface_count > 0)` block, so
         * the atlas is in GENERAL layout and ready to read
         * here.  A SHADER_WRITE -> SHADER_READ barrier is
         * needed to make the alias / particle / sprite
         * dispatch writes visible to this dispatch's
         * imageLoad on u_zbuffer (none of those preceding
         * blocks ended with a barrier scoped to "next read"
         * -- the barriers inside alias_active sequence one
         * alias-batch dispatch against the next). */
        if (vk_turb_surface_count > 0
            && vk_turb_surfaces[0].is_pass2)
        {
            VkMemoryBarrier tu2_pre_bar;
            VkMemoryBarrier tu2_post_bar;
            unsigned        si;

            memset(&tu2_pre_bar, 0, sizeof(tu2_pre_bar));
            tu2_pre_bar.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            tu2_pre_bar.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            tu2_pre_bar.dstAccessMask = VK_ACCESS_SHADER_READ_BIT
                                      | VK_ACCESS_SHADER_WRITE_BIT;
            vk_fn.CmdPipelineBarrier(vk_cmd_buffer,
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     0,
                                     1, &tu2_pre_bar,
                                     0, NULL,
                                     0, NULL);

            vk_fn.CmdBindPipeline(vk_cmd_buffer,
                                  VK_PIPELINE_BIND_POINT_COMPUTE,
                                  vk_turb_pipeline);
            vk_fn.CmdBindDescriptorSets(vk_cmd_buffer,
                                        VK_PIPELINE_BIND_POINT_COMPUTE,
                                        vk_turb_pipeline_layout,
                                        0,
                                        1, &vk_turb_set[vk_active_slot],
                                        0, NULL);
            for (si = 0; si < vk_turb_surface_count; si++) {
                const struct vk_turb_surface *surf = &vk_turb_surfaces[si];
                struct {
                    uint32_t bbox[4];
                    uint32_t range[4];
                    float    grad0[4];
                    float    grad1[4];
                    float    grad2[4];
                    int32_t  adj[4];
                    uint32_t mode[4];
                } push;
                push.bbox[0]  = surf->bbox_min_x;
                push.bbox[1]  = surf->bbox_min_y;
                push.bbox[2]  = surf->bbox_w;
                push.bbox[3]  = surf->bbox_h;
                push.range[0] = surf->rows_first;
                push.range[1] = (uint32_t)surf->phase;
                push.range[2] = surf->spans_first;
                push.range[3] = surf->tex_slot;
                push.grad0[0] = surf->grad.sdivzorigin;
                push.grad0[1] = surf->grad.sdivzstepu;
                push.grad0[2] = surf->grad.sdivzstepv;
                push.grad0[3] = 0.0f;
                push.grad1[0] = surf->grad.tdivzorigin;
                push.grad1[1] = surf->grad.tdivzstepu;
                push.grad1[2] = surf->grad.tdivzstepv;
                push.grad1[3] = 0.0f;
                push.grad2[0] = surf->grad.ziorigin;
                push.grad2[1] = surf->grad.zistepu;
                push.grad2[2] = surf->grad.zistepv;
                push.grad2[3] = 0.0f;
                push.adj[0]   = surf->grad.sadjust;
                push.adj[1]   = surf->grad.tadjust;
                push.adj[2]   = surf->grad.bbextents;
                push.adj[3]   = surf->grad.bbextentt;
                push.mode[0]  = surf->alpha;
                push.mode[1]  = surf->is_pass2;
                push.mode[2]  = 0;
                push.mode[3]  = 0;
                vk_fn.CmdPushConstants(vk_cmd_buffer,
                                       vk_turb_pipeline_layout,
                                       VK_SHADER_STAGE_COMPUTE_BIT,
                                       0, sizeof(push), &push);
                vk_fn.CmdDispatch(vk_cmd_buffer,
                                  (surf->bbox_w + 7u) / 8u,
                                  (surf->bbox_h + 7u) / 8u,
                                  1);
            }

            memset(&tu2_post_bar, 0, sizeof(tu2_post_bar));
            tu2_post_bar.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
            tu2_post_bar.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
            tu2_post_bar.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            vk_fn.CmdPipelineBarrier(vk_cmd_buffer,
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                     0,
                                     1, &tu2_post_bar,
                                     0, NULL,
                                     0, NULL);
        }

        /* After the compute dispatches, vk_texture is in
         * GENERAL.  The downstream palette compute
         * dispatch reads it as a sampled image (the
         * existing pipeline binds vk_texture_view via
         * descriptor set 0 binding 0 with imageLayout =
         * SHADER_READ_ONLY_OPTIMAL).  Transition GENERAL
         * -> SHADER_READ_ONLY_OPTIMAL, with srcStage =
         * dstStage = COMPUTE_SHADER so the compute-3D
         * dispatches' writes flush before the palette
         * dispatch's reads.  srcAccess = SHADER_WRITE
         * (what the compute-3D dispatches did);
         * dstAccess = SHADER_READ (what the palette
         * dispatch will do). */
        memset(&barriers[0], 0, sizeof(barriers[0]));
        barriers[0].sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barriers[0].srcAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;
        barriers[0].dstAccessMask                   = VK_ACCESS_SHADER_READ_BIT;
        barriers[0].oldLayout                       = VK_IMAGE_LAYOUT_GENERAL;
        barriers[0].newLayout                       = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barriers[0].srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barriers[0].dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
        barriers[0].image                           = vk_texture;
        barriers[0].subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        barriers[0].subresourceRange.baseMipLevel   = 0;
        barriers[0].subresourceRange.levelCount     = 1;
        barriers[0].subresourceRange.baseArrayLayer = 0;
        barriers[0].subresourceRange.layerCount     = 1;

        vk_fn.CmdPipelineBarrier(vk_cmd_buffer,
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 0,
                                 0, NULL,
                                 0, NULL,
                                 1, barriers);
    } else {
        /* No-particles fast path: both textures
         * TRANSFER_DST -> SHADER_READ_ONLY directly.
         * dstStage = COMPUTE_SHADER (was FRAGMENT_SHADER
         * in Phase 4f) -- the consumer is now the
         * compute dispatch below. */
        barriers[0].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barriers[0].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barriers[0].oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barriers[0].newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barriers[1].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barriers[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        barriers[1].oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barriers[1].newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        vk_fn.CmdPipelineBarrier(vk_cmd_buffer,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                 0,
                                 0, NULL,
                                 0, NULL,
                                 2, barriers);
    }

    /* Render target: UNDEFINED -> GENERAL.  UNDEFINED-as-
     * discard because the compute dispatch will write
     * every pixel via imageStore.  dstStage = COMPUTE_SHADER
     * orders the layout transition before the dispatch's
     * SHADER_WRITE. */
    memset(&image_barrier, 0, sizeof(image_barrier));
    image_barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_barrier.srcAccessMask                   = 0;
    image_barrier.dstAccessMask                   = VK_ACCESS_SHADER_WRITE_BIT;
    image_barrier.oldLayout                       = VK_IMAGE_LAYOUT_UNDEFINED;
    image_barrier.newLayout                       = VK_IMAGE_LAYOUT_GENERAL;
    image_barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    image_barrier.image                           = vk_image[vk_active_slot];
    image_barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    image_barrier.subresourceRange.baseMipLevel   = 0;
    image_barrier.subresourceRange.levelCount     = 1;
    image_barrier.subresourceRange.baseArrayLayer = 0;
    image_barrier.subresourceRange.layerCount     = 1;

    vk_fn.CmdPipelineBarrier(vk_cmd_buffer,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                             0,
                             0, NULL,
                             0, NULL,
                             1, &image_barrier);

    /* Compute dispatch.  Workgroup size 16x16 (declared in
     * the shader); dispatch ceil(width/16) x ceil(height/16)
     * workgroups so we cover the whole image, with the
     * shader's bounds check handling the overshoot when
     * width or height isn't a multiple of 16.
     *
     * Phase 5b-03: when vk_warp_active, dispatch the fused
     * warp + palette pipeline instead of the regular
     * palette pipeline.  Same workgroup layout, same
     * dispatch grid, same image-layout requirements
     * (vk_texture SHADER_READ_ONLY, vk_palette_texture
     * SHADER_READ_ONLY, vk_image GENERAL) -- the only
     * differences are the pipeline / pipeline-layout /
     * descriptor-set handles and the additional push-
     * constants payload (phase + scr_vrect bounds).  The
     * warp pipeline reads its sin-table UBO as binding 3
     * of its own descriptor set; nothing to coordinate
     * with the rest of the frame's barrier sequence
     * (UBO reads don't need a layout transition). */
    if (vk_warp_active) {
        vk_fn.CmdBindPipeline(vk_cmd_buffer,
                              VK_PIPELINE_BIND_POINT_COMPUTE,
                              vk_warp_pipeline);
        vk_fn.CmdBindDescriptorSets(vk_cmd_buffer,
                                    VK_PIPELINE_BIND_POINT_COMPUTE,
                                    vk_warp_pipeline_layout,
                                    0,
                                    1, &vk_warp_set[vk_active_slot],
                                    0, NULL);
        vk_fn.CmdPushConstants(vk_cmd_buffer,
                               vk_warp_pipeline_layout,
                               VK_SHADER_STAGE_COMPUTE_BIT,
                               0, sizeof(vk_warp_push),
                               &vk_warp_push);
    } else {
        vk_fn.CmdBindPipeline(vk_cmd_buffer,
                              VK_PIPELINE_BIND_POINT_COMPUTE,
                              vk_compute_pipeline);
        vk_fn.CmdBindDescriptorSets(vk_cmd_buffer,
                                    VK_PIPELINE_BIND_POINT_COMPUTE,
                                    vk_pipeline_layout,
                                    0,                /* firstSet */
                                    1, &vk_descriptor_set[vk_active_slot],
                                    0, NULL);          /* no dynamic offsets */
    }

    group_count_x = (width  + 15u) / 16u;
    group_count_y = (height + 15u) / 16u;
    vk_fn.CmdDispatch(vk_cmd_buffer, group_count_x, group_count_y, 1);

    /* Phase 4h: overlay render pass.  No explicit
     * pipeline barrier between the compute dispatch and
     * the render pass; the render pass's EXTERNAL -> 0
     * subpass dependency carries the memory dependency
     * (srcStage = COMPUTE_SHADER, srcAccess = SHADER_
     * WRITE -> dstStage = COLOR_ATTACHMENT_OUTPUT,
     * dstAccess = COLOR_ATTACHMENT_WRITE | _READ), and
     * the attachment description's
     * initialLayout = GENERAL receives the image straight
     * from the dispatch.
     *
     * The render pass's finalLayout = SHADER_READ_ONLY_
     * OPTIMAL is what hands the image to the frontend in
     * the right layout; no explicit post-render-pass
     * barrier needed (the render pass implicitly does
     * the COLOR_ATTACHMENT_OPTIMAL ->
     * SHADER_READ_ONLY_OPTIMAL transition at end).
     *
     * The Draw is 4 vertices, 1 instance: triangle-strip
     * topology turns those into the 2 triangles of the
     * gradient quad. */
    memset(&rpbi, 0, sizeof(rpbi));
    rpbi.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.renderPass               = vk_overlay_render_pass;
    rpbi.framebuffer              = vk_overlay_framebuffer[vk_active_slot];
    rpbi.renderArea.extent.width  = width;
    rpbi.renderArea.extent.height = height;
    rpbi.clearValueCount          = 0;
    rpbi.pClearValues             = NULL;

    /* Phase 4k: write the vertex buffer from the draw
     * list Draw_Pic intercepts populated during retro_run.
     * Phase 4j had this fill happen once at create_
     * resources; moving it here lets the draw list change
     * each frame.  Safe to call with vk_overlay_draw_count
     * == 0 (the loop simply does nothing and the per-
     * frame CmdDraw count below stays zero too -- the
     * render pass still runs but draws nothing, which is
     * cheap). */
    backend_vk_fill_overlay_vb();

    vk_fn.CmdBeginRenderPass(vk_cmd_buffer, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    vk_fn.CmdBindPipeline(vk_cmd_buffer,
                          VK_PIPELINE_BIND_POINT_GRAPHICS,
                          vk_overlay_pipeline);
    /* Phase 4j: bind the overlay vertex buffer once, then
     * loop the draw list -- each entry rebinds its slot's
     * descriptor set (different texture per pic) and
     * draws 4 verts at the right firstVertex offset.  The
     * GRAPHICS bind point is independent of the COMPUTE
     * bind point used earlier in the frame, so the
     * compute set stays bound where the compute dispatch
     * left it. */
    vb_offset = 0;
    vk_fn.CmdBindVertexBuffers(vk_cmd_buffer,
                               0,                          /* firstBinding */
                               1, &vk_overlay_vertex_buffer[vk_active_slot],
                               &vb_offset);
    {
        unsigned di;
        for (di = 0; di < vk_overlay_draw_count; di++) {
            const struct overlay_draw *d   = &vk_overlay_draws[di];
            const struct overlay_slot *slt = &vk_overlay_slots[d->slot_idx];
            vk_fn.CmdBindDescriptorSets(vk_cmd_buffer,
                                        VK_PIPELINE_BIND_POINT_GRAPHICS,
                                        vk_pipeline_layout,
                                        0,
                                        1, &slt->descriptor_set,
                                        0, NULL);
            vk_fn.CmdDraw(vk_cmd_buffer, 4, 1, di * 4, 0);
        }
    }
    vk_fn.CmdEndRenderPass(vk_cmd_buffer);

    r = vk_fn.EndCommandBuffer(vk_cmd_buffer);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR, "rhi-vk: EndCommandBuffer failed (%d)\n", (int)r);
        return false;
    }

    /* Phase 4k: reset the draw list now that the command
     * buffer has captured everything it needs.  The next
     * frame's retro_run will populate it from scratch via
     * Draw_Pic intercepts.  Slot data stays put -- only
     * the per-frame queue resets; the cache lives until
     * destroy_resources. */
    vk_overlay_draw_count = 0;

    /* Phase 5b-02: reset the particle dispatch count now
     * that the command buffer has captured the dispatch.
     * Next frame's R_DrawParticles will repopulate it if
     * GPU particles run again; otherwise the no-particles
     * fast path takes over. */
    vk_pending_particle_count = 0;

    /* Phase 5b-03: reset the warp flag too.  Next frame's
     * R_RenderView will re-set it via dispatch_3d_warp_
     * screen if the view is still in water. */
    vk_warp_active = false;

    /* Phase 5b-05: reset the sprite render list.  Next
     * frame's D_DrawSprite calls repopulate it. */
    vk_sprite_count         = 0;
    vk_sprite_vertex_cursor = 0;

    /* Phase 5b-06: reset the alias render list + caches.
     * Next frame's D_PolysetDraw calls repopulate. */
    vk_alias_count           = 0;
    vk_alias_vertex_cursor   = 0;
    vk_alias_triangle_cursor = 0;
    vk_alias_skin_count      = 0;
    vk_alias_cmap_count      = 0;

    /* Phase 5b-07a step 2: reset the sky span collector.
     * Next frame's d_edge.c::D_DrawSurfaces will repopulate
     * via dispatch_3d_sky_span on every SURF_DRAWSKY span. */
    vk_sky_collected_count = 0;

    /* Phase 5b-07b: reset the turb per-frame collector +
     * texture pointer cache + upload window.  Texture data
     * stays in the atlas image; the cache reset just forces
     * re-resolution next frame, with hits returning the same
     * slot for the same cacheblock pointer (textures are
     * Hunk-allocated and stable for the level, so the second
     * frame's cache fills with the same {ptr -> slot}
     * mapping in dispatch order until the level ends and a
     * destroy/create cycle resets everything anyway). */
    vk_turb_surface_count    = 0;
    vk_turb_rows_used        = 0;
    vk_turb_spans_used       = 0;
    vk_turb_tex_count        = 0;
    vk_turb_tex_upload_first = 0;
    vk_turb_tex_upload_last  = 0;

    return true;
}
#endif

static void
backend_vk_end_frame(void)
{
#ifdef RHI_HAVE_VULKAN
    VkSubmitInfo si;
    VkResult     r;

    if (!vk_iface || !vk_resources_ready)
        return;

    /* Refresh the staging buffer with the latest vid.buffer
     * contents before recording the copy command.  The host
     * write must happen before the GPU read; both record
     * (which references the buffer handle, not its bytes)
     * and submit (which actually reads through the handle)
     * come after this. */
    perf_timing_section_begin(PERF_SECTION_UPLOAD_VID);
    backend_vk_upload_vid_buffer();
    perf_timing_section_end(PERF_SECTION_UPLOAD_VID);

    /* Phase 5b-02 + 5b-05 + 5b-06 + 5b-07b: if any compute-
     * 3D dispatch (particles, sprites, alias, or turb) was
     * staged this frame, push d_pzbuffer into the GPU-side
     * zbuffer staging buffer so record_frame can
     * CopyBufferToImage it into vk_zbuffer.  Conditional
     * because the upload is ~8 MiB at 1080p (R32_UINT
     * after Phase 5b-04) -- worth skipping on frames
     * with no GPU 3D activity (the staging buffer's
     * previous contents are irrelevant; nothing reads
     * them).
     *
     * Turb is in this gate even though single-pass turb
     * doesn't read existing Z: the GPU-side gate
     * (zbuf_active in record_frame) needs to fire to do
     * the vk_zbuffer layout transition that single-pass
     * turb's imageStore depends on, and both gates must
     * agree -- if record_frame transitions vk_zbuffer to
     * GENERAL and then CopyBufferToImage's, the host
     * staging needs fresh content too.  Pass-2 turb needs
     * the actual SW pass-1 wall Z values uploaded so its
     * imageLoad-based z-test sees the wall depths. */
    if (vk_pending_particle_count > 0
     || vk_sprite_count > 0
     || vk_alias_count > 0
     || vk_turb_surface_count > 0) {
        perf_timing_section_begin(PERF_SECTION_UPLOAD_ZBUF);
        backend_vk_upload_zbuffer();
        perf_timing_section_end(PERF_SECTION_UPLOAD_ZBUF);
    }

    /* Fresh per-frame command-buffer recording.  Phase 4e
     * lifted this out of create_resources so subsequent
     * phases can vary the recorded commands per frame
     * (different draw counts, conditional passes, etc.).
     * A failure here means we can't submit a coherent
     * frame -- bail and let retro_run's dupe path keep the
     * display going. */

    /* Snapshot per-frame work counters BEFORE record_frame,
     * which consumes the per-entity counts and resets them
     * to 0 at its tail before returning.  Out of any timed
     * section so the perf-counter overhead doesn't bias the
     * RECORD_FRAME or SUBMIT_PRESENT measurements.  The
     * values are populated by R_RenderView dispatches that
     * ran inside the prior Host_Frame call. */
    perf_timing_counter_add(PERF_COUNTER_PARTICLES,      (uint64_t)vk_pending_particle_count);
    perf_timing_counter_add(PERF_COUNTER_SPRITES,        (uint64_t)vk_sprite_count);
    perf_timing_counter_add(PERF_COUNTER_ALIAS_ENTITIES, (uint64_t)vk_alias_count);
    perf_timing_counter_add(PERF_COUNTER_TURB_SURFACES,  (uint64_t)vk_turb_surface_count);
    perf_timing_counter_add(PERF_COUNTER_DISPATCHES,
                            (uint64_t)(vk_pending_particle_count
                                     + vk_sprite_count
                                     + vk_alias_count
                                     + vk_turb_surface_count));
    perf_timing_counter_add(PERF_COUNTER_VID_BUFFER_BYTES,
                            (uint64_t)(width * height));
    if (vk_pending_particle_count > 0
     || vk_sprite_count > 0
     || vk_alias_count > 0
     || vk_turb_surface_count > 0)
        perf_timing_counter_add(PERF_COUNTER_ZBUF_UPLOAD_BYTES,
                                (uint64_t)(width * height * 4));

    perf_timing_section_begin(PERF_SECTION_RECORD_FRAME);
    if (!backend_vk_record_frame()) {
        perf_timing_section_end(PERF_SECTION_RECORD_FRAME);
        return;
    }
    perf_timing_section_end(PERF_SECTION_RECORD_FRAME);

    /* Submit the just-recorded command buffer. */
    perf_timing_section_begin(PERF_SECTION_SUBMIT_PRESENT);
    memset(&si, 0, sizeof(si));
    si.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    si.commandBufferCount   = 1;
    si.pCommandBuffers      = &vk_cmd_buffer;
    /* Phase 5b-08 step 3b: signal the active slot's
     * render-done semaphore so set_image's handoff to the
     * frontend defers the actual image read until GPU
     * completion.  Binary semaphore; the frontend's wait
     * resets it to unsignaled.  Done_fence is also signaled
     * (pSubmits' fence parameter) so begin_frame's next
     * rotation back to this slot blocks until the slot's
     * prior frame's GPU work completes. */
    si.signalSemaphoreCount = 1;
    si.pSignalSemaphores    = &vk_frame_ctx[vk_active_slot].render_done_semaphore;

    perf_timing_section_begin(PERF_SECTION_QUEUE_SUBMIT);
    r = vk_fn.QueueSubmit(vk_queue, 1, &si,
                          vk_frame_ctx[vk_active_slot].done_fence);
    perf_timing_section_end(PERF_SECTION_QUEUE_SUBMIT);
    if (r != VK_SUCCESS) {
        if (log_cb)
            log_cb(RETRO_LOG_ERROR, "rhi-vk: QueueSubmit failed (%d)\n", (int)r);
        perf_timing_section_end(PERF_SECTION_SUBMIT_PRESENT);
        return;
    }

    /* No host-side wait here.  Cross-frame sync is done by
     * begin_frame's WaitForFences on the rotated-to slot's
     * done_fence (which the QueueSubmit above signals).
     * The frontend's read of vk_image[vk_active_slot] is
     * gated by the render_done_semaphore passed to
     * set_image below, so frame N+1's backend can start
     * recording its CB while the frontend is still
     * consuming frame N's image (different slot, different
     * descriptor set, different framebuffer -- no race).
     * Pre-3b this was a QueueWaitIdle drain; perf_timing
     * tracks the new sync point as "bw" in begin_frame. */

    /* Hand the image to the frontend.  src_queue_family
     * matches the queue we submitted from, so no ownership
     * transfer happens (libretro_vulkan.h spec: if
     * src_queue_family == queue_index, no transfer
     * occurs).  Phase 5b-08 step 3b: pass the active
     * slot's render-done semaphore so the frontend waits
     * for GPU completion before consuming the image. */
    perf_timing_section_begin(PERF_SECTION_SET_IMAGE);
    vk_iface->set_image(vk_iface->handle,
                        &vk_retro_image[vk_active_slot],
                        1, &vk_frame_ctx[vk_active_slot].render_done_semaphore,
                        vk_queue_family);
    perf_timing_section_end(PERF_SECTION_SET_IMAGE);

    /* Tell the frontend a HW frame is ready.  The data
     * pointer is the magic value RETRO_HW_FRAME_BUFFER_VALID;
     * pitch is 0 (not meaningful for HW frames).  did_flip
     * suppresses retro_run's dupe-NULL-frame path. */
    perf_timing_section_begin(PERF_SECTION_VIDEO_CB);
    if (video_cb)
        video_cb(RETRO_HW_FRAME_BUFFER_VALID, width, height, 0);
    perf_timing_section_end(PERF_SECTION_VIDEO_CB);
    did_flip = true;
    perf_timing_section_end(PERF_SECTION_SUBMIT_PRESENT);
#endif
}

/*
 * backend_vk_notify_cache_invalidate -- Phase 5b-06 follow-up
 *
 * zone.c fires this right before a cache payload pointer becomes
 * invalid (Cache_Free, or the Cache_Free inside Cache_Move's
 * relocate-then-discard-old branch).  The overlay slot cache keys
 * each uploaded image by qpic_t pointer; when the Quake cache
 * Hunk-relocates or evicts an entry, the slot that uploaded its
 * pixels is left holding a stale key that could later collide with
 * a different pic landing at the same recycled Hunk address.  Clear
 * the slot->key for any slot whose key falls in [data, data + size)
 * so the next lookup with an aliased pointer no longer matches it.
 *
 * GPU resources stay attached to the slot; freeing them mid-flight
 * is unsafe if commands recorded earlier in the current frame still
 * reference the image.  The LRU pass in backend_vk_begin_frame
 * reclaims the resources once the slot has aged past the grace
 * window with key == NULL (and so won't see another touch).
 *
 * Safe to call before resources are stood up: the slot array is
 * BSS-zero, no slot->key matches, the scan returns instantly. */
static void
backend_vk_notify_cache_invalidate(const void *data, int size)
{
#ifdef RHI_HAVE_VULKAN
    const byte *base, *end;
    unsigned    si;

    if (!data || size <= 0)
        return;

    base = (const byte *)data;
    end  = base + size;

    for (si = 0; si < OVERLAY_SLOT_MAX; si++) {
        struct overlay_slot *slot = &vk_overlay_slots[si];
        const byte          *key  = (const byte *)slot->key;
        if (key && key >= base && key < end)
            slot->key = NULL;
    }
#else
    (void)data;
    (void)size;
#endif
}

/*
 * Phase 5b-07a: cache the sky texture and flag it for upload
 * on the next record_frame.
 *
 * Called from R_InitSky in r_sky.c at level load.  The
 * source pointers index INTO a 256-wide texture (256x128,
 * the Quake sky format), where the LEFT 128 columns are
 * the front masked-overlay layer and the RIGHT 128 columns
 * are the back opaque layer.  r_sky.c hands us the two
 * column-pair-sized pointers (`front` at offset 0, `back`
 * at offset 128 into the same row); each row therefore
 * advances by 256 bytes in the source, but our GPU images
 * are tightly packed 128x128, so the per-row memcpy
 * compresses the stride.
 *
 * Upload itself is deferred to record_frame for three
 * reasons.  First, R_InitSky runs during retro_load_game
 * which is BEFORE the Vulkan context stands up
 * (create_resources hasn't run yet) -- no command pool /
 * command buffer / images exist to do the work on.  Second,
 * folding the upload into the per-frame command stream
 * shares vk_cmd_buffer's BeginCommandBuffer / submit cycle
 * (no separate one-shot CB, no extra QueueSubmit/WaitIdle
 * pair).  Third, after a context reset the CPU cache
 * survives and the create_resources path can re-arm
 * pending without R_InitSky needing to re-fire from the
 * engine side -- the per-frame upload picks it up
 * automatically.
 *
 * Always defined (no outer #ifdef on the symbol itself) so
 * the unconditional g_rhi_backend_vk vtable entry below can
 * reference it whether or not RHI_HAVE_VULKAN is on; the
 * body's internal guard makes the function a no-op in the
 * non-Vulkan build, mirroring the notify_cache_invalidate
 * pattern above.
 */
static void
backend_vk_notify_sky_texture(const byte *front, const byte *back)
{
#ifdef RHI_HAVE_VULKAN
    unsigned row;
    if (!front || !back)
        return;
    for (row = 0; row < VK_SKY_TEXEL_H; row++) {
        memcpy(vk_sky_cache_front + (size_t)row * VK_SKY_TEXEL_W,
               front + (size_t)row * 256,
               VK_SKY_TEXEL_W);
        memcpy(vk_sky_cache_back  + (size_t)row * VK_SKY_TEXEL_W,
               back  + (size_t)row * 256,
               VK_SKY_TEXEL_W);
    }
    vk_sky_cache_populated = true;
    vk_sky_upload_pending  = true;
#else
    (void)front;
    (void)back;
#endif
}

/*
 * backend_vk_dispatch_3d_sky_span -- Phase 5b-07a step 2
 *
 * Called from d_edge.c's SURF_DRAWSKY branch once per
 * espan_t.  Appends to vk_sky_collected[] and grows the
 * screen-space bbox.  The actual GPU work happens in
 * record_frame: bucket-by-row, upload to mapped buffers,
 * CmdDispatch sky.comp.  Drops silently when the table
 * is full (VK_SKY_MAX_SPANS = 8192 is enough for any
 * realistic Quake frame; a typical level has a few hundred
 * sky spans).  Drops silently when resources aren't ready
 * (pre-context-reset state).
 *
 * Always defined so the unconditional vtable can reference
 * it; the body is a no-op without RHI_HAVE_VULKAN.
 */
static void
backend_vk_dispatch_3d_sky_span(int u, int v, int count)
{
#ifdef RHI_HAVE_VULKAN
    struct vk_sky_collected_span *entry;

    if (!vk_resources_ready || count <= 0)
        return;
    if (vk_sky_collected_count >= VK_SKY_MAX_SPANS)
        return;
    if (v < 0 || v >= (int)VK_SKY_MAX_ROWS)
        return;

    entry = &vk_sky_collected[vk_sky_collected_count++];
    entry->u     = (uint16_t)u;
    entry->v     = (uint16_t)v;
    entry->count = (uint16_t)count;

    /* Grow the bbox.  vk_sky_collected_count == 1 is the
     * first-span case where we seed the bbox; subsequent
     * spans expand it. */
    if (vk_sky_collected_count == 1) {
        vk_sky_bbox_min_x = u;
        vk_sky_bbox_max_x = u + count;
        vk_sky_bbox_min_y = v;
        vk_sky_bbox_max_y = v + 1;
    } else {
        if (u < vk_sky_bbox_min_x)         vk_sky_bbox_min_x = u;
        if (u + count > vk_sky_bbox_max_x) vk_sky_bbox_max_x = u + count;
        if (v < vk_sky_bbox_min_y)         vk_sky_bbox_min_y = v;
        if (v + 1 > vk_sky_bbox_max_y)     vk_sky_bbox_max_y = v + 1;
    }
#else
    (void)u;
    (void)v;
    (void)count;
#endif
}

/*
 * backend_vk_dispatch_3d_turb_surface -- Phase 5b-07b
 *
 * Called from d_edge.c's SURF_DRAWTURB branch once per
 * compute-eligible surface (single-pass opaque case).
 * Resolves the surface's texture to an atlas slot (per-frame
 * pointer-keyed cache + on-demand upload), walks the espan_t
 * list to collect spans + bucket-by-row, and records a
 * per-surface entry that record_frame consumes.
 *
 * Per-frame state (vk_turb_surfaces, vk_turb_rows_used,
 * vk_turb_spans_used, vk_turb_tex_count) is reset at
 * begin_frame.  All append operations bail silently when
 * their respective MAX is reached -- the surface falls off
 * the GPU path with no fallback into SW (the host's branch
 * has already committed to compute).  This is conservative
 * but matches sky's behaviour.  Bumping the MAX_* constants
 * costs only BSS; the buffers are sized accordingly.
 *
 * Always defined for the unconditional vtable; body guarded
 * by RHI_HAVE_VULKAN.
 */
static void
backend_vk_dispatch_3d_turb_surface(const void *spans_head,
                                    const rhi_turb_gradient_t *grad,
                                    const byte *texture,
                                    int turb_phase,
                                    int alpha,
                                    int is_pass2)
{
#ifdef RHI_HAVE_VULKAN
    const espan_t *p;
    struct vk_turb_surface *surf;
    uint32_t  tex_slot;
    unsigned  i;
    int       bbox_min_x, bbox_min_y, bbox_max_x, bbox_max_y;
    unsigned  num_spans, bbox_w, bbox_h;
    unsigned  rows_first, spans_first;
    uint32_t *row_entries;
    uint32_t *span_entries;

    if (!vk_resources_ready || !spans_head || !grad || !texture)
        return;
    if (vk_turb_surface_count >= VK_TURB_MAX_SURFACES)
        return;

    /* Pointer-keyed texture cache lookup.  Same shape as the
     * alias skin cache: linear probe, miss = allocate new slot
     * + memcpy 4 KiB (64x64) into the staging buffer at slot
     * offset.  Track the staging range that needs CmdCopy-
     * BufferToImage in record_frame via the upload window. */
    tex_slot = UINT32_MAX;
    for (i = 0; i < vk_turb_tex_count; i++) {
        if (vk_turb_tex_cache[i].ptr == (const void *)texture) {
            tex_slot = vk_turb_tex_cache[i].slot;
            break;
        }
    }
    if (tex_slot == UINT32_MAX) {
        if (vk_turb_tex_count >= VK_TURB_TEX_SLOTS)
            return;
        tex_slot = vk_turb_tex_count;
        vk_turb_tex_cache[vk_turb_tex_count].ptr  = (const void *)texture;
        vk_turb_tex_cache[vk_turb_tex_count].slot = tex_slot;
        vk_turb_tex_count++;
        if (vk_turb_atlas_staging_ptr[vk_active_slot]) {
            memcpy((byte *)vk_turb_atlas_staging_ptr[vk_active_slot]
                       + tex_slot * VK_TURB_SLOT_BYTES,
                   texture, VK_TURB_SLOT_BYTES);
        }
        /* Extend the upload window.  CmdCopyBufferToImage in
         * record_frame copies [upload_first, upload_last). */
        if (vk_turb_tex_upload_last == vk_turb_tex_upload_first) {
            /* Window empty -- seed. */
            vk_turb_tex_upload_first = tex_slot;
            vk_turb_tex_upload_last  = tex_slot + 1;
        } else if (tex_slot + 1 > vk_turb_tex_upload_last) {
            vk_turb_tex_upload_last  = tex_slot + 1;
        }
    }

    /* Walk spans: count, compute bbox.  Drop the surface if
     * we can't fit all its spans in the global span pool. */
    num_spans  = 0;
    bbox_min_x = INT32_MAX;
    bbox_min_y = INT32_MAX;
    bbox_max_x = INT32_MIN;
    bbox_max_y = INT32_MIN;
    for (p = (const espan_t *)spans_head; p; p = p->pnext) {
        if (p->count <= 0)
            continue;
        if (p->u            < bbox_min_x) bbox_min_x = p->u;
        if (p->v            < bbox_min_y) bbox_min_y = p->v;
        if (p->u + p->count > bbox_max_x) bbox_max_x = p->u + p->count;
        if (p->v + 1        > bbox_max_y) bbox_max_y = p->v + 1;
        num_spans++;
    }
    if (num_spans == 0)
        return;
    if (vk_turb_spans_used + num_spans > VK_TURB_MAX_SPANS)
        return;
    bbox_w = (unsigned)(bbox_max_x - bbox_min_x);
    bbox_h = (unsigned)(bbox_max_y - bbox_min_y);
    if (vk_turb_rows_used + bbox_h > VK_TURB_MAX_ROWS)
        return;

    rows_first  = vk_turb_rows_used;
    spans_first = vk_turb_spans_used;
    row_entries  = (uint32_t *)vk_turb_rows_ptr[vk_active_slot]  + 2 * rows_first;
    span_entries = (uint32_t *)vk_turb_spans_ptr[vk_active_slot] + 2 * spans_first;

    /* Pass 1: zero per-row counts. */
    for (i = 0; i < bbox_h; i++) {
        row_entries[2 * i + 0] = 0;
        row_entries[2 * i + 1] = 0;
    }
    /* Pass 2: per-row count.  rinfo.y holds the count temp-
     * orarily; rinfo.x stays 0 to be the prefix-sum target
     * in pass 3. */
    for (p = (const espan_t *)spans_head; p; p = p->pnext) {
        unsigned row;
        if (p->count <= 0)
            continue;
        row = (unsigned)(p->v - bbox_min_y);
        row_entries[2 * row + 1]++;
    }
    /* Pass 3: prefix-sum into rinfo.x, reset rinfo.y to 0 for
     * the placement pass.  rows[i].x ends up as the "first
     * span index, relative to spans_first" for row i. */
    {
        uint32_t running = 0;
        for (i = 0; i < bbox_h; i++) {
            uint32_t c = row_entries[2 * i + 1];
            row_entries[2 * i + 0] = running;
            row_entries[2 * i + 1] = 0;
            running += c;
        }
    }
    /* Pass 4: place each span at rows[row].x + rows[row].y++,
     * then bump rows[row].y so subsequent spans on the same
     * row get the next slot. */
    for (p = (const espan_t *)spans_head; p; p = p->pnext) {
        unsigned row, slot;
        if (p->count <= 0)
            continue;
        row  = (unsigned)(p->v - bbox_min_y);
        slot = row_entries[2 * row + 0] + row_entries[2 * row + 1];
        span_entries[2 * slot + 0] = (uint32_t)p->u;
        span_entries[2 * slot + 1] = (uint32_t)p->count;
        row_entries[2 * row + 1]++;
    }

    /* Record the per-surface entry. */
    surf = &vk_turb_surfaces[vk_turb_surface_count++];
    surf->bbox_min_x  = (uint32_t)bbox_min_x;
    surf->bbox_min_y  = (uint32_t)bbox_min_y;
    surf->bbox_w      = bbox_w;
    surf->bbox_h      = bbox_h;
    surf->rows_first  = rows_first;
    surf->spans_first = spans_first;
    surf->tex_slot    = tex_slot;
    surf->phase       = turb_phase;
    surf->alpha       = (uint32_t)alpha;
    surf->is_pass2    = (uint32_t)(is_pass2 != 0);
    surf->grad        = *grad;

    vk_turb_rows_used  += bbox_h;
    vk_turb_spans_used += num_spans;
#else
    (void)spans_head;
    (void)grad;
    (void)texture;
    (void)turb_phase;
    (void)alpha;
    (void)is_pass2;
#endif
}

const render_backend_t g_rhi_backend_vk = {
    "vulkan",
    RHI_BACKEND_VULKAN,
    backend_vk_init,
    backend_vk_shutdown,
    backend_vk_begin_frame,
    backend_vk_draw_view,
    backend_vk_end_frame,
    backend_vk_queue_2d_pic_entry,
    backend_vk_queue_2d_char_entry,
    backend_vk_queue_2d_pic_scaled_entry,
    backend_vk_queue_2d_console_background_entry,
    backend_vk_queue_2d_pic_translate_scaled_entry,
    backend_vk_queue_2d_fill_entry,
    backend_vk_queue_2d_fade_screen_entry,
    backend_vk_dispatch_3d_particles_impl,
    backend_vk_dispatch_3d_warp_screen_impl,
    backend_vk_dispatch_3d_sprite_impl,
    backend_vk_dispatch_3d_alias_impl,
    backend_vk_notify_cache_invalidate,
    backend_vk_notify_sky_texture,
    backend_vk_dispatch_3d_sky_span,
    backend_vk_dispatch_3d_turb_surface
};
