# Task Checklist

- [x] **Research & Diagnostics**
    - [x] Review `av_encoder.py` and `model_ledger.py` 
    - [x] Check mapping for `multi_modal_projector` weights
    - [x] Identify RAM bottleneck: builder scanning 48GB of shards for 9MB of data
- [x] **Implementation (Round 1 — Feature Extractor)**
    - [x] Create `GemmaFeaturesExtractorLightricks` in `feature_extractor.py`
    - [x] Update `AVGemmaTextEncoderModelConfigurator` in `av_encoder.py`
- [x] **Implementation (Round 2 — Split-Source Loading)**
    - [x] Split `AV_GEMMA_TEXT_ENCODER_KEY_OPS` into projector + connector ops in `av_encoder.py`
    - [x] Rewrite `text_encoder_builder` in `model_ledger.py` to use two separate builders
    - [x] Override `text_encoder()` to compose the two
- [/] **Verification**
    - [ ] Run `scripts/test_gemma_load.py`
    - [ ] Run `scripts/bench_hardware.py` if load succeeds
- [ ] **Polishing**
    - [ ] Robustify `start_justdubit.bat` port cleanup logic
