// Original Bilateral filter code from:
// https://github.com/halide/Halide/blob/392430d1133ee8cc5f4fe87b4dd06ac9779eb8d8/apps/bilateral_grid/bilateral_grid_generator.cpp

#include <Halide.h>
#include <halide_trace_config.h>

namespace {

class JointBilateralGrid : public Halide::Generator<JointBilateralGrid> {
public:
  GeneratorParam<int> s_sigma{"s_sigma", 8};

  Input<Buffer<float>> input{"input", 2};
  Input<Buffer<float>> reference{"reference", 2};
  Input<float> r_sigma{"r_sigma"};

  Output<Buffer<float>> joint_bilateral_grid{"joint_bilateral_grid", 2};

  void generate() {
    Var x("x"), y("y"), z("z"), c("c");

    // Add a boundary condition
    Func clamped = Halide::BoundaryConditions::repeat_edge(input);
    Func clamped_reference = Halide::BoundaryConditions::repeat_edge(reference);

    const int rad = 7;

    RDom r(-rad, 2 * rad + 1, -rad, 2 * rad + 1);
    Func blur("blur");
    Expr ds = -1.0f / (2.0f * s_sigma * s_sigma);
    Expr dc = -1.0f / (2.0f * r_sigma * r_sigma);
    Expr total =
        Halide::sum(Halide::fast_exp((r.x * r.x + r.y * r.y) * ds) *
                    Halide::fast_exp((clamped_reference(x + r.x, y + r.y) - clamped_reference(x, y)) *
                                     (clamped_reference(x + r.x, y + r.y) - clamped_reference(x, y)) * dc));
    blur(x, y) += Halide::fast_exp((r.x * r.x + r.y * r.y) * ds) *
                  Halide::fast_exp((clamped_reference(x + r.x, y + r.y) - clamped_reference(x, y)) *
                                   (clamped_reference(x + r.x, y + r.y) - clamped_reference(x, y)) * dc) *
                  clamped(x + r.x, y + r.y);

    // Normalize
    joint_bilateral_grid(x, y) = blur(x, y) / total;

    /* ESTIMATES */
    // (This can be useful in conjunction with RunGen and benchmarks as well
    // as auto-schedule, so we do it in all cases.)
    // Provide estimates on the input image
    input.set_estimates({{0, 1536}, {0, 2560}});
    reference.set_estimates({{0, 1536}, {0, 2560}});
    // Provide estimates on the parameters
    r_sigma.set_estimate(0.1f);
    // TODO: Compute estimates from the parameter values
    // histogram.set_estimate(z, -2, 16);
    // blurz.set_estimate(z, 0, 12);
    // blurx.set_estimate(z, 0, 12);
    // blury.set_estimate(z, 0, 12);
    joint_bilateral_grid.set_estimates({{0, 1536}, {0, 2560}});

    // if (auto_schedule) {
    //   // nothing
    // } else if (get_target().has_gpu_feature()) {
    //   // 0.50ms on an RTX 2060

    //   Var xi("xi"), yi("yi"), zi("zi");

    //   // Schedule blurz in 8x8 tiles. This is a tile in
    //   // grid-space, which means it represents something like
    //   // 64x64 pixels in the input (if s_sigma is 8).
    //   blurz.compute_root().reorder(c, z, x, y).gpu_tile(x, y, xi, yi, 8, 8);

    //   // Schedule histogram to happen per-tile of blurz, with
    //   // intermediate results in shared memory. This means histogram
    //   // and blurz makes a three-stage kernel:
    //   // 1) Zero out the 8x8 set of histograms
    //   // 2) Compute those histogram by iterating over lots of the input image
    //   // 3) Blur the set of histograms in z
    //   histogram.reorder(c, z, x, y).compute_at(blurz, x).gpu_threads(x, y);
    //   histogram.update().reorder(c, r.x, r.y, x, y).gpu_threads(x, y).unroll(c);

    //   // Schedule the remaining blurs and the sampling at the end similarly.
    //   blurx.compute_root()
    //       .reorder(c, x, y, z)
    //       .reorder_storage(c, x, y, z)
    //       .vectorize(c)
    //       .unroll(y, 2, TailStrategy::RoundUp)
    //       .gpu_tile(x, y, z, xi, yi, zi, 32, 8, 1, TailStrategy::RoundUp);
    //   blury.compute_root()
    //       .reorder(c, x, y, z)
    //       .reorder_storage(c, x, y, z)
    //       .vectorize(c)
    //       .unroll(y, 2, TailStrategy::RoundUp)
    //       .gpu_tile(x, y, z, xi, yi, zi, 32, 8, 1, TailStrategy::RoundUp);
    //   joint_bilateral_grid.compute_root().gpu_tile(x, y, xi, yi, 32, 8);
    //   interpolated.compute_at(joint_bilateral_grid, xi).vectorize(c);
    // } else {
    //   // CPU schedule.

    //   // 3.98ms on an Intel i9-9960X using 32 threads at 3.7 GHz
    //   // using target x86-64-avx2. This is a little less
    //   // SIMD-friendly than some of the other apps, so we
    //   // benefit from hyperthreading, and don't benefit from
    //   // AVX-512, which on my machine reduces the clock to 3.0
    //   // GHz.

    //   blurz.compute_root().reorder(c, z, x, y).parallel(y).vectorize(x, 8).unroll(c);
    //   histogram.compute_at(blurz, y);
    //   histogram.update().reorder(c, r.x, r.y, x, y).unroll(c);
    //   blurx.compute_root().reorder(c, x, y, z).parallel(z).vectorize(x, 8).unroll(c);
    //   blury.compute_root().reorder(c, x, y, z).parallel(z).vectorize(x, 8).unroll(c);
    //   joint_bilateral_grid.compute_root().parallel(y).vectorize(x, 8);
    // }

    /* Optional tags to specify layout for HalideTraceViz */
    {
      Halide::Trace::FuncConfig cfg;
      cfg.pos.x = 100;
      cfg.pos.y = 300;
      input.add_trace_tag(cfg.to_trace_tag());

      cfg.pos.x = 1564;
      joint_bilateral_grid.add_trace_tag(cfg.to_trace_tag());
    }
    {
      Halide::Trace::FuncConfig cfg;
      cfg.strides = {{1, 0}, {0, 1}, {40, 0}};
      cfg.zoom = 3;

      cfg.max = 32;
      cfg.pos.x = 550;
      cfg.pos.y = 100;
      // histogram.add_trace_tag(cfg.to_trace_tag());

      cfg.max = 512;
      cfg.pos.y = 300;
      // blurz.add_trace_tag(cfg.to_trace_tag());

      cfg.max = 8192;
      cfg.pos.y = 500;
      // blurx.add_trace_tag(cfg.to_trace_tag());

      cfg.max = 131072;
      cfg.pos.y = 700;
      // blury.add_trace_tag(cfg.to_trace_tag());
    }
    {
      // GlobalConfig applies to the entire visualization pipeline;
      // you can set this tag on any Func that is realized, but only
      // the last one seen will be used. (Since the tags are emitted in
      // an arbitrary order, emitting only one such tag is the best practice).
      // Note also that since the global settings are often context-dependent
      // (eg the output size and timestep may vary depending on the
      // input data), it's often more useful to specify these on the
      // command line.
      Halide::Trace::GlobalConfig global_cfg;
      global_cfg.timestep = 1000;

      joint_bilateral_grid.add_trace_tag(global_cfg.to_trace_tag());
    }
  }
};

} // namespace

HALIDE_REGISTER_GENERATOR(JointBilateralGrid, joint_bilateral_grid)
