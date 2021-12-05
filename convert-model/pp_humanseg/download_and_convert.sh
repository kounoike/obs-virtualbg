#!/bin/sh

MODEL=ppseg_lite_portrait_398x224_with_softmax
INTEL_OPENVINO_DIR=/opt/intel/openvino_2021

[ -e ppseg_lite_portrait_398x224_with_softmax.tar.gz ] || wget https://paddleseg.bj.bcebos.com/dygraph/ppseg/ppseg_lite_portrait_398x224_with_softmax.tar.gz
[ -d ppseg_lite_portrait_398x224_with_softmax ] || tar zxvf ppseg_lite_portrait_398x224_with_softmax.tar.gz

paddle2onnx --model_dir $MODEL \
    --model_filename model.pdmodel \
    --params_filename model.pdiparams \
    --opset_version 11 \
    --enable_onnx_checker True \
    --save_file ${MODEL}.onnx \
    --input_shape_dict='{"x":[1,3,224,398]}'

[ -d saved_model ] && rm -r saved_model

python3 -m onnxsim ${MODEL}.onnx ${MODEL}_simplified.onnx

$INTEL_OPENVINO_DIR/deployment_tools/model_optimizer/mo.py \
--input_model ${MODEL}_simplified.onnx \
--data_type FP32 \
--static_shape \
--output_dir saved_model/openvino/FP32


openvino2tensorflow \
--model_path saved_model/openvino/FP32/${MODEL}_simplified.xml \
--output_saved_model \
--output_pb \
--output_onnx \
--onnx_opset 11 \
--keep_input_tensor_in_nchw \
--weight_replacement_config replace.json
