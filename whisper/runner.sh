#!/bin/bash
set -eu  # エラー発生時に停止、未定義変数の検知（推奨）

mkdir -p build/run
cd build
make

echo '====run===='

ANALYZE_BIN="./analyze/analyze"
WHISPER_BIN="./whisper"
TARGET_DIR="../testdata"

for filepath in "$TARGET_DIR"/*; do
    if [ -f "$filepath" ]; then
        filename=$(basename "$filepath")
        bin_file="./run/${filename}.bin"
        wav_file="./run/${filename}.out.wav"

        if [ ! -f "$bin_file" ] || [ "$ANALYZE_BIN" -nt "$bin_file" ]; then
            "$ANALYZE_BIN" "$filepath" "$bin_file"
        else
            echo "[SKIP] $filename (.bin is up-to-date)"
        fi

        if [ ! -f "$wav_file" ] || [ "$WHISPER_BIN" -nt "$wav_file" ]; then
            "$WHISPER_BIN" "$bin_file" "$wav_file"
        else
            echo "[SKIP] $bin_file (.bin is up-to-date)"
        fi

    fi
done
