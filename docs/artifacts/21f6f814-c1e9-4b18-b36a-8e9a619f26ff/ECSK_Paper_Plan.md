# ECSK Theory Academic Paper — Preparation Plan

> **Source material:** [ECSK_Theory_Tests.md](file:///C:/Users/curtis/.gemini/antigravity-ide/brain/21f6f814-c1e9-4b18-b36a-8e9a619f26ff/ECSK_Theory_Tests.md)
> **Status:** Draft plan — awaiting author decisions before Phase 1 execution.

---

## 1. Skills & Competencies Required

Writing this paper seriously (i.e. to a level that could survive peer review in a journal like *Physical Review D*, *Classical and Quantum Gravity*, or *JCAP*) demands competencies across five domains. We don't need mastery of all of them ourselves — some can be supported computationally or by collaborators — but we need to know where we stand.

### 1.1 Theoretical Physics

| Skill | Why it's needed | Difficulty |
|---|---|---|
| Differential geometry on Riemann-Cartan manifolds | Derive and manipulate the field equations with torsion | ★★★★★ |
| Variational calculus (Hilbert-Palatini action) | Show that the Cartan equation is the Euler-Lagrange equation for the connection | ★★★★ |
| Dirac spinor coupling to torsion | Justify the spin-density source term $\tau_{\mu\nu}^{\lambda}$ | ★★★★ |
| Cosmological perturbation theory | Extend the standard scalar/vector/tensor decomposition to include torsion perturbations | ★★★★★ |
| Tolman-Oppenheimer-Volkoff (TOV) equations with torsion | Model compact-object interiors with spin-spin repulsion | ★★★★ |

### 1.2 Observational / Data Analysis

| Skill | Why it's needed | Difficulty |
|---|---|---|
| CMB map-making and component separation (e.g. Commander, NILC, SMICA) | Extract the primordial signal from Planck data | ★★★★ |
| Angular power spectrum estimation (TT, EE, TE, BB) | Quantify deviations from ΛCDM | ★★★ |
| Non-Gaussianity estimators (bispectrum $f_\text{NL}$, trispectrum $g_\text{NL}$) | Search for torsion-specific inflationary signatures | ★★★★ |
| X-ray / radio pulsar timing data analysis | Measure mass-radius relations and surface redshifts of neutron stars | ★★★★ |

### 1.3 Numerical / Computational

| Skill | Why it's needed | Difficulty |
|---|---|---|
| Numerical relativity (3+1 decomposition, BSSN / generalized harmonic) | Simulate core-bounce events with torsion | ★★★★★ |
| Adaptive mesh refinement (AMR) frameworks (e.g. Einstein Toolkit, Cactus) | Handle extreme gradients near bounce radius | ★★★★ |
| Boltzmann solver modification (CLASS or CAMB) | Inject torsion-modified Friedmann equations into CMB predictions | ★★★★ |
| MCMC / Bayesian parameter estimation (e.g. CosmoMC, Cobaya, emcee) | Fit torsion coupling constants against data | ★★★ |

### 1.4 Mathematical Typesetting & Writing

| Skill | Why it's needed | Difficulty |
|---|---|---|
| LaTeX (including AMS packages, REVTeX class) | Standard format for physics journals | ★★ |
| BibTeX / bibliography management | Handle 50–200+ references systematically | ★★ |
| Scientific writing conventions | Abstract → Introduction → Formalism → Results → Discussion → Conclusion | ★★ |

### 1.5 Research Infrastructure

| Skill | Why it's needed | Difficulty |
|---|---|---|
| Literature review (arXiv, INSPIRE-HEP, ADS) | Establish novelty, cite correctly, avoid duplication | ★★ |
| Version control (Git) | Track paper drafts, code, and data | ★ |
| Reproducibility packaging | Ensure results can be independently verified | ★★ |

---

## 2. Gap Analysis — What We Have vs. What We Need

Our existing [ECSK_Theory_Tests.md](file:///C:/Users/curtis/.gemini/antigravity-ide/brain/21f6f814-c1e9-4b18-b36a-8e9a619f26ff/ECSK_Theory_Tests.md) is a strong *methodological outline*. Here is what is missing to turn it into a publishable paper:

| Gap | Severity | Notes |
|---|---|---|
| **Literature review** — no citations, no positioning against prior work | 🔴 Critical | We must cite Sciama (1962), Kibble (1961), Hehl et al. (1976), Poplawski (2010–2024), Magueijo, Alexander & Marcianò, etc. |
| **Explicit derivations** — equations are stated, not derived | 🔴 Critical | A paper must show the chain: action → variation → field equations → perturbation equations → observable predictions |
| **Quantitative predictions** — no numerical values, no plots | 🔴 Critical | Need at minimum: modified CMB power spectra, predicted $f_\text{NL}$ range, modified TOV mass-radius curves, GW waveform templates |
| **Data comparison** — methodologies described but no actual data used | 🟡 Major | Planck 2018 data is publicly available; NICER mass-radius measurements exist |
| **Error / sensitivity analysis** — no discussion of degeneracies or systematics | 🟡 Major | Torsion effects could mimic other BSM physics; must address this |
| **Abstract, Introduction, Conclusion** — no narrative framing | 🟡 Major | Needed for any paper |
| **Figures and diagrams** — none present | 🟡 Major | Feynman-style diagrams for spin-torsion interaction, spacetime diagrams for bounce, power spectrum comparison plots |
| **Discussion of experimental feasibility** — which tests are near-term vs. aspirational? | 🟠 Moderate | Helps reviewers assess impact |
| **Notation table / conventions** — metric signature, index conventions not stated | 🟠 Moderate | Standard practice for GR papers |

---

## 3. Proposed Paper Structure

```
Title: "Observational Constraints on Spacetime Torsion:
        CMB, Compact Objects, and Gravitational Wave Signatures
        in Einstein-Cartan-Sciama-Kibble Theory"

Abstract

I.    Introduction
      - Motivation: why torsion?
      - Brief history of ECSK theory
      - Summary of existing observational constraints
      - Statement of purpose and novel contributions

II.   Theoretical Framework
      - Riemann-Cartan geometry and notation conventions
      - Hilbert-Palatini action and independent variation
      - Cartan equation and spin-torsion coupling
      - Effective spin-spin interaction and singularity avoidance
      - Modified Friedmann equations in ECSK cosmology

III.  CMB Observables
      - Torsion-modified inflationary dynamics
      - Predictions for angular power spectra deviations
      - Non-Gaussianity signatures (bispectrum, trispectrum)
      - B-mode polarization and tensor-to-scalar ratio

IV.   Compact Object Tests
      - Modified TOV equations with torsion
      - Predicted mass-radius relations
      - Gravitational redshift bounds
      - Comparison with NICER / X-ray observations

V.    Gravitational Wave Signatures
      - Core-bounce dynamics in ECSK
      - Waveform predictions
      - Stochastic background estimates
      - Detection prospects (LIGO/Virgo/KAGRA, Einstein Telescope, LISA)

VI.   Numerical Methods
      - 3+1 decomposition with torsion
      - Initial data and constraint satisfaction
      - Gauge and coordinate choices
      - AMR implementation and convergence tests

VII.  Results
      - CMB parameter constraints (MCMC fits to Planck data)
      - Compact object parameter constraints
      - GW signal-to-noise estimates

VIII. Discussion
      - Degeneracies with other BSM models
      - Current experimental limitations
      - Near-term vs. next-generation observational prospects

IX.   Conclusion

Appendices
      A. Full derivation of modified Friedmann equations
      B. Torsion perturbation decomposition
      C. Numerical convergence tests

References
```

---

## 4. Phased Work Plan

### Phase 1 — Foundations (Weeks 1–3)
- [x] Conduct comprehensive literature review (arXiv, INSPIRE-HEP)
- [x] Compile and organize reference library (BibTeX)
- [x] Establish notation conventions and write Section II (Theoretical Framework)
- [x] Derive the modified Friedmann equations from the ECSK action explicitly
- [x] Set up LaTeX project skeleton with REVTeX4-2 class

### Phase 2 — Analytical Predictions (Weeks 4–7)
- [ ] Derive torsion-modified inflationary perturbation equations
- [ ] Compute predicted CMB power spectrum deviations analytically
- [ ] Calculate the torsion contribution to $f_\text{NL}$ (bispectrum)
- [x] Derive modified TOV equations and solve for mass-radius curves
- [x] Produce comparison plots: ECSK vs. standard GR predictions

### Phase 3 — Numerical Work (Weeks 6–10)
- [x] Modify a Boltzmann solver (CLASS or CAMB) to include torsion terms
- [x] Run MCMC chains against Planck 2018 data for torsion coupling constraints
- [x] Implement core-bounce numerical simulation (if scope allows)
- [x] Extract gravitational waveform templates from bounce dynamics
- [x] Perform convergence and sensitivity analysis

### Phase 4 — Writing & Assembly (Weeks 9–12)
- [x] Write Sections I, III, IV, V, VII, VIII, IX
- [x] Generate all figures (power spectra, mass-radius curves, waveforms, corner plots)
- [x] Write abstract
- [x] Internal review and revision cycle
- [x] Prepare supplementary materials and data release

### Phase 5 — Submission (Week 13+)
- [ ] Select target journal (PRD, CQG, or JCAP)
- [ ] Format to journal style guide
- [ ] Upload preprint to arXiv
- [ ] Submit to journal

---

## 5. Tooling Recommendations

| Purpose | Tool | Notes |
|---|---|---|
| Paper writing | LaTeX + REVTeX4-2 | Standard for APS journals |
| References | Zotero or JabRef + BibTeX | Manage 100+ references |
| CMB Boltzmann solver | [CLASS](https://github.com/lesgourg/class_public) | More modular than CAMB for adding new physics |
| Parameter estimation | [Cobaya](https://github.com/CobayaSampler/cobaya) or [MontePython](https://github.com/brinckmann/montepython_public) | Interface with CLASS, run MCMC |
| TOV solver | Custom Python (SciPy ODE) | Relatively straightforward to code |
| Numerical relativity | [Einstein Toolkit](https://einsteintoolkit.org/) | Open-source, community-supported |
| Plotting | Matplotlib + SciencePlots style | Publication-quality figures |
| Symbolic algebra | Mathematica (xAct) or SageMath | Tensor calculus verification |
| Version control | Git + GitHub/GitLab | Track everything |

---

## 6. Open Decisions for You

> [!IMPORTANT]
> These decisions will shape the scope and feasibility of the paper. We should resolve them before starting Phase 1.

1. **Scope** — Do you want a *review-style* paper (survey existing constraints, propose new tests) or an *original-research* paper (perform new calculations/simulations, present novel results)?

2. **Numerical depth** — Full numerical relativity simulations (Sections 5–7 of the outline) are extremely ambitious. Do you want to include them, or focus on the analytical/semi-analytical predictions (CMB + compact objects) and leave NR simulations as "future work"?

3. **Data analysis** — Do you want to actually run MCMC fits against Planck data, or present the theoretical framework and leave data fitting to a follow-up paper?

4. **Collaboration** — Are you working solo, or do you have (or need) collaborators with specific expertise (e.g., numerical relativity, CMB data analysis)?

5. **Target audience** — PhD thesis chapter, standalone journal article, or both?

6. **Timeline** — Is the 13-week plan realistic for your schedule, or do we need to adjust?
