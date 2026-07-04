Title: Live Content

Description: Fetched live

Source: https://raw.githubusercontent.com/justdubit/just-dub-it/main/packages/ltx-pipelines/src/ltx_pipelines/pipeline_justdubit.py

---

import os
import tempfile
from collections.abc import Iterator
from dataclasses import replace

import cv2
import torch

from ltx_core.components.diffusion_steps import EulerDiffusionStep
from ltx_core.components.guiders import CFGGuider
from ltx_core.components.noisers import GaussianNoiser
from ltx_core.components.protocols import DiffusionStepProtocol
from ltx_core.components.schedulers import LTX2Scheduler
from ltx_core.conditioning import ConditioningItem, VideoConditionByKeyframeIndex
from ltx_core.loader import LTXV_LORA_COMFY_RENAMING_MAP, LoraPathStrengthAndSDOps
from ltx_core.model.audio_vae import Encoder as AudioEncoder
from ltx_core.model.audio_vae.ops import AudioProcessor
from ltx_core.model.video_vae import Encoder as VideoEncoder
from ltx_core.model.video_vae import TilingConfig
from ltx_core.tools import AudioLatentTools
from ltx_core.types import AudioLatentShape, LatentState, VideoPixelShape
from ltx_pipelines import utils
from ltx_pipelines.constants import (
    AUDIO_SAMPLE_RATE,
    DEFAULT_LORA_STRENGTH,
    STAGE_2_DISTILLED_SIGMA_VALUES,
)
from ltx_pipelines.media_io import decode_audio_from_file, encode_video, load_video_conditioning
from ltx_pipelines.model_ledger import ModelLedger
from ltx_pipelines.pipeline_utils import (
    PipelineComponents,
    denoise_audio_video,
    encode_text,
    euler_denoising_loop,
    guider_denoising_func,
    simple_denoising_func,
)
from ltx_pipelines.pipeline_utils import decode_audio as vae_decode_audio
from ltx_pipelines.pipeline_utils import decode_video as vae_decode_video


def extract_first_frame(video_path: str, output_path: str | None = None) -> str:
    """Extract the first frame from a video file and save it as an image."""
    cap = cv2.VideoCapture(video_path)
    ret, frame = cap.read()
    cap.release()

    if not ret:
        raise RuntimeError(f"Failed to read first frame from: {video_path}")

    if output_path is None:
        temp_fd, output_path = tempfile.mkstemp(suffix=".png")
        os.close(temp_fd)

    cv2.imwrite(output_path, frame)
    return output_path


class AudioConditionByKeyframeIndex(ConditioningItem):
    """Conditions audio generation on keyframe latents at a specific frame index."""

    def __init__(self, keyframes: torch.Tensor, frame_idx: int, strength: float):
        self.keyframes = keyframes
        self.frame_idx = frame_idx
        self.strength = strength

    def apply_to(
        self,
        latent_state: LatentState,
        latent_tools: AudioLatentTools,
    ) -> LatentState:
        tokens = latent_tools.patchifier.patchify(self.keyframes)
        positions = latent_tools.patchifier.get_patch_grid_bounds(
            output_shape=AudioLatentShape.from_torch_shape(self.keyframes.shape),
            device=self.keyframes.device,
        )
        if self.frame_idx != 0:
            raise NotImplementedError("AudioConditionByKeyframeIndex does not support frame_idx != 0")

        denoise_mask = torch.full(
            size=(*tokens.shape[:2], 1),
            fill_value=1.0 - self.strength,
            device=self.keyframes.device,
            dtype=self.keyframes.dtype,
        )

        return LatentState(
            latent=torch.cat([latent_state.latent, tokens], dim=1),
            denoise_mask=torch.cat([latent_state.denoise_mask, denoise_mask], dim=1),
            positions=torch.cat([latent_state.positions, positions], dim=2),
            clean_latent=torch.cat([latent_state.clean_latent, tokens], dim=1),
        )


class JustDubitPipeline:
    """
    Two-stage audio-video generation pipeline with video conditioning.

    Stage 1 generates video and audio at target resolution with CFG guidance, then
    Stage 2 upsamples by 2x and refines using a distilled LoRA for higher quality output.
    Audio is automatically extracted from video conditioning sources.
    """

    def __init__(
        self,
        checkpoint_path: str,
        distilled_lora_path: str,
        distilled_lora_strength: float,
        spatial_upsampler_path: str,
        gemma_root: str,
        loras: list[LoraPathStrengthAndSDOps],
        device: str = utils.get_device(),
        fp8transformer: bool = False,
    ):
        print("[JustDubit] Initializing pipeline...")
        self.device = device
        self.dtype = torch.bfloat16
        print("[JustDubit] Loading model ledger...")
        self.stage_1_model_ledger = ModelLedger(
            dtype=self.dtype,
            device=device,
            checkpoint_path=checkpoint_path,
            spatial_upsampler_path=spatial_upsampler_path,
            gemma_root_path=gemma_root,
            loras=loras,
            fp8transformer=fp8transformer,
        )

        self.stage_2_model_ledger = self.stage_1_model_ledger.with_loras(
            loras=[
                LoraPathStrengthAndSDOps(
                    path=distilled_lora_path,
                    strength=distilled_lora_strength,
      

