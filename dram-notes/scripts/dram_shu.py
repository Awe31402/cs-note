#!/usr/bin/env python3
# Read-only sampling of the DRAM "current shuffle" (frequency set) through /dev/mem.
# mmap is PROT_READ only; this script never writes a register.
#
# Addresses come from libdram (lk/libdram-lk, src/mt8391):
#   DDRPHY_AO CHA 0x10238000 / CHB 0x10248000  (inc/mt8189/x_hal_platform.h:142, IO_PHYS 0x10000000)
#   DRAMC_AO  CHA 0x10230000 / CHB 0x10240000  (x_hal_platform.h:140)
#   DDRPHY_REG_DVFS_STATUS = DDRPHY_AO + 0xC7C  (Register_DDRPHY_AO.h:4564)
#     [22:19] SC_DR_SHU_LEVEL_SRAM_MUX_OUT  [24:23] SC_DR_SHU_LEVEL_MUX_OUT
#     [15:14] OTHER_SHU_GP  [11:8] DBG_LATCH_SHU_LEVEL_SRAM  [25] PLL_SEL
#   DRAMC_REG_SA_RESERVE = DRAMC_AO + 0x00C  (Register_DRAMC_AO.h:29)
#     [7:4] DRM_SHU_SRAM_LEVEL  [8] DRM_SHU_LEVEL
# DEVAPC (TF-A devapc/mt8189/devapc.h) gives domain 0 NO_PROTECTION / SEC_RW_NS_R
# on DRAMC_CH0/CH1_TOP0..6, so a non-secure read is allowed.
# The DPM (DRAMC_MD32, 0x10900000) is FORBIDDEN for domain 0 -> not read here.
import mmap, os, struct, sys, time

# SRAM shuffle -> data rate (Mbps), MT8189 LP5 table (lpddr_cal/dramc_pi_main.c:330-341)
SHU_RATE = {0: 6400, 1: 5500, 2: 4266, 3: 3200, 4: 2133}

CH = [("CHA", 0x10238000, 0x10230000), ("CHB", 0x10248000, 0x10240000)]


def rd(fd, addr):
    page = addr & ~0xfff
    m = mmap.mmap(fd, 0x1000, mmap.MAP_SHARED, mmap.PROT_READ, offset=page)
    v = struct.unpack_from("<I", m, addr - page)[0]
    m.close()
    return v


def sample(fd):
    out = []
    for name, phy, dramc in CH:
        st = rd(fd, phy + 0xC7C)
        sa = rd(fd, dramc + 0x00C)
        sram = (st >> 19) & 0xf
        out.append("%s DVFS_STATUS=0x%08x sram_shu=%d(%s) reg_gp=%d other_gp=%d pll_sel=%d latch=%d | SA_RESERVE=0x%08x drm_sram=%d"
                   % (name, st, sram, SHU_RATE.get(sram, "?"), (st >> 23) & 3, (st >> 14) & 3,
                      (st >> 25) & 1, (st >> 8) & 0xf, sa, (sa >> 4) & 0xf))
    return out


n = int(sys.argv[1]) if len(sys.argv) > 1 else 1
gap = float(sys.argv[2]) if len(sys.argv) > 2 else 0.0
fd = os.open("/dev/mem", os.O_RDONLY | os.O_SYNC)
for i in range(n):
    t = time.monotonic()
    for line in sample(fd):
        print("%.3f %s" % (t, line))
    sys.stdout.flush()
    if gap:
        time.sleep(gap)
os.close(fd)
