#!/bin/bash

docker run --rm --user=root \
  -e http_proxy -e https_proxy -e no_proxy \
  -v "$(dirname "$(readlink -f "$0")"):/opt/project" \
  intel/dlstreamer:2025.0.1.3-ubuntu24 bash -c "$(cat <<EOF

cd /opt/project
export HOST_IP="${1:-$(hostname -I | cut -f1 -d' ')}"
echo "Configuring application to use \$HOST_IP"

# shellcheck disable=SC1091
. ./update_dashboard.sh \$HOST_IP

##############################################################################
# Process OMZ models
##############################################################################
mkdir -p src/dlstreamer-pipeline-server/models/intel
OMZ_MODELS=(pedestrian-and-vehicle-detector-adas-0001)
for model in "\${OMZ_MODELS[@]}"; do
  if [ ! -e "src/dlstreamer-pipeline-server/models/intel/\$model/\$model.json" ]; then
    python3 -m pip install openvino-dev[onnx]
    echo "Download \$model..." && \
    omz_downloader --name "\$model" --output_dir src/dlstreamer-pipeline-server/models && \

    echo "Download \$model proc file..." && \
    wget -O "src/dlstreamer-pipeline-server/models/intel/\${model}/\${model}.json" "https://github.com/dlstreamer/dlstreamer/blob/master/samples/gstreamer/model_proc/intel/\${model}.json?raw=true"

  fi
done

##############################################################################
# Download and setup videos
##############################################################################
mkdir -p src/dlstreamer-pipeline-server/videos
declare -A video_urls=(
    ["VIRAT_S_000101.mp4"]="https://github.com/intel/metro-ai-suite/raw/refs/heads/videos/videos/VIRAT_S_000101.mp4"
    ["VIRAT_S_000102.mp4"]="https://github.com/intel/metro-ai-suite/raw/refs/heads/videos/videos/VIRAT_S_000102.mp4"
    ["VIRAT_S_000103.mp4"]="https://github.com/intel/metro-ai-suite/raw/refs/heads/videos/videos/VIRAT_S_000103.mp4"
    ["VIRAT_S_000104.mp4"]="https://github.com/intel/metro-ai-suite/raw/refs/heads/videos/videos/VIRAT_S_000104.mp4"
)
for video_name in "\${!video_urls[@]}"; do
    if [ ! -f src/dlstreamer-pipeline-server/videos/\${video_name} ]; then
        echo "Download \${video_name}..."
        wget -O "src/dlstreamer-pipeline-server/videos/\${video_name}" "\${video_urls[\$video_name]}"
    fi
done

echo "Fix ownership..."
chown -R "$(id -u):$(id -g)" src/dlstreamer-pipeline-server/models src/dlstreamer-pipeline-server/videos 2>/dev/null || true
EOF

)"

