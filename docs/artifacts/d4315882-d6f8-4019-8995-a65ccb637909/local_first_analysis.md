# Local-First PyTorch CUDA Analysis

## Problem Summary

The JustDubit server crashes on startup with:
```
AssertionError: Torch not compiled with CUDA enabled
```

This occurs because the current `.venv` has PyTorch 2.9.1 (CPU-only), preventing GPU acceleration for Whisper and LTX-2.

## Local Resource Discovery

Following the **Local-First Package Management** skill, I analyzed the `torch.efu` file system search and found:

### Existing CUDA-Enabled PyTorch

**Location**: `C:\Users\curtis\anaconda3\envs\wan2gp\Lib\site-packages\`

**Version**: `torch-2.7.0+cu128` (~5.9GB installed)

**Included Components**:
- `torch` - Core library with CUDA 12.8 support
- `torchaudio-2.7.0+cu128` 
- `torchvision-0.22.0+cu128`

### Current Installation

**Location**: `C:\just-dub-it\.venv\Lib\site-packages\`

**Version**: `torch-2.9.1` (CPU-only, ~20MB)

## Recommended Strategy

> [!IMPORTANT]
> **Do NOT download a new 2-3GB PyTorch+CUDA package**. We already have a compatible CUDA installation.

### Option A: Environment Variable Symlink/Path (Preferred)

Install the exact same CUDA build from the PyTorch index using `uv`, which will leverage the existing wheels cache if uv has cached them, or download minimal deltas.

**Advantages**:
- Clean, isolated environment
- Proper dependency resolution
- No conda/venv conflicts
- uv cache may have partial data

**Disadvantages**:
- May still require some download (~2GB)

### Option B: Direct Copy from wan2gp (Fast)

Copy the torch packages directly from the conda environment to the uv venv.

**Advantages**:
- **Zero bandwidth** - all files local
- **Instant setup** - ~30 seconds vs. 10+ minutes
- Guaranteed CUDA compatibility

**Disadvantages**:
- Dependency mismatch risks (conda vs. uv)
- May need manual dependency resolution
- Less "clean" than Option A

### Option C: Use Conda Environment Directly

Run the entire JustDubit project using the `wan2gp` conda environment.

**Advantages**:
- Zero setup required
- Known working CUDA installation

**Disadvantages**:
- Conda environment may have conflicting packages
- Abandons current uv-based setup
- Less project isolation

## Recommended Action

**Hybrid Approach**: Try Option A first (uv install with CUDA index), but if download is large, fall back to Option B (local copy).

### Implementation Steps

1. **Check uv cache** for existing torch+cu128 wheels
2. **Attempt uv install** from PyTorch CUDA index
   - If download < 500MB: proceed
   - If download > 500MB: abort and use Option B
3. **If Option B**: Copy torch packages from `wan2gp` to `.venv`
4. **Verify** CUDA availability with `torch.cuda.is_available()`
5. **Test** Whisper GPU loading

## Bandwidth Savings

- **Without local-first**: ~2.5GB download (torch+torchaudio+torchvision with CUDA)
- **With local-first**: 0GB (copy) or minimal (cache hit)
- **Time savings**: ~10 minutes → 30 seconds
