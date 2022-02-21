#include "jbf.h"
#include <HalideBuffer.h>
#include <halide_image_io.h>

using namespace Halide::Runtime;
using namespace Halide::Tools;

int main(int argc, char *argv[]) {
  Buffer<float> input = load_and_convert_image(argv[1]);
  Buffer<float> reference = load_and_convert_image(argv[2]);
  Buffer<float> output(input.width(), input.height());

  float r_sigma = (float)atof(argv[3]);
  jbf(input, reference, r_sigma, output);

  convert_and_save_image(output, argv[4]);
}
