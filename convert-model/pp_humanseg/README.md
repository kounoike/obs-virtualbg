Model from: https://github.com/PaddlePaddle/PaddleSeg/tree/release/2.3/contrib/PP-HumanSeg
Convert script referenced: https://github.com/PINTO0309/PINTO_model_zoo/blob/bdb3220b35e3bbeda3c04dd55402206aae4aac83/196_human_segmentation_pphumanseg/convert_script.txt


run PINTO openvino2tensorflow container:

```
docker run --gpus all -it --rm \
-v `pwd`:/home/user/workdir \
ghcr.io/pinto0309/openvino2tensorflow:latest
```

and do `sh download_and_convert.sh`. `saved_model/model_float32.onnx` is converted ONNX model file.
