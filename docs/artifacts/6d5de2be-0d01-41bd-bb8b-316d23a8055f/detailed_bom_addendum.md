# Addendum: Detailed Bill of Materials (BOM) & Pilot Deployment

This addendum provides the granular line-item breakdown for the workstation tiers proposed in the **IT Department RoadMap 2026-2031**. These specifications are designed to eliminate OEM vendor lock-in, ensure 100% part interchangeability, and provide a "disaster-recovery-ready" infrastructure for SIA.

---

## 1.0 Detailed Build Specifications (Line-Item BOM)

### Tier A1: Architectural Powerhouse (Architects)
*Focus: Maximum GPU VRAM and Single-Core Clock Speed*

| Category | Component | Specification Detail |
| :--- | :--- | :--- |
| **Motherboard** | ASUS ProArt X870E-Creator WiFi | USB4, Dual 10G/2.5G LAN, 4x DIMM Slots |
| **Processor** | AMD Ryzen 7 9700X | 8-Core / 16-Thread, 5.5GHz Boost |
| **Cooler** | Noctua NH-D15 G2 | Universal standard mounting; non-proprietary |
| **Memory** | 32GB DDR5-6000 CL30 | 2 x 16GB Modular Sticks (Standard SKU) |
| **Graphics** | NVIDIA RTX 5080 (16GB) | GDDR7; Mandated for Lumion 2026 Ray Tracing |
| **Storage** | 500GB+ NVMe Gen5 SSD | Primary OS & Active Project Drive (Variable) |
| **Power Supply** | Seasonic Vertex GX-1200 | 1200W, 80+ Platinum, ATX 3.1 Standard |
| **Chassis** | Fractal Design North XL | Standard ATX; High Airflow; Modular Layout |

### Tier A2: Multidisciplinary calculation Node (Engineers)
*Focus: CPU Core Density and ECC Memory Precision*

| Category | Component | Specification Detail |
| :--- | :--- | :--- |
| **Motherboard** | ASUS ProArt X870E-Creator WiFi | USB4, Dual 10G/2.5G LAN, 4x DIMM Slots |
| **Processor** | AMD Ryzen 9 9950X | 16-Core / 32-Thread, 5.7GHz Boost |
| **Cooler** | Noctua NH-D15 G2 | Universal standard mounting; non-proprietary |
| **Memory** | 32GB DDR5-5600 **ECC** | 2 x 16GB Modular Sticks (Simulation Accuracy) |
| **Graphics** | NVIDIA RTX 5070 (12GB) | GDDR7; High-Bandwidth BIM Coordination |
| **Storage** | 1TB NVMe Gen5 SSD | Primary OS & Active Project Drive (Variable) |
| **Power Supply** | Seasonic Focus GX-1000 | 1000W, 80+ Gold, ATX 3.1 Standard |
| **Chassis** | Fractal Design North XL | Standard ATX; High Airflow; Modular Layout |

### Tier B1: High-Longevity Administrative (Admin/Secretaries)
*Focus: Snappy Response and 10-Year Lifecycle Sustainability*

| Category | Component | Specification Detail |
| :--- | :--- | :--- |
| **Motherboard** | ASUS Prime B850M-A WiFi | AM5 Socket (Shared with Group A); 4x DIMM |
| **Processor** | AMD Ryzen 5 9600X | 6-Core / 12-Thread, 5.1GHz Boost |
| **Memory** | 16GB DDR5-5200 | 1 x 16GB Modular Stick (Standard SKU) |
| **Graphics** | Radeon Integrated (iGPU) | Sufficient for triple-monitor 4K Office use |
| **Storage** | 500GB NVMe Gen4 SSD | DICT Baseline Compliance (Variable) |
| **Power Supply** | Corsair RM650 | 650W, 80+ Gold, ATX 3.1 Standard |
| **Chassis** | Fractal Design Pop Mini Air | Micro-ATX Standard; Efficient Footprint |

---

## 2.0 Phase 1 Pilot Deployment List (Urgent Legacy Replacements)

Based on the inventory audit provided by Paul Frichot, the following **5 units** represent the most critical legacy systems (4th to 6th Gen Intel) that fail DICT compliance and pose an immediate operational risk.

| User | Current Machine | Issue / Legacy Tier | Proposed Role |
| :--- | :--- | :--- | :--- |
| **Samuel Marie** | SIA-SMARIE | i5-4590 (4th Gen) | Group A (Technical) |
| **Sandy Willecke** | SIA-SWILLECKE | i5-4590 (4th Gen) | Group A (Technical) |
| **Veronique Sinon** | SIA-VSINON | i5-4460 (4th Gen) | Group A (Technical) |
| **Paul Frichot** | pfrichot-pc | i7-6700T (6th Gen) | Group A (Technical/IT) |
| **Joel Philo** | SIA-JPHILOE | i7-7700 (7th Gen) | Group A (Technical) |

---

## 3.0 Operational Resilience Note

> This refresh positions our build as not just a "high-spec" choice, but a **disaster-recovery-ready infrastructure** that makes the IT Department significantly faster and cheaper to run. By standardising on retail ATX and AM5 standards, we have effectively eliminated vendor lock-in and reduced our emergency spare-parts warehouse to just **four core SKUs**.
