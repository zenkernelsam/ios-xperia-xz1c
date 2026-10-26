import os
p='/Users/ciscohe/Desktop/ios-xperia-xz1c/dxb/xnu/bsd/dev/sweet_gpu.c'
s=open(p).read()
if 'trust0 after reset' in s:
    print('already'); raise SystemExit(0)
old='''	gpu_wr(gpu, 0xa8d8u * 4u, 1u);			/* GPMU CM3 reset */
	gpu_wr(gpu, 0xa8e0u * 4u, 0u);			/* GPMU ready */
	gpu_wr(gpu, 0xa8e1u * 4u, 0u);			/* GPMU error */
'''
new=old+'''	/* non-secure transition: sagit bridge requires TrustControl==0. */
	gpu_wr(gpu, 0xf400u * 4u, 0u);			/* trust0 after reset */
	delay(100);
	printf("SWEET GPU: a540 trust after reset=%#x\n",
	    gpu_rd(gpu, 0xf400u * 4u));
'''
assert s.count(old)==1
open(p,'w').write(s.replace(old,new))
print('v194 applied')
