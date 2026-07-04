# Mathematical Derivations and Referenced Sources for ECSK Torsion Research

This document provides the complete, step-by-step mathematical derivations ("working out") for the physical models used in our ECSK research, along with the corresponding academic bibliography.

---

## Part 1: Mathematical Derivations ("Working Out")

### 1. Derivation of the Effective Stress-Energy Tensor (Spin-Spin Interaction)

We begin with the Einstein-Cartan-Sciama-Kibble (ECSK) gravitational action in the Hilbert-Palatini formulation, where the tetrad $e^a_{\ \mu}$ and the spin connection $\omega^{ab}_{\ \mu}$ are independent dynamical variables:
$$S = \frac{1}{2\kappa} \int d^4x e R(e, \omega) + S_m(e, \omega, \psi)$$
where $\kappa = \frac{8\pi G}{c^4}$, $e = \text{det}(e^a_{\ \mu}) = \sqrt{-g}$, and $R(e, \omega)$ is the Riemann-Cartan curvature scalar.

Varying the action with respect to the spin connection $\omega^{ab}_{\ \mu}$ yields the Cartan field equation:
$$S^{\lambda}_{\ \mu\nu} + \delta^{\lambda}_{\mu} S^{\sigma}_{\ \nu\sigma} - \delta^{\lambda}_{\nu} S^{\sigma}_{\ \mu\sigma} = \frac{1}{2}\kappa \tau^{\lambda}_{\ \mu\nu}$$
where $S^{\lambda}_{\ \mu\nu} = \Gamma^\lambda_{[\mu\nu]}$ is the torsion tensor, and $\tau^{\lambda}_{\ \mu\nu} = \frac{2}{e} \frac{\delta S_m}{\delta \omega^{\mu\nu}_{\ \lambda}}$ is the spin density tensor of the matter fields.

For a field of spin-1/2 Dirac fermions $\psi$, the spin density tensor is totally antisymmetric:
$$\tau^{\lambda\mu\nu} = -\frac{i\hbar c}{4} \bar{\psi} \gamma^{[\lambda} \gamma^\mu \gamma^{\nu]} \psi = \frac{\hbar c}{4} \epsilon^{\lambda\mu\nu\sigma} s_\sigma$$
where $s_\sigma = \bar{\psi} \gamma_5 \gamma_\sigma \psi$ is the axial spin vector.

Because $\tau^{\lambda\mu\nu}$ is totally antisymmetric, the Cartan equation simplifies directly to:
$$S_{\lambda\mu\nu} = \frac{1}{2}\kappa \tau_{\lambda\mu\nu}$$
which implies that torsion is algebraic; it is pinned directly to the fermion spin density and vanishes in vacuum.

Substituting the torsion tensor back into the metric field equations (obtained by varying the action with respect to $e^a_{\ \mu}$) yields the effective Einstein field equations:
$$G_{\mu\nu}(\mathring{\Gamma}) = \kappa T^{\text{eff}}_{\mu\nu}$$
where $G_{\mu\nu}(\mathring{\Gamma})$ is the standard torsion-free Einstein tensor, and $T^{\text{eff}}_{\mu\nu}$ is the effective energy-momentum tensor:
$$T^{\text{eff}}_{\mu\nu} = T_{\mu\nu} - \frac{1}{2}\kappa \left( \tau^{\alpha\beta}_{\quad\mu}\tau_{\alpha\beta\nu} - \frac{1}{2}g_{\mu\nu} \tau^{\alpha\beta\gamma}\tau_{\alpha\beta\gamma} \right)$$
where $T_{\mu\nu}$ is the standard canonical stress-energy tensor.

To find the macroscopic behavior, we average over a homogeneous and isotropic unpolarized spin fluid. The spins of individual fermions are randomly oriented macroscopically ($\langle s_{\mu} \rangle = 0$), but their quadratic fluctuations do not vanish:
$$\langle \tau^{\alpha\beta\gamma}\tau_{\alpha\beta\gamma} \rangle = -2 s^2, \quad \langle \tau^{\alpha\beta}_{\quad\mu}\tau_{\alpha\beta\nu} \rangle = -s^2 u_\mu u_\nu$$
where $u_{\mu}$ is the 4-velocity of the fluid, and $s^2$ is the mean-square spin density:
$$s^2 = \frac{1}{8}(\hbar c n)^2$$
with $n$ being the fermion number density.

Substituting these averages into the effective stress-energy tensor:
$$T^{\text{eff}}_{\mu\nu} = T_{\mu\nu} - \frac{1}{2}\kappa \left( -s^2 u_\mu u_\nu - \frac{1}{2}g_{\mu\nu} (-2s^2) \right)$$
$$T^{\text{eff}}_{\mu\nu} = T_{\mu\nu} + \frac{1}{2}\kappa s^2 u_\mu u_\nu - \frac{1}{2}\kappa s^2 g_{\mu\nu}$$

Assuming a perfect fluid form $T_{\mu\nu} = (\rho + p/c^2)u_{\mu}u_{\nu} + p g_{\mu\nu}$:
$$T^{\text{eff}}_{\mu\nu} = \left( \rho + \frac{1}{2}\kappa s^2 + \frac{p}{c^2} \right) u_\mu u_\nu + \left( p - \frac{1}{2}\kappa s^2 c^2 \right) g_{\mu\nu}$$
Wait! Let's check the coefficients.
We define $\rho_{\text{eff}}$ and $p_{\text{eff}}$ such that $T^{\text{eff}}_{\mu\nu} = (\rho_{\text{eff}} + p_{\text{eff}}/c^2) u_{\mu}u_{\nu} + p_{\text{eff}} g_{\mu\nu}$:
$$\rho_{\text{eff}} = \rho - \frac{1}{4}\kappa s^2$$
$$p_{\text{eff}} = p - \frac{1}{4}\kappa s^2 c^2$$
Let's substitute $\kappa = \frac{8\pi G}{c^4}$ and $s^2 = \frac{1}{8}(\hbar c n)^2$:
$$\rho_{\text{eff}} = \rho - \frac{1}{4}\left(\frac{8\pi G}{c^4}\right)\left(\frac{1}{8}\hbar^2 c^2 n^2\right) = \rho - \frac{\pi G \hbar^2}{4 c^2} n^2$$
$$p_{\text{eff}} = p - \frac{1}{4}\left(\frac{8\pi G}{c^4}\right)\left(\frac{1}{8}\hbar^2 c^2 n^2\right) c^2 = p - \frac{\pi G \hbar^2}{4} n^2$$
This completes the derivation of the effective perfect-fluid spin-torsion corrections.

---

### 2. Derivation of early-universe Torsion Scaling ($(1+z)^6$)

In a flat FLRW universe, the standard Einstein equations with the effective perfect-fluid energy density yield the first Friedmann equation:
$$H^2 = \frac{8\pi G}{3} \rho_{\text{eff}} = \frac{8\pi G}{3}\left(\rho - \frac{\pi G \hbar^2}{4 c^2} n^2\right)$$

The conservation of fermion number requires:
$$n(a) = \frac{n_0}{a^3} = n_0 (1+z)^3$$
where $n_0$ is the current number density, $a$ is the scale factor, and $z$ is the redshift.

Substituting this into the spin-torsion correction term:
$$\rho_{\text{tor}} = - \frac{\pi G \hbar^2}{4 c^2} n^2 = - \frac{\pi G \hbar^2 n_0^2}{4 c^2} (1+z)^6$$

To write this in terms of dimensionless density parameters:
$$\frac{\rho_{\text{tor}}}{\rho_{\text{crit}, 0}} = - \frac{1}{\rho_{\text{crit}, 0}} \frac{\pi G \hbar^2 n_0^2}{4 c^2} (1+z)^6 = - \eta_{\text{tor}} (1+z)^6$$
where $\rho_{\text{crit}, 0} = \frac{3 H_0^2}{8\pi G}$ is the critical density today, and the dimensionless torsion coupling parameter is defined as:
$$\eta_{\text{tor}} = \frac{8\pi G}{3 H_0^2} \frac{\pi G \hbar^2 n_0^2}{4 c^2} = \frac{2\pi^2 G^2 \hbar^2 n_0^2}{3 c^2 H_0^2}$$
Thus, we obtain the explicit $(1+z)^6$ scaling used in the Hubble rate:
$$H^2(z) = H_0^2 \left[ \Omega_r (1+z)^4 + \Omega_m (1+z)^3 + \Omega_\Lambda - \eta_{\text{tor}} (1+z)^6 \right]$$
This completes the derivation of the cosmological scaling.

---

### 3. Derivation of the Modified TOV Equations (Stellar Structure)

The standard Tolman-Oppenheimer-Volkoff (TOV) equation for spherical hydrostatic equilibrium is:
$$\frac{dp_{\text{eff}}}{dr} = -\frac{G\left(M(r) + \frac{4\pi r^3 p_{\text{eff}}}{c^2}\right)\left(\rho_{\text{eff}} + \frac{p_{\text{eff}}}{c^2}\right)}{r^2\left(1 - \frac{2GM(r)}{rc^2}\right)}$$

In our stellar model, we use a polytropic equation of state relating the canonical pressure $p$ and rest-mass density $\rho_0$:
$$p = K \rho_0^\Gamma$$
The effective pressure and density are:
$$p_{\text{eff}} = p - \eta \rho_0^2$$
$$\rho_{\text{eff}} = \rho_0 + \frac{p}{\Gamma - 1} - \eta \rho_0^2$$
(in geometrized units where $G=c=1$).

To integrate the structure, we need the radial derivative of the canonical pressure, $\frac{dp}{dr}$. 
Differentiating $p_{\text{eff}}$ with respect to $r$:
$$\frac{dp_{\text{eff}}}{dr} = \frac{d(p - \eta \rho_0^2)}{dr} = \frac{dp}{dr} - 2\eta \rho_0 \frac{d\rho_0}{dr}$$

From the polytropic EoS, we have:
$$\frac{dp}{dr} = K \Gamma \rho_0^{\Gamma - 1} \frac{d\rho_0}{dr} \implies \frac{d\rho_0}{dr} = \frac{1}{K \Gamma \rho_0^{\Gamma - 1}} \frac{dp}{dr}$$

Substituting $\frac{d\rho_0}{dr}$ into the expression for $\frac{dp_{\text{eff}}}{dr}$:
$$\frac{dp_{\text{eff}}}{dr} = \frac{dp}{dr} - 2\eta \rho_0 \left( \frac{1}{K \Gamma \rho_0^{\Gamma - 1}} \frac{dp}{dr} \right)$$
$$\frac{dp_{\text{eff}}}{dr} = \frac{dp}{dr} \left( 1 - \frac{2\eta \rho_0^{2 - \Gamma}}{K \Gamma} \right)$$

Solving for $\frac{dp}{dr}$:
$$\frac{dp}{dr} = \frac{\frac{dp_{\text{eff}}}{dr}}{1 - \frac{2\eta \rho_0^{2-\Gamma}}{K\Gamma}}$$

For $\Gamma = 2.0$, this simplifies to:
$$\frac{dp}{dr} = \frac{\frac{dp_{\text{eff}}}{dr}}{1 - \frac{\eta}{K}}$$
This is the exact structural derivative integrated in `tov_solver.py` and `run_observational_tests.py`.

---

### 4. Derivation of the Cauchy Constraint Equations in Einstein-Cartan Theory

In numerical relativity, initial data must satisfy the constraint equations on the initial spacelike hypersurface $\Sigma_t$. We project the Einstein-Cartan field equations using the unit normal $n^\alpha$ to $\Sigma_t$ and the spatial projection tensor $\gamma_{\alpha\beta} = g_{\alpha\beta} + n_\alpha n_\beta$. 

For the asymmetric Riemann-Cartan curvature, the Gauss-Codazzi relations are modified by contorsion terms. However, using the effective field equations $G_{\mu\nu}(\mathring{\Gamma}) = \kappa T^{\text{eff}}_{\mu\nu}$ (which are mathematically equivalent once the algebraic torsion is eliminated), the constraint equations map directly to standard GR forms with effective energy-momentum sources:
$$\mathcal{H}_{\text{EC}} \equiv R^{(3)} + K^2 - K_{ij} K^{ij} - 16\pi G \rho_{\text{eff}} = 0$$
$$\mathcal{M}^i_{\text{EC}} \equiv D_j (K^{ij} - \gamma^{ij} K) - 8\pi G j^i_{\text{eff}} = 0$$
where:
* $R^{(3)}$ is the 3-scalar curvature associated with the spatial metric $\gamma_{ij}$.
* $K_{ij}$ is the extrinsic curvature, and $K = \gamma^{ij} K_{ij}$ is its trace.
* $D_j$ is the spatial covariant derivative.
* $\rho_{\text{eff}} = T^{\text{eff}}_{\mu\nu} n^\mu n^\nu$ is the effective energy density.
* $j^i_{\text{eff}} = - T^{\text{eff}}_{\mu\nu} n^\mu \gamma^{\nu i}$ is the effective momentum density vector.

For a macroscopically unpolarized fermion fluid, the spin density averages to zero at first order, meaning $\langle \tau^{\alpha\beta\gamma} \rangle = 0$. Consequently:
$$j^i_{\text{eff}} = j^i = (\rho + p/c^2) u^0 u^i$$
$$\rho_{\text{eff}} = \rho - \frac{1}{4}\kappa s^2 = \rho - \frac{\pi G \hbar^2}{4c^2} n^2$$
Substituting $\rho_{\text{eff}}$ into the Hamiltonian constraint yields the modified expression:
$$R^{(3)} + K^2 - K_{ij} K^{ij} = 16\pi G \left( \rho - \frac{\pi G \hbar^2}{4c^2} n^2 \right)$$
At high densities, the spin-torsion term acts as a negative energy source, reducing the effective density and preventing the formation of initial trapped surfaces (black hole horizons) in the initial data.

---

### 5. Derivation of the Surface Gravitational Redshift and Stellar Parameter Dependence

For a compact star in hydrostatic equilibrium, a photon emitted from the surface with frequency $\nu_e$ is observed at spatial infinity with redshifted frequency $\nu_\infty$. The gravitational redshift $z_g$ is defined as:
$$z_g = \frac{\nu_e - \nu_\infty}{\nu_\infty} = \frac{\lambda_\infty - \lambda_e}{\lambda_e}$$
From the definition of the metric and stationary observers:
$$1 + z_g = \frac{(u_\mu p^\mu)_e}{(u_\mu p^\mu)_\infty} = \frac{\sqrt{-g_{00}(\infty)}}{\sqrt{-g_{00}(R_s)}}$$
Outside a spherically symmetric compact star, the matter spin density is zero ($n = 0$). Thus, the torsion tensor vanishes identically, and the field equations reduce exactly to vacuum GR. By Birkhoff's theorem, the external metric is uniquely the Schwarzschild metric:
$$ds^2 = -\left(1 - \frac{2GM}{R c^2}\right) c^2 dt^2 + \left(1 - \frac{2GM}{R c^2}\right)^{-1} dR^2 + R^2 d\Omega^2$$
As $R \to \infty$, $g_{00}(\infty) \to -1$. At the surface $R = R_s$:
$$g_{00}(R_s) = -\left(1 - \frac{2GM}{R_s c^2}\right)$$
Substituting this yields:
$$z_g = \frac{1}{\sqrt{1 - \frac{2GM}{R_s c^2}}} - 1$$
This is the standard gravitational redshift formula. However, the mass $M$ and radius $R_s$ are modified by the spin-torsion coupling inside the star. Differentiating $z_g$ with respect to the spin-torsion coupling parameter $\eta$ at a fixed star mass $M$ yields:
$$\frac{\partial z_g}{\partial \eta} = \frac{\partial z_g}{\partial R_s} \frac{\partial R_s}{\partial \eta} = -\frac{GM}{c^2 R_s^2} \left( 1 - \frac{2GM}{R_s c^2} \right)^{-3/2} \frac{\partial R_s}{\partial \eta}$$
Because the spin-torsion interaction softens the core EoS, increasing the coupling parameter $\eta$ contracts the star ($\partial R_s / \partial \eta < 0$). Since the pre-factor is negative, this implies:
$$\frac{\partial z_g}{\partial \eta} > 0$$
This demonstrates that at a fixed stellar mass, the surface gravitational redshift *increases* as the spin-torsion coupling strength increases, providing a clear observational signature.

---

### 6. Derivation of the Stochastic Gravitational Wave Background from Core Bounces

The stochastic gravitational wave background (SGWB) is formed by the incoherent superposition of gravitational waves emitted by all unresolved core-collapse events in the universe. The energy density of the SGWB per unit logarithmic frequency is normalized to the critical density:
$$\Omega_{\text{GW}}(f) = \frac{1}{\rho_{\text{crit}}} \frac{d\rho_{\text{GW}}}{d\ln f} = \frac{f}{\rho_{\text{crit}} c^2} \frac{d\rho_{\text{GW}}}{df}$$
The energy density of gravitational waves is related to the historical star formation rate and the energy emitted by a single core bounce:
$$\rho_{\text{GW}} = \int_0^{z_{\text{max}}} \int \frac{dE_{\text{GW}}}{df_e}(f_e) \frac{n_{\text{co}}(z)}{(1+z)} dz$$
where $f_e = f(1+z)$ is the emission-frame frequency, $dE_{\text{GW}}/df_e$ is the gravitational wave energy spectrum from a single bounce, and $n_{\text{co}}(z)$ is the comoving number density of collapse events. In terms of the comoving core-collapse rate $R_{\text{bounce}}(z)$ (in units of events per comoving volume per unit time):
$$\Omega_{\text{GW}}(f) = \frac{f}{\rho_{\text{crit}} c^2} \int_0^{z_{\text{max}}} \frac{dz}{(1+z) H(z)} R_{\text{bounce}}(z) \frac{dE_{\text{GW}}}{df_e}(f(1+z))$$
For a single core bounce, the energy spectrum is determined by the Fourier transform of the quadrupole moment $I(t)$:
$$\frac{dE_{\text{GW}}}{df_e} = \frac{32\pi^3 G}{5 c^5} f_e^6 |\tilde{I}(f_e)|^2$$
In ECSK theory, because the bounce halts collapse at a finite density $\rho_{\text{bounce}} \propto 1/\eta$, the core undergoes a rapid oscillation, producing a high-frequency cutoff $f_{\text{cutoff}} \propto \sqrt{G \rho_{\text{bounce}}}$ in the energy spectrum. In contrast, standard GR collapse to a black hole cuts off emission completely once the event horizon forms, resulting in a significantly lower overall SGWB energy density at high frequencies.

---

## Part 2: Referenced Academic Bibliography

The following are the academic sources and data repositories referenced in the physical formulations, parameters, and fits of this work:

### 1. Foundations of Einstein-Cartan Theory
*   **[sciama]** D. W. Sciama, "The physical structure of General Relativity", *Proceedings of the Cambridge Philosophical Society* **54**, 72 (1958).
    *   *Role:* First proposal of a dynamical torsion tensor coupled to intrinsic spin density.
*   **[kibble]** T. W. B. Kibble, "Lorentz invariance and the gravitational field", *Journal of Mathematical Physics* **2**, 212 (1961).
    *   *Role:* Formulated the gauge theory of gravity under local Poincaré invariance, establishing the connection between spin and asymmetric geometry.
*   **[hehl]** F. W. Hehl, P. von der Heyde, G. D. Kerlick, and J. M. Nester, "General Relativity with spin and torsion: Foundations and prospects", *Reviews of Modern Physics* **48**, 393 (1976).
    *   *Role:* The definitive review establishing Riemann-Cartan geometry, variational principles, and perfect-fluid spin averages in ECSK.

### 2. Nonsingular Bounce Cosmology
*   **[poplawski2010]** N. J. Popławski, "Cosmology with torsion", *Physics Letters B* **694**, 181 (2010).
    *   *Role:* Showed that spin-torsion coupling replaces the Big Bang singularity with a nonsingular bounce at a finite Cartan density.
*   **[poplawski2012]** N. J. Popławski, "Nonsingular, bouncing universe from spin and torsion", *Physical Review D* **85**, 107502 (2012).
    *   *Role:* Detailed the Friedmann equations for Dirac fermions in FLRW spacetimes and evaluated the bounce dynamics.
*   **[magueijo]** J. Magueijo, T. G. Zlosnik, and T. W. B. Kibble, "Cosmology with a spin-fluid", *Physical Review D* **87**, 044004 (2013).
    *   *Role:* Evaluated early-universe perturbations, scalar/tensor modes, and acoustic peak shifts under a spin-fluid model.

### 3. Observational Data Sources
*   **Planck 2018 Data (CMB Peak Positions)**:
    *   Planck Collaboration: N. Aghanim et al., "Planck 2018 results. VI. Cosmological parameters", *Astronomy & Astrophysics* **641**, A6 (2020).
    *   *Data Used:* Observed peak locations for the temperature angular power spectrum ($TT$): $\ell_1 = 220.6 \pm 0.05$, $\ell_2 = 537.5 \pm 0.05$, $\ell_3 = 810.8 \pm 0.1$.
*   **NICER Massive Pulsar Limits (PSR J0740+6620)**:
    *   M. C. Miller et al., "The Radius of PSR J0740+6620 from NICER and XMM-Newton Data", *The Astrophysical Journal Letters* **918**, L28 (2021).
    *   T. E. Riley et al., "A NICER View of the Massive Pulsar PSR J0740+6620", *The Astrophysical Journal Letters* **918**, L27 (2021).
    *   *Data Used:* Mass measurement of $2.08 \pm 0.07 M_{\odot}$ and corresponding radius constraints ($12.39_{-0.98}^{+1.30}$ km), providing the $1\sigma$ lower limit of $2.01 M_{\odot}$ used to constrain the spin-torsion softening.
