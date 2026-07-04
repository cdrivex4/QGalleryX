Title: Live Content

Description: Fetched live

Source: https://raw.githubusercontent.com/justdubit/just-dub-it/main/packages/ltx-pipelines/README.md

---

# JustDubit Pipeline

A two-stage audio-video generation pipeline for **video dubbing** tasks, built on Lightricks' LTX-2 model.

JustDubit generates synchronized audio and video from source content, enabling high-quality dubbing with natural lip movements and speech alignment.

> For other LTX-2 pipelines (text-to-video, image-to-video, keyframe interpolation), see the [main LTX-2 repository](https://github.com/Lightricks/LTX-2/blob/main/packages/ltx-pipelines/).

---

## 📋 Overview

**Key Features:**

- 🎙️ **Synchronized Audio-Video Generation**: Generates aligned audio and video for dubbing tasks
- 🎬 **Two-Stage Architecture**: Stage 1 generates at target resolution, Stage 2 upsamples 2x with refinement
- 📹 **Video Conditioning**: Condition on source video for lip-sync and motion preservation
- 🖼️ **Automatic First-Frame Extraction**: Seamlessly extracts conditioning frames from source video
- 🔧 **LoRA Support**: Easy integration with custom LoRA adapters

---

## 🚀 Quick Start

### Installation

```bash
# From the repository root
uv sync --frozen
```

### Usage


```bash
# Model paths
CHECKPOINT=/path/to/ltx-av-checkpoint.safetensors
GEMMA_ROOT=/path/to/gemma-text-encoder
SPATIAL_UPSAMPLER=/path/to/spatial-upscaler.safetensors
DISTILLED_LORA=/path/to/distilled-lora.safetensors

uv run python src/ltx_pipelines/pipeline_justdubit.py \
    --checkpoint_path ${CHECKPOINT} \
    --gemma_root ${GEMMA_ROOT} \
    --distilled_lora_path ${DISTILLED_LORA} \
    --distilled_lora_strength 1.0 \
    --spatial_upsampler_path ${SPATIAL_UPSAMPLER} \
    --lora /path/to/justdubit-lora.safetensors \
    --lora_strength 1.0 \
    --video_conditioning /path/to/source-video.mp4 1.0 \
    --prompt "The man is speaking English, saying: 'Hello, world!' " \
    --height 512 --width 768 \
    --num_inference_steps 30 \
    --cfg_guidance_scale 3.0 \
    --frame_rate 25 \
    --seed 42 \
    --output_path ./output.mp4
```

> **Resolution Note:** The `--height` and `--width` specify Stage 1 resolution. The final output is **2x upsampled** in Stage 2. For example, `512x768` input produces a `1024x1536` output video.

---

## 🎛️ CLI Arguments

| Argument | Required | Description |
|----------|----------|-------------|
| `--checkpoint_path` | ✅ | Path to LTX-2 AV model checkpoint - [Download](https://huggingface.co/Lightricks/LTX-2/resolve/main/ltx-2-19b-dev.safetensors) |
| `--gemma_root` | ✅ | Path to Gemma text encoder directory - [Download](https://huggingface.co/google/gemma-3-12b-it-qat-q4_0-unquantized/tree/main) |
| `--distilled_lora_path` | ✅ | Path to distilled LoRA for Stage 2 - [Download](https://huggingface.co/Lightricks/LTX-2/resolve/main/ltx-2-19b-distilled-lora-384.safetensors) |
| `--spatial_upsampler_path` | ✅ | Path to spatial upsampler model - [Download](https://huggingface.co/Lightricks/LTX-2/resolve/main/ltx-2-spatial-upscaler-x2-1.0.safetensors) |
| `--output_path` | ✅ | Path for output MP4 file |
| `--prompt` | ✅ | Text prompt describing desired output |
| `--video_conditioning` | ✅ | Source video path and strength (e.g., `video.mp4 1.0`) |
| `--distilled_lora_strength` | | Strength of distilled LoRA (default: 1.0) |
| `--lora` | ✅ | JustDubit LoRA path (use with `--lora_strength`) - [Download](https://huggingface.co/justdubit/justdubit/resolve/main/ltx-2-19b-ic-lora-lipdubbing.safetensors) |
| `--lora_strength` | | Strength of custom LoRA (default: 1.0) |
| `--negative_prompt` | | Negative prompt for CFG guidance |
| `--height` | | Stage 1 video height in pixels (default: 512, final output: 1024) |
| `--width` | | Stage 1 video width in pixels (default: 768, final output: 1536) |
| `--num_inference_steps` | | Number of denoising steps (default: 30) |

> **Note:** The final output resolution is **2x** the specified he

