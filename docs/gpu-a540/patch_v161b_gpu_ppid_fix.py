#!/usr/bin/env python3
# v161b: correct PMIC PPIDs. SPMI DTS: pm8998@1 and pm8005@5, but the
# battery driver's working PPIDs show PM8998 enumerated at slave 0; PM8998
# peripherals therefore use ppid 0x0NN. A PMIC4 regulator block is
# 0x40 range / 0x41 voltage-set / 0x45 mode / 0x46 enable / 0x47 power-mode.
import os

ROOT = '/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu'
p = os.path.join(ROOT, 'bsd/dev/sweet_gpu.c')
s = open(p).read()

old_defs = """#define PM8998_S1_PPID 0x114u	/* CX */
#define PM8998_S9_PPID 0x119u	/* MX */
#define PM8005_S1_PPID 0x514u	/* VDD_GFX, two-phase with S4 */
#define PM8005_S4_PPID 0x517u
#define PM8005_S2_PPID 0x515u
#define PM8005_S3_PPID 0x516u
"""
new_defs = """#define PM8998_S1_PPID 0x014u	/* CX; PM8998 enumerates at SPMI slave 0 */
#define PM8998_S9_PPID 0x02cu	/* MX */
#define PM8005_S1_PPID 0x514u	/* VDD_GFX, two-phase with S4 */
#define PM8005_S2_PPID 0x517u
#define PM8005_S3_PPID 0x51au
#define PM8005_S4_PPID 0x51du
"""
assert s.count(old_defs) == 1
s = s.replace(old_defs, new_defs)

old_body = s[s.index("\tsweet_gpu_reg(\"pm8998-s1\""):s.index("\tprintf(\"SWEET GPU: probe done\\n\");")]
new_body = """\tsweet_gpu_reg("pm8998-s1", PM8998_S1_PPID, 0x1440);
\tsweet_gpu_reg("pm8998-s1", PM8998_S1_PPID, 0x1441);
\tsweet_gpu_reg("pm8998-s1", PM8998_S1_PPID, 0x1445);
\tsweet_gpu_reg("pm8998-s1", PM8998_S1_PPID, 0x1446);
\tsweet_gpu_reg("pm8998-s9", PM8998_S9_PPID, 0x2c40);
\tsweet_gpu_reg("pm8998-s9", PM8998_S9_PPID, 0x2c41);
\tsweet_gpu_reg("pm8998-s9", PM8998_S9_PPID, 0x2c45);
\tsweet_gpu_reg("pm8998-s9", PM8998_S9_PPID, 0x2c46);
\tsweet_gpu_reg("pm8005-s1", PM8005_S1_PPID, 0x1440);
\tsweet_gpu_reg("pm8005-s1", PM8005_S1_PPID, 0x1441);
\tsweet_gpu_reg("pm8005-s1", PM8005_S1_PPID, 0x1445);
\tsweet_gpu_reg("pm8005-s1", PM8005_S1_PPID, 0x1446);
\tsweet_gpu_reg("pm8005-s1", PM8005_S1_PPID, 0x1447);
\tsweet_gpu_reg("pm8005-s2", PM8005_S2_PPID, 0x1741);
\tsweet_gpu_reg("pm8005-s2", PM8005_S2_PPID, 0x1746);
\tsweet_gpu_reg("pm8005-s3", PM8005_S3_PPID, 0x1a41);
\tsweet_gpu_reg("pm8005-s3", PM8005_S3_PPID, 0x1a46);
\tsweet_gpu_reg("pm8005-s4", PM8005_S4_PPID, 0x1d41);
\tsweet_gpu_reg("pm8005-s4", PM8005_S4_PPID, 0x1d46);
"""
s = s.replace(old_body, new_body)
open(p, 'w').write(s)
print('v161b applied')
