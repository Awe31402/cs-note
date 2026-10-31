#!/usr/bin/env python3
"""Lecture 51 verification:
1) Varius spherical spatial correlation (Eq. 12.12) + SSTA Gumbel max-of-paths delay distribution
2) NBTI t_stress^0.25 degradation & partial recovery (eta = 0.35, Eq. in Sec. 12.6.1)
3) RAMP hard-error MTTF collapse from 65 C to 85 C across EM, SM, TDDB, and Thermal Cycling
"""
import numpy as np

def sim_varius_ssta(grid_n=12, phi=0.35, m_gates=16, n_paths=32, trials=5000):
    rng = np.random.default_rng(42)
    sigma_tot = 0.09  # textbook p.660: sigma_total / mu = 0.09, sigma_sys = sigma_rand
    sigma_sys = sigma_tot / np.sqrt(2.0)
    sigma_rnd = sigma_tot / np.sqrt(2.0)

    # Build 2D coordinates in [0, 1]^2 and spherical correlation matrix (Eq. 12.12)
    xs = np.linspace(0, 1, grid_n)
    pts = np.array([(x, y) for x in xs for y in xs])
    diff = pts[:, None, :] - pts[None, :, :]
    r = np.hypot(diff[:, :, 0], diff[:, :, 1])
    rho = np.where(r < phi, 1.0 - 1.5 * (r / phi) + 0.5 * (r / phi) ** 3, 0.0)
    L = np.linalg.cholesky( (sigma_sys ** 2) * rho + 1e-12 * np.eye(grid_n * grid_n) )

    # Sample systematic Vth deviation maps: shape (trials, grid_n^2)
    sys_maps = rng.standard_normal((trials, grid_n * grid_n)) @ L.T

    # Linearized gate delay sensitivity around V=1.0V, Vth0=0.25V, alpha=1.3, d_L = 0.5*d_Vth:
    # T_g / T_g0 ~ (1 + 0.5*d_sys + d_L_rnd) / (1 - (Vth0/(V-Vth0))*(d_sys + d_Vth_rnd))^alpha
    def path_delays(include_spatial_sys):
        # Place n_paths across the first n_paths cells; each path has m_gates in its local cell
        d_sys = sys_maps[:, :n_paths, None] if include_spatial_sys else 0.0
        d_vth_r = rng.normal(0.0, sigma_rnd if include_spatial_sys else sigma_tot, (trials, n_paths, m_gates))
        d_lef_r = rng.normal(0.0, 0.5 * (sigma_rnd if include_spatial_sys else sigma_tot), (trials, n_paths, m_gates))
        num = np.clip(1.0 + 0.5 * d_sys + d_lef_r, 0.2, None)
        den = np.clip(1.0 - (0.25 / 0.75) * (d_sys + d_vth_r), 0.2, None) ** 1.3
        gate_t = num / den  # normalized so ideal gate delay = 1.0, ideal 16-gate path = 16.0
        return gate_t.sum(axis=2)  # shape (trials, n_paths)

    p_uncorr = path_delays(False)
    p_varius = path_delays(True)
    return (
        (p_uncorr[:, 0].mean(), np.percentile(p_uncorr[:, 0], 99.9)),
        (p_varius[:, 0].mean(), np.percentile(p_varius[:, 0], 99.9)),
        (p_varius.max(axis=1).mean(), np.percentile(p_varius.max(axis=1), 99.9)),
    )

def calc_nbti_recovery(eta=0.35):
    # Fractional unrecovered Delta_Vth = 1 - sqrt(eta * t_rec / (t_rec + t_stress))
    ratios = [0.0, 0.5, 1.0, 10.0, 1e9]
    return [(r, 1.0 - np.sqrt(eta * (r / (1.0 + r)))) for r in ratios]

def calc_ramp_mttf_ratios(t_cool_c=65.0, t_hot_c=85.0, t_amb_c=30.0):
    k_B = 8.617333262e-5  # eV / K
    T1, T2, Tamb = t_cool_c + 273.15, t_hot_c + 273.15, t_amb_c + 273.15

    # 1) Electromigration (Eq. 12.15): Ea = 0.9 eV (at constant current density J)
    em_1, em_2 = np.exp(0.9 / (k_B * T1)), np.exp(0.9 / (k_B * T2))
    # 2) Stress Migration (Eq. 12.16): T0 = 500 K, n = 2.5, Ea = 0.9 eV
    sm_1 = (abs(500.0 - T1) ** -2.5) * np.exp(0.9 / (k_B * T1))
    sm_2 = (abs(500.0 - T2) ** -2.5) * np.exp(0.9 / (k_B * T2))
    # 3) TDDB (Eq. 12.17): V = 1.0 V, a=78, b=-0.081, X=0.759, Y=-66.8, Z=-8.37e-4
    def tddb(T):
        v_term = (1.0 / 1.05) ** (78.0 - (-0.081) * T)
        e_term = np.exp((0.759 + (-66.8) / T + (-8.37e-4) * T) / (k_B * T))
        return v_term * e_term
    td_1, td_2 = tddb(T1), tddb(T2)
    # 4) Thermal Cycling (Coffin-Manson Eq. 12.18): q = 2.35
    tc_1 = (1.0 / (T1 - Tamb)) ** 2.35
    tc_2 = (1.0 / (T2 - Tamb)) ** 2.35
    return [("Electromigration (Black)", em_1 / em_2),
            ("Stress Migration",         sm_1 / sm_2),
            ("TDDB (at V = 1.05 V)",     td_1 / td_2),
            ("Thermal Cycling (Coffin)", tc_1 / tc_2)]

if __name__ == "__main__":
    (m_u, p99_u), (m_v, p99_v), (m_g, p99_g) = sim_varius_ssta()
    print("=== 1) Varius Spatial Correlation & SSTA Gumbel Max-Path Delay (Ideal = 16.00) ===")
    print(f"  Single path (uncorrelated random only) : mean={m_u:.2f} | 99.9th pct={p99_u:.2f} (+{(p99_u/16-1)*100:.1f}% guardband)")
    print(f"  Single path (Varius sys + rand)        : mean={m_v:.2f} | 99.9th pct={p99_v:.2f} (+{(p99_v/16-1)*100:.1f}% guardband)")
    print(f"  Chip max of 32 paths (Gumbel SSTA)     : mean={m_g:.2f} | 99.9th pct={p99_g:.2f} (+{(p99_g/16-1)*100:.1f}% guardband)")

    print("\n=== 2) NBTI Stress-Recovery Remaining Degradation Fraction (eta = 0.35) ===")
    print("  " + " | ".join(f"t_rec/t_str={'inf' if r>1e6 else r}: {f*100:.1f}%" for r, f in calc_nbti_recovery()))

    print("\n=== 3) RAMP Hard-Error MTTF Lifespan Loss: 65 C -> 85 C (+20 C Hotspot, Tamb=30 C) ===")
    for name, ratio in calc_ramp_mttf_ratios():
        print(f"  {name:<26}: MTTF(65C) / MTTF(85C) = {ratio:5.2f}x shorter life at 85 C")
