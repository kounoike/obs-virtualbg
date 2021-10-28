#include <Halide.h>

using Halide::ConciseCasts::f32;
using Halide::ConciseCasts::u8_sat;

class BlurGenerator : public Halide::Generator<BlurGenerator> {
public:
  Halide::GeneratorInput<Halide::Buffer<uint8_t>> input{"input", 2};
  Halide::GeneratorOutput<Halide::Buffer<uint8_t>> output{"output", 2};

  Halide::Var x, y;
  // Func kernel;
  Func blur_y;
  const float sigma = 1.5f;

  void generate() {
    Func in_bounded = Halide::BoundaryConditions::repeat_edge(input);
    Func in_float;
    in_float(x, y) = f32(in_bounded(x, y));
    blur_y(x, y) = (2.0f * in_float(x, y) + (in_float(x, y - 1) + in_float(x, y + 1)));
    output(x, y) = u8_sat((2.0f * blur_y(x, y) + (blur_y(x - 1, y) + blur_y(x + 1, y))) / 16.0f);
  }

  void schedule() {
    if (!auto_schedule) {
      output.compute_root().vectorize(x, 8).parallel(y);
      blur_y.compute_at(output, y).vectorize(x, 8);
    } else {
      output.set_estimates({{0, 256}, {0, 144}});
      input.set_estimates({{0, 256}, {0, 144}});
    }
  }
};

HALIDE_REGISTER_GENERATOR(BlurGenerator, blur_generator)
