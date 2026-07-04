# Technical Audit & Compliance Evidence Pack (SIA)

**Status:** CONFIDENTIAL / FOR MANAGEMENT REVIEW ONLY  
**Reference:** DICT Request ID 132684 (Windows 10 End-Of-Life Mandate)  
**Date:** March 2026

---

## 1.0 Objective Data Snapshot

Analysis of the current SIA workstation fleet against the DICT Windows 11 baseline (Minimum: i5-13th Gen / Ryzen 5 7000, 16GB RAM):

| Metric | Organisational Value | Compliance % |
| :--- | :--- | :--- |
| **Total Windows 10 Fleet** | 60 Units | 100% |
| **Direct Hardware Compliance** | 3 Units | **5%** |
| **Direct Hardware Non-Compliance** | 57 Units | **95%** |

**Conclusion:** 95% of the current SIA workstation fleet is legally and technically obsolete as of October 2025. Failure to replace these units constitutes a breach of the national ICT policy and introduces unmitigated cyber security risks to the agency's data infrastructure.

---

## 2.0 Compliance Failure Analysis (The "Tenability Gap")

Our audit identifies two primary points of failure that make a simple "software upgrade" impossible for the current fleet:

### 2.1 The Processor Threshold (DICT Standard)
DICT has mandated a baseline of **Intel i5 13th Generation** or **AMD Ryzen 5 7000 Series**. 
*   **The Problem:** The bulk of SIA’s "high-end" administrative fleet consists of i5-10th and i5-12th Gen machines. Both are functional today but fail the mandate.
*   **The Impact:** Attempting to force Windows 11 onto these unsupported chips violates licensing agreements and forfeits technical support from both Microsoft and DICT.

### 2.2 The Professional Software Threshold (Autodesk/Graphisoft)
The agency is mandated by DICT’s "War on Cracks" to use up-to-date, licensed AEC software (Revit 2026, ArchiCAD 28).
*   **GPU VRAM Limitation:** Architects require **16GB VRAM** to support Lumion 2026 Ray Tracing. Our current inventory averages <4GB VRAM.
*   **RAM Saturation:** Coordinated BIM models for SIA municipal projects currently peak at >24GB RAM usage. Our current inventory averages 8GB–16GB, leading to immediate system crashes during production.

---

## 3.0 The "Red List" (Phase 1 Urgent Replacements)

The following units represent the most extreme legacy hardware currently in use. These machines are not just non-compliant; they are a liability to project deadlines.

| Host Name | User Assigned | Current Hardware | Compliance Generation |
| :--- | :--- | :--- | :--- |
| **SIA-SMARIE** | Samuel Marie | i5-4590 | 4th Gen (Critical Failure) |
| **SIA-SWILLECKE** | Sandy Willecke | i5-4590 | 4th Gen (Critical Failure) |
| **SIA-VSINON** | Veronique Sinon | i5-4460 | 4th Gen (Critical Failure) |
| **pfrichot-pc** | Paul Frichot | i7-6700T | 6th Gen (Critical Failure) |
| **SIA-JPHILOE** | Joel Philo | i7-7700 | 7th Gen (Critical Failure) |

---

## 4.0 Closing Justification: "Disaster-Recovery-Ready"

Traditional procurement (Dell/HP) leaves SIA vulnerable to vendor lock-in and proprietary part delays. By adopting the **Universal Modular Architecture** (AM5/ATX) proposed in the Roadmap:
1.  **Downtime is eliminated:** Any failed component can be swapped with a retail part or a "loaner" from a less-critical station in minutes.
2.  **Purchasing Power is protected:** We standardise on only **four core SKUs** (CPU, 16GB RAM, ATX PSU, NVMe), allowing IT to maintain 100% emergency coverage with minimal inventory.

**This is not a proposal for "premium toys"—it is a plan for a robust, sustainable, and legally compliant infrastructure that removes the IT department as a single point of failure for the agency.**
