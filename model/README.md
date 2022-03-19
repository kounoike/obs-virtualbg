curl https://github.com/google/mediapipe/raw/master/mediapipe/modules/selfie_segmentation/selfie_segmentation_landscape.tflite
docker pull ghcr.io/pinto0309/tflite2tensorflow:latest

docker run ......

tflite2tensorflow \
  --model_path segm_full_v679.tflite \
  --flatc_path ../flatc \
  --schema_path ../schema.fbs \
  --output_pb

