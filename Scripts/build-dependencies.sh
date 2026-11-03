#!/usr/bin/env bash
# Copyright (c) 2026 DLR e.V.
#
# This program and the accompanying materials are made
# available under the terms of the Eclipse Public License 2.0
# which is available at https://www.eclipse.org/legal/epl-2.0/
#
# SPDX-License-Identifier: EPL-2.0

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_TAG="${IMAGE_TAG:-gemstar:sumo_thirdparty}"
SUMO_REF="${SUMO_REF:-v1_27_1}"
THIRD_PARTY_DIR="$ROOT_DIR/Source/ue_sumo_featureRuntime/ThirdParty/sumo"
INCLUDE_DIR="$THIRD_PARTY_DIR/Include"
LIB_DIR="$THIRD_PARTY_DIR/Lib"

docker build --build-arg "SUMO_REF=$SUMO_REF" -t "$IMAGE_TAG" "$ROOT_DIR"

container_id="$(docker create "$IMAGE_TAG")"
cleanup() {
    docker rm -f "$container_id" >/dev/null 2>&1 || true
}
trap cleanup EXIT

mkdir -p "$INCLUDE_DIR" "$LIB_DIR"

docker cp "$container_id:/home/ue4/UnrealEngine/sumo/src/libsumo" "$INCLUDE_DIR/"
find "$INCLUDE_DIR/libsumo" -maxdepth 1 -type f ! -name '*.h' -delete
mkdir -p "$INCLUDE_DIR/libsumo/c"
docker cp "$container_id:/home/ue4/UnrealEngine/sumo/build/src/libsumo/c/libsumoC_wrap.h" "$INCLUDE_DIR/libsumo/c/libsumoC_wrap.h"
docker cp "$container_id:/home/ue4/UnrealEngine/sumo/build/src/version.h" "$INCLUDE_DIR/version.h"
docker cp "$container_id:/home/ue4/UnrealEngine/sumo/bin/libsumoc.so" "$LIB_DIR/libsumoc.so"

rm -f "$LIB_DIR/*.so*"

for dependency in \
    /usr/lib/x86_64-linux-gnu/libxerces-c-3.2.so \
    /usr/lib/x86_64-linux-gnu/libicudata.so.70.1 \
    /usr/lib/x86_64-linux-gnu/libicui18n.so.70.1 \
    /usr/lib/x86_64-linux-gnu/libicuuc.so.70.1 \
    /lib/x86_64-linux-gnu/libz.so.1.2.11; do
    docker cp "$container_id:$dependency" "$LIB_DIR/" >/dev/null 2>&1 || true
done

ln -sfn "libxerces-c-3.2.so" "$LIB_DIR/libxerces-c.so"
ln -sfn "libicudata.so.70.1" "$LIB_DIR/libicudata.so.70"
ln -sfn "libicudata.so.70.1" "$LIB_DIR/libicudata.so"
ln -sfn "libicui18n.so.70.1" "$LIB_DIR/libicui18n.so.70"
ln -sfn "libicui18n.so.70.1" "$LIB_DIR/libicui18n.so"
ln -sfn "libicuuc.so.70.1" "$LIB_DIR/libicuuc.so.70"
ln -sfn "libicuuc.so.70.1" "$LIB_DIR/libicuuc.so"
ln -sfn "libz.so.1.2.11" "$LIB_DIR/libz.so.1"
ln -sfn "libz.so.1.2.11" "$LIB_DIR/libz.so"

printf 'Built %s from SUMO %s and refreshed Linux C binding third-party artefacts.\n' "$IMAGE_TAG" "$SUMO_REF"
