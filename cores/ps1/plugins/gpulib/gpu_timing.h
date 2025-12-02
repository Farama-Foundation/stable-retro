
// very conservative and wrong
#define gput_fill(w, h)     (23 + (4 + (w) / 16u) * (h))
#define gput_copy(w, h)     ((w) * (h))
#define gput_poly_base()    (23)
#define gput_poly_base_t()  (gput_poly_base() + 90)
#define gput_poly_base_g()  (gput_poly_base() + 144)
#define gput_poly_base_gt() (gput_poly_base() + 225)
#define gput_quad_base()    gput_poly_base()
#define gput_quad_base_t()  gput_poly_base_t()
#define gput_quad_base_g()  gput_poly_base_g()
#define gput_quad_base_gt() gput_poly_base_gt()
#define gput_line(k)        (8 + (k))
#define gput_sprite(w, h)   (8 + ((w) / 2u) * (h))

// sort of a workaround for lack of proper fifo emulation
#define gput_sum(sum, cnt, new_cycles) do { \
  sum += cnt; cnt = new_cycles; \
} while (0)
