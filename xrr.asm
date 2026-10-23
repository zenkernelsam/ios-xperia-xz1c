
/root/xz1c/xbl-O82-XResetReasonDxe.pe:     file format pei-aarch64-little


Disassembly of section .text:

0000000000001000 <.text>:
    1000:	a9be4ff4 	stp	x20, x19, [sp, #-32]!
    1004:	a9017bfd 	stp	x29, x30, [sp, #16]
    1008:	910043fd 	add	x29, sp, #0x10
    100c:	aa0003f4 	mov	x20, x0
    1010:	940000e3 	bl	0x139c
    1014:	aa0003f3 	mov	x19, x0
    1018:	b7f800b3 	tbnz	x19, #63, 0x102c
    101c:	90000028 	adrp	x8, 0x5000
    1020:	aa1403e0 	mov	x0, x20
    1024:	f940b101 	ldr	x1, [x8, #352]
    1028:	940000c9 	bl	0x134c
    102c:	a9417bfd 	ldp	x29, x30, [sp, #16]
    1030:	aa1303e0 	mov	x0, x19
    1034:	a8c24ff4 	ldp	x20, x19, [sp], #32
    1038:	d65f03c0 	ret
    103c:	a9bd57f6 	stp	x22, x21, [sp, #-48]!
    1040:	a9014ff4 	stp	x20, x19, [sp, #16]
    1044:	a9027bfd 	stp	x29, x30, [sp, #32]
    1048:	910083fd 	add	x29, sp, #0x20
    104c:	d10043ff 	sub	sp, sp, #0x10
    1050:	d0000008 	adrp	x8, 0x3000
    1054:	aa0103f3 	mov	x19, x1
    1058:	aa0003f4 	mov	x20, x0
    105c:	b9420d08 	ldr	w8, [x8, #524]
    1060:	340000e8 	cbz	w8, 0x107c
    1064:	b9400a69 	ldr	w9, [x19, #8]
    1068:	6b08013f 	cmp	w9, w8
    106c:	54000082 	b.cs	0x107c  // b.hs, b.nlast
    1070:	d2f00015 	mov	x21, #0x8000000000000000    	// #-9223372036854775808
    1074:	f2800335 	movk	x21, #0x19
    1078:	14000028 	b	0x1118
    107c:	aa1403e0 	mov	x0, x20
    1080:	aa1303e1 	mov	x1, x19
    1084:	9400003e 	bl	0x117c
    1088:	d0000009 	adrp	x9, 0x3000
    108c:	3948412b 	ldrb	w11, [x9, #528]
    1090:	3400034b 	cbz	w11, 0x10f8
    1094:	9000002a 	adrp	x10, 0x5000
    1098:	90000021 	adrp	x1, 0x5000
    109c:	91024021 	add	x1, x1, #0x90
    10a0:	f940b54b 	ldr	x11, [x10, #360]
    10a4:	910023e2 	add	x2, sp, #0x8
    10a8:	aa1403e0 	mov	x0, x20
    10ac:	f9404d69 	ldr	x9, [x11, #152]
    10b0:	d63f0120 	blr	x9
    10b4:	aa0003e8 	mov	x8, x0
    10b8:	b6f80188 	tbz	x8, #63, 0x10e8
    10bc:	d0000001 	adrp	x1, 0x3000
    10c0:	91067021 	add	x1, x1, #0x19c
    10c4:	320103e0 	orr	w0, wzr, #0x80000000
    10c8:	aa0803e2 	mov	x2, x8
    10cc:	94000132 	bl	0x1594
    10d0:	d0000000 	adrp	x0, 0x3000
    10d4:	d0000002 	adrp	x2, 0x3000
    10d8:	9106f400 	add	x0, x0, #0x1bd
    10dc:	91074042 	add	x2, x2, #0x1d0
    10e0:	321f17e1 	orr	w1, wzr, #0x7e
    10e4:	9400020f 	bl	0x1920
    10e8:	f94007ec 	ldr	x12, [sp, #8]
    10ec:	9000000a 	adrp	x10, 0x1000
    10f0:	9100014a 	add	x10, x10, #0x0
    10f4:	f9002d8a 	str	x10, [x12, #88]
    10f8:	aa1403e0 	mov	x0, x20
    10fc:	aa1303e1 	mov	x1, x19
    1100:	940000a6 	bl	0x1398
    1104:	aa0003f5 	mov	x21, x0
    1108:	b6f80095 	tbz	x21, #63, 0x1118
    110c:	aa1403e0 	mov	x0, x20
    1110:	aa1303e1 	mov	x1, x19
    1114:	9400008e 	bl	0x134c
    1118:	aa1503e0 	mov	x0, x21
    111c:	d10083bf 	sub	sp, x29, #0x20
    1120:	a9427bfd 	ldp	x29, x30, [sp, #32]
    1124:	a9414ff4 	ldp	x20, x19, [sp, #16]
    1128:	a8c357f6 	ldp	x22, x21, [sp], #48
    112c:	d65f03c0 	ret
    1130:	f100001f 	cmp	x0, #0x0
    1134:	54000140 	b.eq	0x115c  // b.none
    1138:	a9bf0be3 	stp	x3, x2, [sp, #-16]!
    113c:	a9bf03e1 	stp	x1, x0, [sp, #-16]!
    1140:	a9bf23e9 	stp	x9, x8, [sp, #-16]!
    1144:	a9bf77fe 	stp	x30, x29, [sp, #-16]!
    1148:	94000006 	bl	0x1160
    114c:	a8c177fe 	ldp	x30, x29, [sp], #16
    1150:	a8c123e9 	ldp	x9, x8, [sp], #16
    1154:	a8c103e1 	ldp	x1, x0, [sp], #16
    1158:	a8c10be3 	ldp	x3, x2, [sp], #16
    115c:	17ffffb8 	b	0x103c
    1160:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    1164:	910003fd 	mov	x29, sp
    1168:	9400049f 	bl	0x23e4
    116c:	90000028 	adrp	x8, 0x5000
    1170:	f9000100 	str	x0, [x8]
    1174:	a8c17bfd 	ldp	x29, x30, [sp], #16
    1178:	d65f03c0 	ret
    117c:	a9be4ff4 	stp	x20, x19, [sp, #-32]!
    1180:	a9017bfd 	stp	x29, x30, [sp, #16]
    1184:	910043fd 	add	x29, sp, #0x10
    1188:	aa0103f3 	mov	x19, x1
    118c:	aa0003f4 	mov	x20, x0
    1190:	940002eb 	bl	0x1d3c
    1194:	aa0003e8 	mov	x8, x0
    1198:	b6f80188 	tbz	x8, #63, 0x11c8
    119c:	d0000001 	adrp	x1, 0x3000
    11a0:	91067021 	add	x1, x1, #0x19c
    11a4:	320103e0 	orr	w0, wzr, #0x80000000
    11a8:	aa0803e2 	mov	x2, x8
    11ac:	940000fa 	bl	0x1594
    11b0:	d0000000 	adrp	x0, 0x3000
    11b4:	d0000002 	adrp	x2, 0x3000
    11b8:	9107d000 	add	x0, x0, #0x1f4
    11bc:	528022e1 	mov	w1, #0x117                 	// #279
    11c0:	91074042 	add	x2, x2, #0x1d0
    11c4:	940001d7 	bl	0x1920
    11c8:	94000301 	bl	0x1dcc
    11cc:	aa0003e9 	mov	x9, x0
    11d0:	b6f80189 	tbz	x9, #63, 0x1200
    11d4:	d0000001 	adrp	x1, 0x3000
    11d8:	91067021 	add	x1, x1, #0x19c
    11dc:	320103e0 	orr	w0, wzr, #0x80000000
    11e0:	aa0903e2 	mov	x2, x9
    11e4:	940000ec 	bl	0x1594
    11e8:	d0000000 	adrp	x0, 0x3000
    11ec:	d0000002 	adrp	x2, 0x3000
    11f0:	9107d000 	add	x0, x0, #0x1f4
    11f4:	52802341 	mov	w1, #0x11a                 	// #282
    11f8:	91074042 	add	x2, x2, #0x1d0
    11fc:	940001c9 	bl	0x1920
    1200:	aa1403e0 	mov	x0, x20
    1204:	aa1303e1 	mov	x1, x19
    1208:	9400032c 	bl	0x1eb8
    120c:	aa0003ea 	mov	x10, x0
    1210:	b6f8018a 	tbz	x10, #63, 0x1240
    1214:	d0000001 	adrp	x1, 0x3000
    1218:	91067021 	add	x1, x1, #0x19c
    121c:	320103e0 	orr	w0, wzr, #0x80000000
    1220:	aa0a03e2 	mov	x2, x10
    1224:	940000dc 	bl	0x1594
    1228:	d0000000 	adrp	x0, 0x3000
    122c:	d0000002 	adrp	x2, 0x3000
    1230:	9107d000 	add	x0, x0, #0x1f4
    1234:	528023a1 	mov	w1, #0x11d                 	// #285
    1238:	91074042 	add	x2, x2, #0x1d0
    123c:	940001b9 	bl	0x1920
    1240:	aa1403e0 	mov	x0, x20
    1244:	aa1303e1 	mov	x1, x19
    1248:	9400032c 	bl	0x1ef8
    124c:	aa0003eb 	mov	x11, x0
    1250:	b6f8018b 	tbz	x11, #63, 0x1280
    1254:	d0000001 	adrp	x1, 0x3000
    1258:	91067021 	add	x1, x1, #0x19c
    125c:	320103e0 	orr	w0, wzr, #0x80000000
    1260:	aa0b03e2 	mov	x2, x11
    1264:	940000cc 	bl	0x1594
    1268:	d0000000 	adrp	x0, 0x3000
    126c:	d0000002 	adrp	x2, 0x3000
    1270:	9107d000 	add	x0, x0, #0x1f4
    1274:	52802401 	mov	w1, #0x120                 	// #288
    1278:	91074042 	add	x2, x2, #0x1d0
    127c:	940001a9 	bl	0x1920
    1280:	aa1403e0 	mov	x0, x20
    1284:	aa1303e1 	mov	x1, x19
    1288:	94000355 	bl	0x1fdc
    128c:	aa0003ec 	mov	x12, x0
    1290:	b6f8018c 	tbz	x12, #63, 0x12c0
    1294:	d0000001 	adrp	x1, 0x3000
    1298:	91067021 	add	x1, x1, #0x19c
    129c:	320103e0 	orr	w0, wzr, #0x80000000
    12a0:	aa0c03e2 	mov	x2, x12
    12a4:	940000bc 	bl	0x1594
    12a8:	d0000000 	adrp	x0, 0x3000
    12ac:	d0000002 	adrp	x2, 0x3000
    12b0:	9107d000 	add	x0, x0, #0x1f4
    12b4:	52802461 	mov	w1, #0x123                 	// #291
    12b8:	91074042 	add	x2, x2, #0x1d0
    12bc:	94000199 	bl	0x1920
    12c0:	aa1403e0 	mov	x0, x20
    12c4:	aa1303e1 	mov	x1, x19
    12c8:	940003b3 	bl	0x2194
    12cc:	aa0003ed 	mov	x13, x0
    12d0:	b6f8018d 	tbz	x13, #63, 0x1300
    12d4:	d0000001 	adrp	x1, 0x3000
    12d8:	91067021 	add	x1, x1, #0x19c
    12dc:	320103e0 	orr	w0, wzr, #0x80000000
    12e0:	aa0d03e2 	mov	x2, x13
    12e4:	940000ac 	bl	0x1594
    12e8:	d0000000 	adrp	x0, 0x3000
    12ec:	d0000002 	adrp	x2, 0x3000
    12f0:	9107d000 	add	x0, x0, #0x1f4
    12f4:	528024c1 	mov	w1, #0x126                 	// #294
    12f8:	91074042 	add	x2, x2, #0x1d0
    12fc:	94000189 	bl	0x1920
    1300:	94000410 	bl	0x2340
    1304:	aa0003ee 	mov	x14, x0
    1308:	b7f8008e 	tbnz	x14, #63, 0x1318
    130c:	a9417bfd 	ldp	x29, x30, [sp, #16]
    1310:	a8c24ff4 	ldp	x20, x19, [sp], #32
    1314:	d65f03c0 	ret
    1318:	d0000001 	adrp	x1, 0x3000
    131c:	91067021 	add	x1, x1, #0x19c
    1320:	320103e0 	orr	w0, wzr, #0x80000000
    1324:	aa0e03e2 	mov	x2, x14
    1328:	9400009b 	bl	0x1594
    132c:	a9417bfd 	ldp	x29, x30, [sp, #16]
    1330:	d0000000 	adrp	x0, 0x3000
    1334:	d0000002 	adrp	x2, 0x3000
    1338:	9107d000 	add	x0, x0, #0x1f4
    133c:	52802521 	mov	w1, #0x129                 	// #297
    1340:	91074042 	add	x2, x2, #0x1d0
    1344:	a8c24ff4 	ldp	x20, x19, [sp], #32
    1348:	14000176 	b	0x1920
    134c:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    1350:	910003fd 	mov	x29, sp
    1354:	940003f3 	bl	0x2320
    1358:	aa0003e8 	mov	x8, x0
    135c:	b7f80068 	tbnz	x8, #63, 0x1368
    1360:	a8c17bfd 	ldp	x29, x30, [sp], #16
    1364:	d65f03c0 	ret
    1368:	d0000001 	adrp	x1, 0x3000
    136c:	91067021 	add	x1, x1, #0x19c
    1370:	320103e0 	orr	w0, wzr, #0x80000000
    1374:	aa0803e2 	mov	x2, x8
    1378:	94000087 	bl	0x1594
    137c:	d0000000 	adrp	x0, 0x3000
    1380:	d0000002 	adrp	x2, 0x3000
    1384:	9107d000 	add	x0, x0, #0x1f4
    1388:	52802801 	mov	w1, #0x140                 	// #320
    138c:	91074042 	add	x2, x2, #0x1d0
    1390:	a8c17bfd 	ldp	x29, x30, [sp], #16
    1394:	14000163 	b	0x1920
    1398:	14000003 	b	0x13a4
    139c:	aa1f03e0 	mov	x0, xzr
    13a0:	d65f03c0 	ret
    13a4:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    13a8:	910003fd 	mov	x29, sp
    13ac:	d10043ff 	sub	sp, sp, #0x10
    13b0:	f90007e0 	str	x0, [sp, #8]
    13b4:	94000491 	bl	0x25f8
    13b8:	90000028 	adrp	x8, 0x5000
    13bc:	90000021 	adrp	x1, 0x5000
    13c0:	90000022 	adrp	x2, 0x5000
    13c4:	f940b509 	ldr	x9, [x8, #360]
    13c8:	91018021 	add	x1, x1, #0x60
    13cc:	91034042 	add	x2, x2, #0xd0
    13d0:	910023e0 	add	x0, sp, #0x8
    13d4:	aa1f03e3 	mov	x3, xzr
    13d8:	f940a528 	ldr	x8, [x9, #328]
    13dc:	d63f0100 	blr	x8
    13e0:	9000002a 	adrp	x10, 0x5000
    13e4:	12800008 	mov	w8, #0xffffffff            	// #-1
    13e8:	aa1f03e0 	mov	x0, xzr
    13ec:	b9015148 	str	w8, [x10, #336]
    13f0:	910003bf 	mov	sp, x29
    13f4:	a8c17bfd 	ldp	x29, x30, [sp], #16
    13f8:	d65f03c0 	ret
    13fc:	a9bd57f6 	stp	x22, x21, [sp, #-48]!
    1400:	a9014ff4 	stp	x20, x19, [sp, #16]
    1404:	a9027bfd 	stp	x29, x30, [sp, #32]
    1408:	910083fd 	add	x29, sp, #0x20
    140c:	90000035 	adrp	x21, 0x5000
    1410:	aa0003f3 	mov	x19, x0
    1414:	b94152a8 	ldr	w8, [x21, #336]
    1418:	3100051f 	cmn	w8, #0x1
    141c:	540001a1 	b.ne	0x1450  // b.any
    1420:	94000411 	bl	0x2464
    1424:	2a0003f4 	mov	w20, w0
    1428:	940003f0 	bl	0x23e8
    142c:	2a0003e8 	mov	w8, w0
    1430:	d0000001 	adrp	x1, 0x3000
    1434:	91084421 	add	x1, x1, #0x211
    1438:	321f03e0 	orr	w0, wzr, #0x2
    143c:	b90152a8 	str	w8, [x21, #336]
    1440:	2a1403e2 	mov	w2, w20
    1444:	2a0803e3 	mov	w3, w8
    1448:	94000053 	bl	0x1594
    144c:	b94152a8 	ldr	w8, [x21, #336]
    1450:	b9000268 	str	w8, [x19]
    1454:	aa1f03e0 	mov	x0, xzr
    1458:	a9427bfd 	ldp	x29, x30, [sp, #32]
    145c:	a9414ff4 	ldp	x20, x19, [sp, #16]
    1460:	a8c357f6 	ldp	x22, x21, [sp], #48
    1464:	d65f03c0 	ret
    1468:	a9be4ff4 	stp	x20, x19, [sp, #-32]!
    146c:	a9017bfd 	stp	x29, x30, [sp, #16]
    1470:	910043fd 	add	x29, sp, #0x10
    1474:	940003ed 	bl	0x2428
    1478:	2a0003f3 	mov	w19, w0
    147c:	94000410 	bl	0x24bc
    1480:	90000028 	adrp	x8, 0x5000
    1484:	d0000001 	adrp	x1, 0x3000
    1488:	9108bc21 	add	x1, x1, #0x22f
    148c:	b9415102 	ldr	w2, [x8, #336]
    1490:	321f03e0 	orr	w0, wzr, #0x2
    1494:	2a1303e3 	mov	w3, w19
    1498:	9400003f 	bl	0x1594
    149c:	a9417bfd 	ldp	x29, x30, [sp, #16]
    14a0:	aa1f03e0 	mov	x0, xzr
    14a4:	a8c24ff4 	ldp	x20, x19, [sp], #32
    14a8:	d65f03c0 	ret
    14ac:	36000080 	tbz	w0, #0, 0x14bc
    14b0:	90000028 	adrp	x8, 0x5000
    14b4:	12800009 	mov	w9, #0xffffffff            	// #-1
    14b8:	b9015109 	str	w9, [x8, #336]
    14bc:	36080100 	tbz	w0, #1, 0x14dc
    14c0:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    14c4:	910003fd 	mov	x29, sp
    14c8:	52b575a0 	mov	w0, #0xabad0000            	// #-1414725632
    14cc:	729757c0 	movk	w0, #0xbabe
    14d0:	940003d6 	bl	0x2428
    14d4:	9400040f 	bl	0x2510
    14d8:	a8c17bfd 	ldp	x29, x30, [sp], #16
    14dc:	aa1f03e0 	mov	x0, xzr
    14e0:	d65f03c0 	ret
    14e4:	a9be4ff4 	stp	x20, x19, [sp, #-32]!
    14e8:	a9017bfd 	stp	x29, x30, [sp, #16]
    14ec:	910043fd 	add	x29, sp, #0x10
    14f0:	aa0003f3 	mov	x19, x0
    14f4:	94000409 	bl	0x2518
    14f8:	2a0003e8 	mov	w8, w0
    14fc:	aa1f03e0 	mov	x0, xzr
    1500:	39000268 	strb	w8, [x19]
    1504:	a9417bfd 	ldp	x29, x30, [sp, #16]
    1508:	a8c24ff4 	ldp	x20, x19, [sp], #32
    150c:	d65f03c0 	ret
    1510:	a9be4ff4 	stp	x20, x19, [sp, #-32]!
    1514:	a9017bfd 	stp	x29, x30, [sp, #16]
    1518:	910043fd 	add	x29, sp, #0x10
    151c:	aa0003f3 	mov	x19, x0
    1520:	94000413 	bl	0x256c
    1524:	2a0003e8 	mov	w8, w0
    1528:	aa1f03e0 	mov	x0, xzr
    152c:	39000268 	strb	w8, [x19]
    1530:	a9417bfd 	ldp	x29, x30, [sp, #16]
    1534:	a8c24ff4 	ldp	x20, x19, [sp], #32
    1538:	d65f03c0 	ret
    153c:	a9be4ff4 	stp	x20, x19, [sp, #-32]!
    1540:	a9017bfd 	stp	x29, x30, [sp, #16]
    1544:	910043fd 	add	x29, sp, #0x10
    1548:	aa0003f3 	mov	x19, x0
    154c:	9400041a 	bl	0x25b4
    1550:	2a0003e8 	mov	w8, w0
    1554:	aa1f03e0 	mov	x0, xzr
    1558:	39000268 	strb	w8, [x19]
    155c:	a9417bfd 	ldp	x29, x30, [sp, #16]
    1560:	a8c24ff4 	ldp	x20, x19, [sp], #32
    1564:	d65f03c0 	ret
    1568:	a9be4ff4 	stp	x20, x19, [sp, #-32]!
    156c:	a9017bfd 	stp	x29, x30, [sp, #16]
    1570:	910043fd 	add	x29, sp, #0x10
    1574:	aa0003f3 	mov	x19, x0
    1578:	94000418 	bl	0x25d8
    157c:	2a0003e8 	mov	w8, w0
    1580:	aa1f03e0 	mov	x0, xzr
    1584:	39000268 	strb	w8, [x19]
    1588:	a9417bfd 	ldp	x29, x30, [sp, #16]
    158c:	a8c24ff4 	ldp	x20, x19, [sp], #32
    1590:	d65f03c0 	ret
    1594:	a9bb6ffc 	stp	x28, x27, [sp, #-80]!
    1598:	a9015ff8 	stp	x24, x23, [sp, #16]
    159c:	a90257f6 	stp	x22, x21, [sp, #32]
    15a0:	a9034ff4 	stp	x20, x19, [sp, #48]
    15a4:	a9047bfd 	stp	x29, x30, [sp, #64]
    15a8:	910103fd 	add	x29, sp, #0x40
    15ac:	d10683ff 	sub	sp, sp, #0x1a0
    15b0:	aa0103f5 	mov	x21, x1
    15b4:	ad031fe6 	stp	q6, q7, [sp, #96]
    15b8:	aa0003f7 	mov	x23, x0
    15bc:	ad0217e4 	stp	q4, q5, [sp, #64]
    15c0:	ad010fe2 	stp	q2, q3, [sp, #32]
    15c4:	ad0007e0 	stp	q0, q1, [sp]
    15c8:	a90a1fe6 	stp	x6, x7, [sp, #160]
    15cc:	a90917e4 	stp	x4, x5, [sp, #144]
    15d0:	a9080fe2 	stp	x2, x3, [sp, #128]
    15d4:	b4001615 	cbz	x21, 0x1894
    15d8:	9400030a 	bl	0x2200
    15dc:	2a0003ea 	mov	w10, w0
    15e0:	8a17014b 	and	x11, x10, x23
    15e4:	b400182b 	cbz	x11, 0x18e8
    15e8:	aa1503e0 	mov	x0, x21
    15ec:	940001aa 	bl	0x1c94
    15f0:	9101a013 	add	x19, x0, #0x68
    15f4:	f103427f 	cmp	x19, #0xd0
    15f8:	54001788 	b.hi	0x18e8  // b.pmore
    15fc:	910343e8 	add	x8, sp, #0xd0
    1600:	b900d7f7 	str	w23, [sp, #212]
    1604:	9101a116 	add	x22, x8, #0x68
    1608:	52800d01 	mov	w1, #0x68                  	// #104
    160c:	aa1603e0 	mov	x0, x22
    1610:	aa1503e2 	mov	x2, x21
    1614:	aa0103e3 	mov	x3, x1
    1618:	b27e0114 	orr	x20, x8, #0x4
    161c:	91002118 	add	x24, x8, #0x8
    1620:	9400013b 	bl	0x1b0c
    1624:	910003ea 	mov	x10, sp
    1628:	910203eb 	add	x11, sp, #0x80
    162c:	321963e8 	orr	w8, wzr, #0xffffff80
    1630:	128005e9 	mov	w9, #0xffffffd0            	// #-48
    1634:	91020146 	add	x6, x10, #0x80
    1638:	9100c167 	add	x7, x11, #0x30
    163c:	910043aa 	add	x10, x29, #0x10
    1640:	291923e9 	stp	w9, w8, [sp, #200]
    1644:	320003e8 	orr	w8, wzr, #0x1
    1648:	a90b9be7 	stp	x7, x6, [sp, #184]
    164c:	d2cd0029 	mov	x9, #0x680100000000        	// #114353504256000
    1650:	f9005bea 	str	x10, [sp, #176]
    1654:	d000000a 	adrp	x10, 0x3000
    1658:	9109414a 	add	x10, x10, #0x250
    165c:	14000002 	b	0x1664
    1660:	910006b5 	add	x21, x21, #0x1
    1664:	394002ab 	ldrb	w11, [x21]
    1668:	7100957f 	cmp	w11, #0x25
    166c:	54000060 	b.eq	0x1678  // b.none
    1670:	35ffff8b 	cbnz	w11, 0x1660
    1674:	1400008f 	b	0x18b0
    1678:	2a1f03ee 	mov	w14, wzr
    167c:	14000004 	b	0x168c
    1680:	f9400230 	ldr	x16, [x17]
    1684:	2a0b03ee 	mov	w14, w11
    1688:	f8008710 	str	x16, [x24], #8
    168c:	aa1503ec 	mov	x12, x21
    1690:	38401ead 	ldrb	w13, [x21, #1]!
    1694:	2a0e03eb 	mov	w11, w14
    1698:	7100b9bf 	cmp	w13, #0x2e
    169c:	540000a8 	b.hi	0x16b0  // b.pmore
    16a0:	9acd210e 	lsl	x14, x8, x13
    16a4:	8a0901cf 	and	x15, x14, x9
    16a8:	2a0b03ee 	mov	w14, w11
    16ac:	b5ffff0f 	cbnz	x15, 0x168c
    16b0:	5100c1ae 	sub	w14, w13, #0x30
    16b4:	710029df 	cmp	w14, #0xa
    16b8:	2a0b03ee 	mov	w14, w11
    16bc:	54fffe83 	b.cc	0x168c  // b.lo, b.ul, b.last
    16c0:	71012dbf 	cmp	w13, #0x4b
    16c4:	540001ac 	b.gt	0x16f8
    16c8:	7100a9bf 	cmp	w13, #0x2a
    16cc:	54000281 	b.ne	0x171c  // b.any
    16d0:	b980cbee 	ldrsw	x14, [sp, #200]
    16d4:	36f800ae 	tbz	w14, #31, 0x16e8
    16d8:	110021cc 	add	w12, w14, #0x8
    16dc:	7100019f 	cmp	w12, #0x0
    16e0:	b900cbec 	str	w12, [sp, #200]
    16e4:	5400016d 	b.le	0x1710
    16e8:	f9405bf1 	ldr	x17, [sp, #176]
    16ec:	9100222d 	add	x13, x17, #0x8
    16f0:	f9005bed 	str	x13, [sp, #176]
    16f4:	17ffffe3 	b	0x1680
    16f8:	320003ee 	orr	w14, wzr, #0x1
    16fc:	710131bf 	cmp	w13, #0x4c
    1700:	54fffc60 	b.eq	0x168c  // b.none
    1704:	7101b1bf 	cmp	w13, #0x6c
    1708:	54fffc20 	b.eq	0x168c  // b.none
    170c:	14000007 	b	0x1728
    1710:	f9405fed 	ldr	x13, [sp, #184]
    1714:	8b0e01b1 	add	x17, x13, x14
    1718:	17ffffda 	b	0x1680
    171c:	3500006d 	cbnz	w13, 0x1728
    1720:	aa0c03f5 	mov	x21, x12
    1724:	14000002 	b	0x172c
    1728:	91000595 	add	x21, x12, #0x1
    172c:	394002ad 	ldrb	w13, [x21]
    1730:	510185a5 	sub	w5, w13, #0x61
    1734:	71005cbf 	cmp	w5, #0x17
    1738:	540001e8 	b.hi	0x1774  // b.pmore
    173c:	f8657951 	ldr	x17, [x10, x5, lsl #3]
    1740:	d61f0220 	br	x17
    1744:	b980cbef 	ldrsw	x15, [sp, #200]
    1748:	36f800af 	tbz	w15, #31, 0x175c
    174c:	110021e1 	add	w1, w15, #0x8
    1750:	7100003f 	cmp	w1, #0x0
    1754:	b900cbe1 	str	w1, [sp, #200]
    1758:	5400092d 	b.le	0x187c
    175c:	f9405be3 	ldr	x3, [sp, #176]
    1760:	9100206f 	add	x15, x3, #0x8
    1764:	f9005bef 	str	x15, [sp, #176]
    1768:	f9400062 	ldr	x2, [x3]
    176c:	f8008702 	str	x2, [x24], #8
    1770:	14000037 	b	0x184c
    1774:	71014dbf 	cmp	w13, #0x53
    1778:	54000260 	b.eq	0x17c4  // b.none
    177c:	710161bf 	cmp	w13, #0x58
    1780:	54000661 	b.ne	0x184c  // b.any
    1784:	b980cbec 	ldrsw	x12, [sp, #200]
    1788:	12001d6f 	and	w15, w11, #0xff
    178c:	3500034f 	cbnz	w15, 0x17f4
    1790:	7101c1bf 	cmp	w13, #0x70
    1794:	54000300 	b.eq	0x17f4  // b.none
    1798:	36f800ac 	tbz	w12, #31, 0x17ac
    179c:	11002191 	add	w17, w12, #0x8
    17a0:	7100023f 	cmp	w17, #0x0
    17a4:	b900cbf1 	str	w17, [sp, #200]
    17a8:	540005ed 	b.le	0x1864
    17ac:	f9405be1 	ldr	x1, [sp, #176]
    17b0:	9100202e 	add	x14, x1, #0x8
    17b4:	f9005bee 	str	x14, [sp, #176]
    17b8:	f9400025 	ldr	x5, [x1]
    17bc:	b8008705 	str	w5, [x24], #8
    17c0:	14000023 	b	0x184c
    17c4:	b980cbeb 	ldrsw	x11, [sp, #200]
    17c8:	36f800ab 	tbz	w11, #31, 0x17dc
    17cc:	11002160 	add	w0, w11, #0x8
    17d0:	7100001f 	cmp	w0, #0x0
    17d4:	b900cbe0 	str	w0, [sp, #200]
    17d8:	5400040d 	b.le	0x1858
    17dc:	f9405be2 	ldr	x2, [sp, #176]
    17e0:	9100204b 	add	x11, x2, #0x8
    17e4:	f9005beb 	str	x11, [sp, #176]
    17e8:	f9400040 	ldr	x0, [x2]
    17ec:	f8008700 	str	x0, [x24], #8
    17f0:	14000017 	b	0x184c
    17f4:	36f800ac 	tbz	w12, #31, 0x1808
    17f8:	11002190 	add	w16, w12, #0x8
    17fc:	7100021f 	cmp	w16, #0x0
    1800:	b900cbf0 	str	w16, [sp, #200]
    1804:	5400036d 	b.le	0x1870
    1808:	f9405be0 	ldr	x0, [sp, #176]
    180c:	9100200c 	add	x12, x0, #0x8
    1810:	f9005bec 	str	x12, [sp, #176]
    1814:	f9400007 	ldr	x7, [x0]
    1818:	f8008707 	str	x7, [x24], #8
    181c:	1400000c 	b	0x184c
    1820:	b980cbf0 	ldrsw	x16, [sp, #200]
    1824:	36f800b0 	tbz	w16, #31, 0x1838
    1828:	11002202 	add	w2, w16, #0x8
    182c:	7100005f 	cmp	w2, #0x0
    1830:	b900cbe2 	str	w2, [sp, #200]
    1834:	540002ad 	b.le	0x1888
    1838:	f9405be4 	ldr	x4, [sp, #176]
    183c:	91002090 	add	x16, x4, #0x8
    1840:	f9005bf0 	str	x16, [sp, #176]
    1844:	f940008b 	ldr	x11, [x4]
    1848:	f800870b 	str	x11, [x24], #8
    184c:	eb16031f 	cmp	x24, x22
    1850:	54fff089 	b.ls	0x1660  // b.plast
    1854:	1400002c 	b	0x1904
    1858:	f9405fe3 	ldr	x3, [sp, #184]
    185c:	8b0b0062 	add	x2, x3, x11
    1860:	17ffffe2 	b	0x17e8
    1864:	f9405fe6 	ldr	x6, [sp, #184]
    1868:	8b0c00c1 	add	x1, x6, x12
    186c:	17ffffd3 	b	0x17b8
    1870:	f9405ff7 	ldr	x23, [sp, #184]
    1874:	8b0c02e0 	add	x0, x23, x12
    1878:	17ffffe7 	b	0x1814
    187c:	f9405fe1 	ldr	x1, [sp, #184]
    1880:	8b0f0023 	add	x3, x1, x15
    1884:	17ffffb9 	b	0x1768
    1888:	f9405fe4 	ldr	x4, [sp, #184]
    188c:	8b100084 	add	x4, x4, x16
    1890:	17ffffed 	b	0x1844
    1894:	d0000000 	adrp	x0, 0x3000
    1898:	d0000002 	adrp	x2, 0x3000
    189c:	910c4000 	add	x0, x0, #0x310
    18a0:	528008e1 	mov	w1, #0x47                  	// #71
    18a4:	910c6c42 	add	x2, x2, #0x31b
    18a8:	9400001e 	bl	0x1920
    18ac:	14000000 	b	0x18ac
    18b0:	94000413 	bl	0x28fc
    18b4:	94000416 	bl	0x290c
    18b8:	94000419 	bl	0x291c
    18bc:	12001c08 	and	w8, w0, #0xff
    18c0:	34000148 	cbz	w8, 0x18e8
    18c4:	90000024 	adrp	x4, 0x5000
    18c8:	2a1f03e2 	mov	w2, wzr
    18cc:	aa1f03e3 	mov	x3, xzr
    18d0:	52a060a1 	mov	w1, #0x3050000             	// #50659328
    18d4:	9100c084 	add	x4, x4, #0x30
    18d8:	320007e0 	orr	w0, wzr, #0x3
    18dc:	aa1403e5 	mov	x5, x20
    18e0:	aa1303e6 	mov	x6, x19
    18e4:	94000412 	bl	0x292c
    18e8:	d10103bf 	sub	sp, x29, #0x40
    18ec:	a9447bfd 	ldp	x29, x30, [sp, #64]
    18f0:	a9434ff4 	ldp	x20, x19, [sp, #48]
    18f4:	a94257f6 	ldp	x22, x21, [sp, #32]
    18f8:	a9415ff8 	ldp	x24, x23, [sp, #16]
    18fc:	a8c56ffc 	ldp	x28, x27, [sp], #80
    1900:	d65f03c0 	ret
    1904:	d0000000 	adrp	x0, 0x3000
    1908:	d0000002 	adrp	x2, 0x3000
    190c:	910c4000 	add	x0, x0, #0x310
    1910:	52801a21 	mov	w1, #0xd1                  	// #209
    1914:	910cc842 	add	x2, x2, #0x332
    1918:	94000002 	bl	0x1920
    191c:	14000000 	b	0x191c
    1920:	a9ba6ffc 	stp	x28, x27, [sp, #-96]!
    1924:	a90167fa 	stp	x26, x25, [sp, #16]
    1928:	a9025ff8 	stp	x24, x23, [sp, #32]
    192c:	a90357f6 	stp	x22, x21, [sp, #48]
    1930:	a9044ff4 	stp	x20, x19, [sp, #64]
    1934:	a9057bfd 	stp	x29, x30, [sp, #80]
    1938:	910143fd 	add	x29, sp, #0x50
    193c:	d10343ff 	sub	sp, sp, #0xd0
    1940:	9000003c 	adrp	x28, 0x5000
    1944:	aa0003f6 	mov	x22, x0
    1948:	aa0203f4 	mov	x20, x2
    194c:	f9400f88 	ldr	x8, [x28, #24]
    1950:	aa0103f8 	mov	x24, x1
    1954:	aa0803e0 	mov	x0, x8
    1958:	940000cf 	bl	0x1c94
    195c:	aa0003fa 	mov	x26, x0
    1960:	aa1603e0 	mov	x0, x22
    1964:	940000cc 	bl	0x1c94
    1968:	aa0003f3 	mov	x19, x0
    196c:	aa1403e0 	mov	x0, x20
    1970:	940000c9 	bl	0x1c94
    1974:	aa0003f5 	mov	x21, x0
    1978:	8b130349 	add	x9, x26, x19
    197c:	8b150129 	add	x9, x9, x21
    1980:	9100a928 	add	x8, x9, #0x2a
    1984:	f103251f 	cmp	x8, #0xc9
    1988:	54000223 	b.cc	0x19cc  // b.lo, b.ul, b.last
    198c:	9100a268 	add	x8, x19, #0x28
    1990:	8b15010a 	add	x10, x8, x21
    1994:	f103255f 	cmp	x10, #0xc9
    1998:	54000103 	b.cc	0x19b8  // b.lo, b.ul, b.last
    199c:	5280140c 	mov	w12, #0xa0                  	// #160
    19a0:	f103211f 	cmp	x8, #0xc8
    19a4:	528013ee 	mov	w14, #0x9f                  	// #159
    19a8:	cb13018f 	sub	x15, x12, x19
    19ac:	320003f0 	orr	w16, wzr, #0x1
    19b0:	9a8e3273 	csel	x19, x19, x14, cc  // cc = lo, ul, last
    19b4:	9a9031f5 	csel	x21, x15, x16, cc  // cc = lo, ul, last
    19b8:	910023f7 	add	x23, sp, #0x8
    19bc:	b9001ff8 	str	w24, [sp, #28]
    19c0:	aa1f03fb 	mov	x27, xzr
    19c4:	9100a2f9 	add	x25, x23, #0x28
    19c8:	14000017 	b	0x1a24
    19cc:	910023f7 	add	x23, sp, #0x8
    19d0:	b9001ff8 	str	w24, [sp, #28]
    19d4:	91000b5b 	add	x27, x26, #0x2
    19d8:	9100a2f9 	add	x25, x23, #0x28
    19dc:	b1000b5f 	cmn	x26, #0x2
    19e0:	54000220 	b.eq	0x1a24  // b.none
    19e4:	d0000001 	adrp	x1, 0x3000
    19e8:	910d6821 	add	x1, x1, #0x35a
    19ec:	320003e2 	orr	w2, wzr, #0x1
    19f0:	aa1903e0 	mov	x0, x25
    19f4:	910023f7 	add	x23, sp, #0x8
    19f8:	94000469 	bl	0x2b9c
    19fc:	f9400f81 	ldr	x1, [x28, #24]
    1a00:	91000720 	add	x0, x25, #0x1
    1a04:	d1000742 	sub	x2, x26, #0x1
    1a08:	94000465 	bl	0x2b9c
    1a0c:	8b1b032b 	add	x11, x25, x27
    1a10:	d0000001 	adrp	x1, 0x3000
    1a14:	d1000960 	sub	x0, x11, #0x2
    1a18:	910d7021 	add	x1, x1, #0x35c
    1a1c:	321f03e2 	orr	w2, wzr, #0x2
    1a20:	9400045f 	bl	0x2b9c
    1a24:	8b1b0320 	add	x0, x25, x27
    1a28:	aa1603e1 	mov	x1, x22
    1a2c:	aa1303e2 	mov	x2, x19
    1a30:	9400045b 	bl	0x2b9c
    1a34:	8b130000 	add	x0, x0, x19
    1a38:	aa1403e1 	mov	x1, x20
    1a3c:	aa1503e2 	mov	x2, x21
    1a40:	381ff01f 	sturb	wzr, [x0, #-1]
    1a44:	94000456 	bl	0x2b9c
    1a48:	8b0002ac 	add	x12, x21, x0
    1a4c:	381ff19f 	sturb	wzr, [x12, #-1]
    1a50:	940003ab 	bl	0x28fc
    1a54:	940003ae 	bl	0x290c
    1a58:	12001c08 	and	w8, w0, #0xff
    1a5c:	340001c8 	cbz	w8, 0x1a94
    1a60:	8b1302a2 	add	x2, x21, x19
    1a64:	8b1b004d 	add	x13, x2, x27
    1a68:	52b20000 	mov	w0, #0x90000000            	// #-1879048192
    1a6c:	52a060a1 	mov	w1, #0x3050000             	// #50659328
    1a70:	9100a1a6 	add	x6, x13, #0x28
    1a74:	2a1f03e2 	mov	w2, wzr
    1a78:	aa1f03e3 	mov	x3, xzr
    1a7c:	aa1f03e4 	mov	x4, xzr
    1a80:	72800040 	movk	w0, #0x2
    1a84:	728000e1 	movk	w1, #0x7
    1a88:	aa1703e5 	mov	x5, x23
    1a8c:	940003a8 	bl	0x292c
    1a90:	14000002 	b	0x1a98
    1a94:	940003a2 	bl	0x291c
    1a98:	9000002e 	adrp	x14, 0x5000
    1a9c:	321f03e0 	orr	w0, wzr, #0x2
    1aa0:	aa1f03e1 	mov	x1, xzr
    1aa4:	f940b9cf 	ldr	x15, [x14, #368]
    1aa8:	aa1f03e2 	mov	x2, xzr
    1aac:	aa1f03e3 	mov	x3, xzr
    1ab0:	f94035e4 	ldr	x4, [x15, #104]
    1ab4:	d63f0080 	blr	x4
    1ab8:	d0000010 	adrp	x16, 0x3000
    1abc:	3947fe08 	ldrb	w8, [x16, #511]
    1ac0:	37200088 	tbnz	w8, #4, 0x1ad0
    1ac4:	36280088 	tbz	w8, #5, 0x1ad4
    1ac8:	9400000b 	bl	0x1af4
    1acc:	14000002 	b	0x1ad4
    1ad0:	94000099 	bl	0x1d34
    1ad4:	d10143bf 	sub	sp, x29, #0x50
    1ad8:	a9457bfd 	ldp	x29, x30, [sp, #80]
    1adc:	a9444ff4 	ldp	x20, x19, [sp, #64]
    1ae0:	a94357f6 	ldp	x22, x21, [sp, #48]
    1ae4:	a9425ff8 	ldp	x24, x23, [sp, #32]
    1ae8:	a94167fa 	ldp	x26, x25, [sp, #16]
    1aec:	a8c66ffc 	ldp	x28, x27, [sp], #96
    1af0:	d65f03c0 	ret
    1af4:	d10043ff 	sub	sp, sp, #0x10
    1af8:	f90007ff 	str	xzr, [sp, #8]
    1afc:	f94007e8 	ldr	x8, [sp, #8]
    1b00:	b4ffffe8 	cbz	x8, 0x1afc
    1b04:	910043ff 	add	sp, sp, #0x10
    1b08:	d65f03c0 	ret
    1b0c:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    1b10:	910003fd 	mov	x29, sp
    1b14:	aa0003e8 	mov	x8, x0
    1b18:	b40001c8 	cbz	x8, 0x1b50
    1b1c:	b4000282 	cbz	x2, 0x1b6c
    1b20:	d000000b 	adrp	x11, 0x3000
    1b24:	b9420169 	ldr	w9, [x11, #512]
    1b28:	34000429 	cbz	w9, 0x1bac
    1b2c:	eb01013f 	cmp	x9, x1
    1b30:	540002c2 	b.cs	0x1b88  // b.hs, b.nlast
    1b34:	d0000000 	adrp	x0, 0x3000
    1b38:	d0000002 	adrp	x2, 0x3000
    1b3c:	910dd800 	add	x0, x0, #0x376
    1b40:	528054a1 	mov	w1, #0x2a5                 	// #677
    1b44:	91152042 	add	x2, x2, #0x548
    1b48:	97ffff76 	bl	0x1920
    1b4c:	14000000 	b	0x1b4c
    1b50:	d0000000 	adrp	x0, 0x3000
    1b54:	d0000002 	adrp	x2, 0x3000
    1b58:	910dd800 	add	x0, x0, #0x376
    1b5c:	528053c1 	mov	w1, #0x29e                 	// #670
    1b60:	910fc442 	add	x2, x2, #0x3f1
    1b64:	97ffff6f 	bl	0x1920
    1b68:	14000000 	b	0x1b68
    1b6c:	d0000000 	adrp	x0, 0x3000
    1b70:	d0000002 	adrp	x2, 0x3000
    1b74:	910dd800 	add	x0, x0, #0x376
    1b78:	528053e1 	mov	w1, #0x29f                 	// #671
    1b7c:	91103c42 	add	x2, x2, #0x40f
    1b80:	97ffff68 	bl	0x1920
    1b84:	14000000 	b	0x1b84
    1b88:	eb03013f 	cmp	x9, x3
    1b8c:	54000102 	b.cs	0x1bac  // b.hs, b.nlast
    1b90:	d0000000 	adrp	x0, 0x3000
    1b94:	d0000002 	adrp	x2, 0x3000
    1b98:	910dd800 	add	x0, x0, #0x376
    1b9c:	528054c1 	mov	w1, #0x2a6                 	// #678
    1ba0:	91178442 	add	x2, x2, #0x5e1
    1ba4:	97ffff5f 	bl	0x1920
    1ba8:	14000000 	b	0x1ba8
    1bac:	b4000241 	cbz	x1, 0x1bf4
    1bb0:	aa1f03ea 	mov	x10, xzr
    1bb4:	386a6849 	ldrb	w9, [x2, x10]
    1bb8:	34000089 	cbz	w9, 0x1bc8
    1bbc:	9100054a 	add	x10, x10, #0x1
    1bc0:	eb01015f 	cmp	x10, x1
    1bc4:	54ffff83 	b.cc	0x1bb4  // b.lo, b.ul, b.last
    1bc8:	eb01007f 	cmp	x3, x1
    1bcc:	54000223 	b.cc	0x1c10  // b.lo, b.ul, b.last
    1bd0:	eb01015f 	cmp	x10, x1
    1bd4:	540001e3 	b.cc	0x1c10  // b.lo, b.ul, b.last
    1bd8:	d0000000 	adrp	x0, 0x3000
    1bdc:	d0000002 	adrp	x2, 0x3000
    1be0:	910dd800 	add	x0, x0, #0x376
    1be4:	52805661 	mov	w1, #0x2b3                 	// #691
    1be8:	9111dc42 	add	x2, x2, #0x477
    1bec:	97ffff4d 	bl	0x1920
    1bf0:	14000000 	b	0x1bf0
    1bf4:	d0000000 	adrp	x0, 0x3000
    1bf8:	d0000002 	adrp	x2, 0x3000
    1bfc:	910dd800 	add	x0, x0, #0x376
    1c00:	52805581 	mov	w1, #0x2ac                 	// #684
    1c04:	9111a042 	add	x2, x2, #0x468
    1c08:	97ffff46 	bl	0x1920
    1c0c:	14000000 	b	0x1c0c
    1c10:	eb03015f 	cmp	x10, x3
    1c14:	9a8a8069 	csel	x9, x3, x10, hi  // hi = pmore
    1c18:	eb02011f 	cmp	x8, x2
    1c1c:	540000a3 	b.cc	0x1c30  // b.lo, b.ul, b.last
    1c20:	8b09004c 	add	x12, x2, x9
    1c24:	9100058d 	add	x13, x12, #0x1
    1c28:	eb0d011f 	cmp	x8, x13
    1c2c:	540000c3 	b.cc	0x1c44  // b.lo, b.ul, b.last
    1c30:	eb08005f 	cmp	x2, x8
    1c34:	54000163 	b.cc	0x1c60  // b.lo, b.ul, b.last
    1c38:	8b010105 	add	x5, x8, x1
    1c3c:	eb05005f 	cmp	x2, x5
    1c40:	54000102 	b.cs	0x1c60  // b.hs, b.nlast
    1c44:	d0000000 	adrp	x0, 0x3000
    1c48:	d0000002 	adrp	x2, 0x3000
    1c4c:	910dd800 	add	x0, x0, #0x376
    1c50:	52805781 	mov	w1, #0x2bc                 	// #700
    1c54:	91161842 	add	x2, x2, #0x586
    1c58:	97ffff32 	bl	0x1920
    1c5c:	14000000 	b	0x1c5c
    1c60:	b4000129 	cbz	x9, 0x1c84
    1c64:	3940004b 	ldrb	w11, [x2]
    1c68:	340000eb 	cbz	w11, 0x1c84
    1c6c:	9100044a 	add	x10, x2, #0x1
    1c70:	3800150b 	strb	w11, [x8], #1
    1c74:	3840154b 	ldrb	w11, [x10], #1
    1c78:	f1000529 	subs	x9, x9, #0x1
    1c7c:	54000040 	b.eq	0x1c84  // b.none
    1c80:	35ffff8b 	cbnz	w11, 0x1c70
    1c84:	aa1f03e0 	mov	x0, xzr
    1c88:	3900011f 	strb	wzr, [x8]
    1c8c:	a8c17bfd 	ldp	x29, x30, [sp], #16
    1c90:	d65f03c0 	ret
    1c94:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    1c98:	910003fd 	mov	x29, sp
    1c9c:	94000004 	bl	0x1cac
    1ca0:	91000400 	add	x0, x0, #0x1
    1ca4:	a8c17bfd 	ldp	x29, x30, [sp], #16
    1ca8:	d65f03c0 	ret
    1cac:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    1cb0:	910003fd 	mov	x29, sp
    1cb4:	b40001e0 	cbz	x0, 0x1cf0
    1cb8:	39400008 	ldrb	w8, [x0]
    1cbc:	34000288 	cbz	w8, 0x1d0c
    1cc0:	d0000008 	adrp	x8, 0x3000
    1cc4:	aa1f03ea 	mov	x10, xzr
    1cc8:	91000409 	add	x9, x0, #0x1
    1ccc:	b9420108 	ldr	w8, [x8, #512]
    1cd0:	34000068 	cbz	w8, 0x1cdc
    1cd4:	eb08015f 	cmp	x10, x8
    1cd8:	54000202 	b.cs	0x1d18  // b.hs, b.nlast
    1cdc:	91000540 	add	x0, x10, #0x1
    1ce0:	386a692b 	ldrb	w11, [x9, x10]
    1ce4:	aa0003ea 	mov	x10, x0
    1ce8:	35ffff4b 	cbnz	w11, 0x1cd0
    1cec:	14000009 	b	0x1d10
    1cf0:	d0000000 	adrp	x0, 0x3000
    1cf4:	d0000002 	adrp	x2, 0x3000
    1cf8:	910de800 	add	x0, x0, #0x37a
    1cfc:	5280a1c1 	mov	w1, #0x50e                 	// #1294
    1d00:	911d2842 	add	x2, x2, #0x74a
    1d04:	97ffff07 	bl	0x1920
    1d08:	14000000 	b	0x1d08
    1d0c:	aa1f03e0 	mov	x0, xzr
    1d10:	a8c17bfd 	ldp	x29, x30, [sp], #16
    1d14:	d65f03c0 	ret
    1d18:	d0000000 	adrp	x0, 0x3000
    1d1c:	d0000002 	adrp	x2, 0x3000
    1d20:	910de800 	add	x0, x0, #0x37a
    1d24:	5280a2c1 	mov	w1, #0x516                 	// #1302
    1d28:	91293442 	add	x2, x2, #0xa4d
    1d2c:	97fffefd 	bl	0x1920
    1d30:	14000000 	b	0x1d30
    1d34:	d41b7b61 	svc	#0xdbdb
    1d38:	d65f03c0 	ret
    1d3c:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    1d40:	910003fd 	mov	x29, sp
    1d44:	90000028 	adrp	x8, 0x5000
    1d48:	f900ad00 	str	x0, [x8, #344]
    1d4c:	b4000160 	cbz	x0, 0x1d78
    1d50:	90000029 	adrp	x9, 0x5000
    1d54:	f900b121 	str	x1, [x9, #352]
    1d58:	b40001e1 	cbz	x1, 0x1d94
    1d5c:	f9403028 	ldr	x8, [x1, #96]
    1d60:	9000002a 	adrp	x10, 0x5000
    1d64:	f900b548 	str	x8, [x10, #360]
    1d68:	b4000248 	cbz	x8, 0x1db0
    1d6c:	aa1f03e0 	mov	x0, xzr
    1d70:	a8c17bfd 	ldp	x29, x30, [sp], #16
    1d74:	d65f03c0 	ret
    1d78:	d0000000 	adrp	x0, 0x3000
    1d7c:	d0000002 	adrp	x2, 0x3000
    1d80:	91302c00 	add	x0, x0, #0xc0b
    1d84:	52800661 	mov	w1, #0x33                  	// #51
    1d88:	91309842 	add	x2, x2, #0xc26
    1d8c:	97fffee5 	bl	0x1920
    1d90:	14000000 	b	0x1d90
    1d94:	d0000000 	adrp	x0, 0x3000
    1d98:	d0000002 	adrp	x2, 0x3000
    1d9c:	91302c00 	add	x0, x0, #0xc0b
    1da0:	52800721 	mov	w1, #0x39                  	// #57
    1da4:	91310c42 	add	x2, x2, #0xc43
    1da8:	97fffede 	bl	0x1920
    1dac:	14000000 	b	0x1dac
    1db0:	d0000000 	adrp	x0, 0x3000
    1db4:	d0000002 	adrp	x2, 0x3000
    1db8:	91302c00 	add	x0, x0, #0xc0b
    1dbc:	91315c42 	add	x2, x2, #0xc57
    1dc0:	320017e1 	orr	w1, wzr, #0x3f
    1dc4:	97fffed7 	bl	0x1920
    1dc8:	14000000 	b	0x1dc8
    1dcc:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    1dd0:	910003fd 	mov	x29, sp
    1dd4:	90000008 	adrp	x8, 0x1000
    1dd8:	90000000 	adrp	x0, 0x1000
    1ddc:	9137d000 	add	x0, x0, #0xdf4
    1de0:	b94eb501 	ldr	w1, [x8, #3764]
    1de4:	94000254 	bl	0x2734
    1de8:	aa1f03e0 	mov	x0, xzr
    1dec:	a8c17bfd 	ldp	x29, x30, [sp], #16
    1df0:	d65f03c0 	ret
    1df4:	d53b4222 	mrs	x2, daif
    1df8:	d5034fdf 	msr	daifset, #0xf
    1dfc:	d5033fdf 	isb
    1e00:	d50b7e20 	dc	civac, x0
    1e04:	d5033b9f 	dsb	ish
    1e08:	d5384243 	mrs	x3, currentel
    1e0c:	f100207f 	cmp	x3, #0x8
    1e10:	5400038c 	b.gt	0x1e80
    1e14:	540001e0 	b.eq	0x1e50  // b.none
    1e18:	b5000043 	cbnz	x3, 0x1e20
    1e1c:	14000000 	b	0x1e1c
    1e20:	d5381008 	mrs	x8, sctlr_el1
    1e24:	927ff909 	and	x9, x8, #0xfffffffffffffffe
    1e28:	d5181009 	msr	sctlr_el1, x9
    1e2c:	d5033fdf 	isb
    1e30:	f9000001 	str	x1, [x0]
    1e34:	d5033fbf 	dmb	sy
    1e38:	d5087620 	dc	ivac, x0
    1e3c:	d508871f 	tlbi	vmalle1
    1e40:	d5033f9f 	dsb	sy
    1e44:	d5181008 	msr	sctlr_el1, x8
    1e48:	d5033fdf 	isb
    1e4c:	14000018 	b	0x1eac
    1e50:	d53c1008 	mrs	x8, sctlr_el2
    1e54:	927ff909 	and	x9, x8, #0xfffffffffffffffe
    1e58:	d51c1009 	msr	sctlr_el2, x9
    1e5c:	d5033fdf 	isb
    1e60:	f9000001 	str	x1, [x0]
    1e64:	d5033fbf 	dmb	sy
    1e68:	d5087620 	dc	ivac, x0
    1e6c:	d50c871f 	tlbi	alle2
    1e70:	d5033f9f 	dsb	sy
    1e74:	d51c1008 	msr	sctlr_el2, x8
    1e78:	d5033fdf 	isb
    1e7c:	1400000c 	b	0x1eac
    1e80:	d53e1008 	mrs	x8, sctlr_el3
    1e84:	927ff909 	and	x9, x8, #0xfffffffffffffffe
    1e88:	d51e1009 	msr	sctlr_el3, x9
    1e8c:	d5033fdf 	isb
    1e90:	f9000001 	str	x1, [x0]
    1e94:	d5033fbf 	dmb	sy
    1e98:	d5087620 	dc	ivac, x0
    1e9c:	d50e871f 	tlbi	alle3
    1ea0:	d5033f9f 	dsb	sy
    1ea4:	d51e1008 	msr	sctlr_el3, x8
    1ea8:	d5033fdf 	isb
    1eac:	d51b4222 	msr	daif, x2
    1eb0:	d65f03c0 	ret
    1eb4:	000000c0 	udf	#192
    1eb8:	f9402c28 	ldr	x8, [x1, #88]
    1ebc:	90000029 	adrp	x9, 0x5000
    1ec0:	f900b928 	str	x8, [x9, #368]
    1ec4:	b4000068 	cbz	x8, 0x1ed0
    1ec8:	aa1f03e0 	mov	x0, xzr
    1ecc:	d65f03c0 	ret
    1ed0:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    1ed4:	910003fd 	mov	x29, sp
    1ed8:	d0000000 	adrp	x0, 0x3000
    1edc:	d0000002 	adrp	x2, 0x3000
    1ee0:	9131ac00 	add	x0, x0, #0xc6b
    1ee4:	528005e1 	mov	w1, #0x2f                  	// #47
    1ee8:	91322442 	add	x2, x2, #0xc89
    1eec:	97fffe8d 	bl	0x1920
    1ef0:	a8c17bfd 	ldp	x29, x30, [sp], #16
    1ef4:	14000000 	b	0x1ef4
    1ef8:	aa1f03e0 	mov	x0, xzr
    1efc:	d65f03c0 	ret
    1f00:	a9bc5ff8 	stp	x24, x23, [sp, #-64]!
    1f04:	a90157f6 	stp	x22, x21, [sp, #16]
    1f08:	a9024ff4 	stp	x20, x19, [sp, #32]
    1f0c:	a9037bfd 	stp	x29, x30, [sp, #48]
    1f10:	9100c3fd 	add	x29, sp, #0x30
    1f14:	aa0103f3 	mov	x19, x1
    1f18:	aa0003f4 	mov	x20, x0
    1f1c:	b40002f4 	cbz	x20, 0x1f78
    1f20:	b40003b3 	cbz	x19, 0x1f94
    1f24:	90000028 	adrp	x8, 0x5000
    1f28:	d2f00015 	mov	x21, #0x8000000000000000    	// #-9223372036854775808
    1f2c:	f28001d5 	movk	x21, #0xe
    1f30:	f940b116 	ldr	x22, [x8, #352]
    1f34:	f900027f 	str	xzr, [x19]
    1f38:	f94036c8 	ldr	x8, [x22, #104]
    1f3c:	b4000448 	cbz	x8, 0x1fc4
    1f40:	aa1f03f7 	mov	x23, xzr
    1f44:	aa1f03f8 	mov	x24, xzr
    1f48:	f9403ac9 	ldr	x9, [x22, #112]
    1f4c:	aa1403e0 	mov	x0, x20
    1f50:	8b170121 	add	x1, x9, x23
    1f54:	940002fe 	bl	0x2b4c
    1f58:	12001c08 	and	w8, w0, #0xff
    1f5c:	350002a8 	cbnz	w8, 0x1fb0
    1f60:	f94036cc 	ldr	x12, [x22, #104]
    1f64:	91000718 	add	x24, x24, #0x1
    1f68:	910062f7 	add	x23, x23, #0x18
    1f6c:	eb0c031f 	cmp	x24, x12
    1f70:	54fffec3 	b.cc	0x1f48  // b.lo, b.ul, b.last
    1f74:	14000014 	b	0x1fc4
    1f78:	d0000000 	adrp	x0, 0x3000
    1f7c:	d0000002 	adrp	x2, 0x3000
    1f80:	91327400 	add	x0, x0, #0xc9d
    1f84:	52800c21 	mov	w1, #0x61                  	// #97
    1f88:	91329c42 	add	x2, x2, #0xca7
    1f8c:	97fffe65 	bl	0x1920
    1f90:	14000000 	b	0x1f90
    1f94:	d0000000 	adrp	x0, 0x3000
    1f98:	d0000002 	adrp	x2, 0x3000
    1f9c:	91327400 	add	x0, x0, #0xc9d
    1fa0:	52800c41 	mov	w1, #0x62                  	// #98
    1fa4:	91330442 	add	x2, x2, #0xcc1
    1fa8:	97fffe5e 	bl	0x1920
    1fac:	14000000 	b	0x1fac
    1fb0:	f9403aca 	ldr	x10, [x22, #112]
    1fb4:	aa1f03f5 	mov	x21, xzr
    1fb8:	8b170149 	add	x9, x10, x23
    1fbc:	f940092b 	ldr	x11, [x9, #16]
    1fc0:	f900026b 	str	x11, [x19]
    1fc4:	aa1503e0 	mov	x0, x21
    1fc8:	a9437bfd 	ldp	x29, x30, [sp, #48]
    1fcc:	a9424ff4 	ldp	x20, x19, [sp, #32]
    1fd0:	a94157f6 	ldp	x22, x21, [sp, #16]
    1fd4:	a8c45ff8 	ldp	x24, x23, [sp], #64
    1fd8:	d65f03c0 	ret
    1fdc:	a9be4ff4 	stp	x20, x19, [sp, #-32]!
    1fe0:	a9017bfd 	stp	x29, x30, [sp, #16]
    1fe4:	910043fd 	add	x29, sp, #0x10
    1fe8:	90000020 	adrp	x0, 0x5000
    1fec:	90000021 	adrp	x1, 0x5000
    1ff0:	91010000 	add	x0, x0, #0x40
    1ff4:	9105e021 	add	x1, x1, #0x178
    1ff8:	97ffffc2 	bl	0x1f00
    1ffc:	aa0003f3 	mov	x19, x0
    2000:	b6f80193 	tbz	x19, #63, 0x2030
    2004:	b0000001 	adrp	x1, 0x3000
    2008:	91067021 	add	x1, x1, #0x19c
    200c:	320103e0 	orr	w0, wzr, #0x80000000
    2010:	aa1303e2 	mov	x2, x19
    2014:	97fffd60 	bl	0x1594
    2018:	b0000000 	adrp	x0, 0x3000
    201c:	b0000002 	adrp	x2, 0x3000
    2020:	91396000 	add	x0, x0, #0xe58
    2024:	52800621 	mov	w1, #0x31                  	// #49
    2028:	91074042 	add	x2, x2, #0x1d0
    202c:	97fffe3d 	bl	0x1920
    2030:	f0000008 	adrp	x8, 0x5000
    2034:	f940bd08 	ldr	x8, [x8, #376]
    2038:	b40000a8 	cbz	x8, 0x204c
    203c:	a9417bfd 	ldp	x29, x30, [sp, #16]
    2040:	aa1303e0 	mov	x0, x19
    2044:	a8c24ff4 	ldp	x20, x19, [sp], #32
    2048:	d65f03c0 	ret
    204c:	b0000000 	adrp	x0, 0x3000
    2050:	b0000002 	adrp	x2, 0x3000
    2054:	91396000 	add	x0, x0, #0xe58
    2058:	52800641 	mov	w1, #0x32                  	// #50
    205c:	91398442 	add	x2, x2, #0xe61
    2060:	97fffe30 	bl	0x1920
    2064:	14000000 	b	0x2064
    2068:	b40001a1 	cbz	x1, 0x209c
    206c:	79400029 	ldrh	w9, [x1]
    2070:	32003fe8 	orr	w8, wzr, #0xffff
    2074:	14000004 	b	0x2084
    2078:	79400429 	ldrh	w9, [x1, #2]
    207c:	8b090021 	add	x1, x1, x9
    2080:	79400029 	ldrh	w9, [x1]
    2084:	6b08013f 	cmp	w9, w8
    2088:	540001e0 	b.eq	0x20c4  // b.none
    208c:	12003d29 	and	w9, w9, #0xffff
    2090:	6b20213f 	cmp	w9, w0, uxth
    2094:	54ffff21 	b.ne	0x2078  // b.any
    2098:	1400000c 	b	0x20c8
    209c:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    20a0:	910003fd 	mov	x29, sp
    20a4:	b0000000 	adrp	x0, 0x3000
    20a8:	b0000002 	adrp	x2, 0x3000
    20ac:	91396000 	add	x0, x0, #0xe58
    20b0:	52800d61 	mov	w1, #0x6b                  	// #107
    20b4:	9139e842 	add	x2, x2, #0xe7a
    20b8:	97fffe1a 	bl	0x1920
    20bc:	a8c17bfd 	ldp	x29, x30, [sp], #16
    20c0:	14000000 	b	0x20c0
    20c4:	aa1f03e1 	mov	x1, xzr
    20c8:	aa0103e0 	mov	x0, x1
    20cc:	d65f03c0 	ret
    20d0:	a9be4ff4 	stp	x20, x19, [sp, #-32]!
    20d4:	a9017bfd 	stp	x29, x30, [sp, #16]
    20d8:	910043fd 	add	x29, sp, #0x10
    20dc:	aa0003f3 	mov	x19, x0
    20e0:	14000003 	b	0x20ec
    20e4:	79400688 	ldrh	w8, [x20, #2]
    20e8:	8b080281 	add	x1, x20, x8
    20ec:	321e03e0 	orr	w0, wzr, #0x4
    20f0:	97ffffde 	bl	0x2068
    20f4:	aa0003f4 	mov	x20, x0
    20f8:	b40000f4 	cbz	x20, 0x2114
    20fc:	91002281 	add	x1, x20, #0x8
    2100:	aa1303e0 	mov	x0, x19
    2104:	94000292 	bl	0x2b4c
    2108:	12001c08 	and	w8, w0, #0xff
    210c:	34fffec8 	cbz	w8, 0x20e4
    2110:	14000002 	b	0x2118
    2114:	aa1f03f4 	mov	x20, xzr
    2118:	a9417bfd 	ldp	x29, x30, [sp, #16]
    211c:	aa1403e0 	mov	x0, x20
    2120:	a8c24ff4 	ldp	x20, x19, [sp], #32
    2124:	d65f03c0 	ret
    2128:	f0000008 	adrp	x8, 0x5000
    212c:	f940bd01 	ldr	x1, [x8, #376]
    2130:	b4000041 	cbz	x1, 0x2138
    2134:	17ffffe7 	b	0x20d0
    2138:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    213c:	910003fd 	mov	x29, sp
    2140:	b0000000 	adrp	x0, 0x3000
    2144:	b0000002 	adrp	x2, 0x3000
    2148:	91396000 	add	x0, x0, #0xe58
    214c:	528009a1 	mov	w1, #0x4d                  	// #77
    2150:	91398442 	add	x2, x2, #0xe61
    2154:	97fffdf3 	bl	0x1920
    2158:	a8c17bfd 	ldp	x29, x30, [sp], #16
    215c:	14000000 	b	0x215c
    2160:	b40000c1 	cbz	x1, 0x2178
    2164:	f0000008 	adrp	x8, 0x5000
    2168:	aa1f03e0 	mov	x0, xzr
    216c:	b9418508 	ldr	w8, [x8, #388]
    2170:	f9000028 	str	x8, [x1]
    2174:	d65f03c0 	ret
    2178:	d2f00000 	mov	x0, #0x8000000000000000    	// #-9223372036854775808
    217c:	f2800040 	movk	x0, #0x2
    2180:	d65f03c0 	ret
    2184:	f0000008 	adrp	x8, 0x5000
    2188:	aa1f03e0 	mov	x0, xzr
    218c:	b9018501 	str	w1, [x8, #388]
    2190:	d65f03c0 	ret
    2194:	a9be4ff4 	stp	x20, x19, [sp, #-32]!
    2198:	a9017bfd 	stp	x29, x30, [sp, #16]
    219c:	910043fd 	add	x29, sp, #0x10
    21a0:	d10043ff 	sub	sp, sp, #0x10
    21a4:	f0000008 	adrp	x8, 0x5000
    21a8:	f90007e0 	str	x0, [sp, #8]
    21ac:	b0000009 	adrp	x9, 0x3000
    21b0:	f0000002 	adrp	x2, 0x5000
    21b4:	f900c501 	str	x1, [x8, #392]
    21b8:	f000000a 	adrp	x10, 0x5000
    21bc:	f940302b 	ldr	x11, [x1, #96]
    21c0:	f0000001 	adrp	x1, 0x5000
    21c4:	b9420528 	ldr	w8, [x9, #516]
    21c8:	91020021 	add	x1, x1, #0x80
    21cc:	91044042 	add	x2, x2, #0x110
    21d0:	910023e0 	add	x0, sp, #0x8
    21d4:	f940a569 	ldr	x9, [x11, #328]
    21d8:	aa1f03e3 	mov	x3, xzr
    21dc:	b9018548 	str	w8, [x10, #388]
    21e0:	d63f0120 	blr	x9
    21e4:	aa0003f3 	mov	x19, x0
    21e8:	94000006 	bl	0x2200
    21ec:	aa1303e0 	mov	x0, x19
    21f0:	d10043bf 	sub	sp, x29, #0x10
    21f4:	a9417bfd 	ldp	x29, x30, [sp, #16]
    21f8:	a8c24ff4 	ldp	x20, x19, [sp], #32
    21fc:	d65f03c0 	ret
    2200:	a9bd57f6 	stp	x22, x21, [sp, #-48]!
    2204:	a9014ff4 	stp	x20, x19, [sp, #16]
    2208:	a9027bfd 	stp	x29, x30, [sp, #32]
    220c:	910083fd 	add	x29, sp, #0x20
    2210:	d10043ff 	sub	sp, sp, #0x10
    2214:	f0000015 	adrp	x21, 0x5000
    2218:	f940c6a9 	ldr	x9, [x21, #392]
    221c:	b4000629 	cbz	x9, 0x22e0
    2220:	f0000014 	adrp	x20, 0x5000
    2224:	39460288 	ldrb	w8, [x20, #384]
    2228:	35000568 	cbnz	w8, 0x22d4
    222c:	f940312a 	ldr	x10, [x9, #96]
    2230:	320013e0 	orr	w0, wzr, #0x1f
    2234:	f9400d48 	ldr	x8, [x10, #24]
    2238:	d63f0100 	blr	x8
    223c:	f940c6ab 	ldr	x11, [x21, #392]
    2240:	aa0003f3 	mov	x19, x0
    2244:	f940316c 	ldr	x12, [x11, #96]
    2248:	f9401189 	ldr	x9, [x12, #32]
    224c:	d63f0120 	blr	x9
    2250:	f100227f 	cmp	x19, #0x8
    2254:	54000408 	b.hi	0x22d4  // b.pmore
    2258:	321d03eb 	orr	w11, wzr, #0x8
    225c:	f940c6ad 	ldr	x13, [x21, #392]
    2260:	b0000000 	adrp	x0, 0x3000
    2264:	f0000001 	adrp	x1, 0x5000
    2268:	f90007eb 	str	x11, [sp, #8]
    226c:	913a9800 	add	x0, x0, #0xea6
    2270:	f9402dae 	ldr	x14, [x13, #88]
    2274:	91014021 	add	x1, x1, #0x50
    2278:	910023e3 	add	x3, sp, #0x8
    227c:	910003e4 	mov	x4, sp
    2280:	aa1f03e2 	mov	x2, xzr
    2284:	f94025cc 	ldr	x12, [x14, #72]
    2288:	d63f0180 	blr	x12
    228c:	d2f4000d 	mov	x13, #0xa000000000000000    	// #-6917529027641081856
    2290:	f280004d 	movk	x13, #0x2
    2294:	eb0d001f 	cmp	x0, x13
    2298:	54000341 	b.ne	0x2300  // b.any
    229c:	f0000000 	adrp	x0, 0x5000
    22a0:	91014000 	add	x0, x0, #0x50
    22a4:	97ffffa1 	bl	0x2128
    22a8:	b4000160 	cbz	x0, 0x22d4
    22ac:	79400408 	ldrh	w8, [x0, #2]
    22b0:	7100711f 	cmp	w8, #0x1c
    22b4:	f0000008 	adrp	x8, 0x5000
    22b8:	91061108 	add	x8, x8, #0x184
    22bc:	54000161 	b.ne	0x22e8  // b.any
    22c0:	b940180b 	ldr	w11, [x0, #24]
    22c4:	320003ec 	orr	w12, wzr, #0x1
    22c8:	3906028c 	strb	w12, [x20, #384]
    22cc:	b900010b 	str	w11, [x8]
    22d0:	14000006 	b	0x22e8
    22d4:	f0000008 	adrp	x8, 0x5000
    22d8:	91061108 	add	x8, x8, #0x184
    22dc:	14000003 	b	0x22e8
    22e0:	b0000008 	adrp	x8, 0x3000
    22e4:	91081108 	add	x8, x8, #0x204
    22e8:	b9400100 	ldr	w0, [x8]
    22ec:	d10083bf 	sub	sp, x29, #0x20
    22f0:	a9427bfd 	ldp	x29, x30, [sp, #32]
    22f4:	a9414ff4 	ldp	x20, x19, [sp, #16]
    22f8:	a8c357f6 	ldp	x22, x21, [sp], #48
    22fc:	d65f03c0 	ret
    2300:	f0000008 	adrp	x8, 0x5000
    2304:	320003e9 	orr	w9, wzr, #0x1
    2308:	91061108 	add	x8, x8, #0x184
    230c:	39060289 	strb	w9, [x20, #384]
    2310:	b7fffec0 	tbnz	x0, #63, 0x22e8
    2314:	f94003ef 	ldr	x15, [sp]
    2318:	b900010f 	str	w15, [x8]
    231c:	17fffff3 	b	0x22e8
    2320:	f9403028 	ldr	x8, [x1, #96]
    2324:	f0000001 	adrp	x1, 0x5000
    2328:	f0000002 	adrp	x2, 0x5000
    232c:	aa1f03e3 	mov	x3, xzr
    2330:	91020021 	add	x1, x1, #0x80
    2334:	f940a904 	ldr	x4, [x8, #336]
    2338:	91044042 	add	x2, x2, #0x110
    233c:	d61f0080 	br	x4
    2340:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    2344:	910003fd 	mov	x29, sp
    2348:	9400011b 	bl	0x27b4
    234c:	b40001a0 	cbz	x0, 0x2380
    2350:	b0000008 	adrp	x8, 0x3000
    2354:	b9420908 	ldr	w8, [x8, #520]
    2358:	51000509 	sub	w9, w8, #0x1
    235c:	52a001e8 	mov	w8, #0xf0000               	// #983040
    2360:	728847c8 	movk	w8, #0x423e
    2364:	6b08013f 	cmp	w9, w8
    2368:	54000229 	b.ls	0x23ac  // b.plast
    236c:	940002bd 	bl	0x2e60
    2370:	b40002c0 	cbz	x0, 0x23c8
    2374:	aa1f03e0 	mov	x0, xzr
    2378:	a8c17bfd 	ldp	x29, x30, [sp], #16
    237c:	d65f03c0 	ret
    2380:	b0000001 	adrp	x1, 0x3000
    2384:	913c9021 	add	x1, x1, #0xf24
    2388:	320103e0 	orr	w0, wzr, #0x80000000
    238c:	97fffc82 	bl	0x1594
    2390:	b0000000 	adrp	x0, 0x3000
    2394:	b0000002 	adrp	x2, 0x3000
    2398:	913ae000 	add	x0, x0, #0xeb8
    239c:	52800a01 	mov	w1, #0x50                  	// #80
    23a0:	913c8842 	add	x2, x2, #0xf22
    23a4:	97fffd5f 	bl	0x1920
    23a8:	14000000 	b	0x23a8
    23ac:	b0000000 	adrp	x0, 0x3000
    23b0:	b0000002 	adrp	x2, 0x3000
    23b4:	913ae000 	add	x0, x0, #0xeb8
    23b8:	913b2842 	add	x2, x2, #0xeca
    23bc:	321d0be1 	orr	w1, wzr, #0x38
    23c0:	97fffd58 	bl	0x1920
    23c4:	14000000 	b	0x23c4
    23c8:	b0000000 	adrp	x0, 0x3000
    23cc:	b0000002 	adrp	x2, 0x3000
    23d0:	913ae000 	add	x0, x0, #0xeb8
    23d4:	52800981 	mov	w1, #0x4c                  	// #76
    23d8:	913c0042 	add	x2, x2, #0xf00
    23dc:	97fffd51 	bl	0x1920
    23e0:	14000000 	b	0x23e0
    23e4:	140002d6 	b	0x2f3c
    23e8:	b0000009 	adrp	x9, 0x3000
    23ec:	913e0129 	add	x9, x9, #0xf80
    23f0:	aa1f03ea 	mov	x10, xzr
    23f4:	2a1f03e8 	mov	w8, wzr
    23f8:	91001129 	add	x9, x9, #0x4
    23fc:	385fc12b 	ldurb	w11, [x9, #-4]
    2400:	6b00017f 	cmp	w11, w0
    2404:	54000041 	b.ne	0x240c  // b.any
    2408:	b9400128 	ldr	w8, [x9]
    240c:	9100054b 	add	x11, x10, #0x1
    2410:	f100615f 	cmp	x10, #0x18
    2414:	91002129 	add	x9, x9, #0x8
    2418:	aa0b03ea 	mov	x10, x11
    241c:	54ffff01 	b.ne	0x23fc  // b.any
    2420:	2a0803e0 	mov	w0, w8
    2424:	d65f03c0 	ret
    2428:	b0000008 	adrp	x8, 0x3000
    242c:	913e0108 	add	x8, x8, #0xf80
    2430:	aa1f03e9 	mov	x9, xzr
    2434:	91001108 	add	x8, x8, #0x4
    2438:	b940010a 	ldr	w10, [x8]
    243c:	6b00015f 	cmp	w10, w0
    2440:	540000e0 	b.eq	0x245c  // b.none
    2444:	91000529 	add	x9, x9, #0x1
    2448:	91002108 	add	x8, x8, #0x8
    244c:	f100653f 	cmp	x9, #0x19
    2450:	54ffff4b 	b.lt	0x2438  // b.tstop
    2454:	2a1f03e0 	mov	w0, wzr
    2458:	d65f03c0 	ret
    245c:	385fc100 	ldurb	w0, [x8, #-4]
    2460:	d65f03c0 	ret
    2464:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    2468:	910003fd 	mov	x29, sp
    246c:	d10043ff 	sub	sp, sp, #0x10
    2470:	f0000008 	adrp	x8, 0x5000
    2474:	d10013a1 	sub	x1, x29, #0x4
    2478:	aa1f03e2 	mov	x2, xzr
    247c:	f940c900 	ldr	x0, [x8, #400]
    2480:	aa1f03e3 	mov	x3, xzr
    2484:	f9400409 	ldr	x9, [x0, #8]
    2488:	b81fc3bf 	stur	wzr, [x29, #-4]
    248c:	d63f0120 	blr	x9
    2490:	aa0003e8 	mov	x8, x0
    2494:	b40000c8 	cbz	x8, 0x24ac
    2498:	d0000001 	adrp	x1, 0x4000
    249c:	91012021 	add	x1, x1, #0x48
    24a0:	320103e0 	orr	w0, wzr, #0x80000000
    24a4:	aa0803e2 	mov	x2, x8
    24a8:	97fffc3b 	bl	0x1594
    24ac:	b85fc3a0 	ldur	w0, [x29, #-4]
    24b0:	910003bf 	mov	sp, x29
    24b4:	a8c17bfd 	ldp	x29, x30, [sp], #16
    24b8:	d65f03c0 	ret
    24bc:	14000001 	b	0x24c0
    24c0:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    24c4:	910003fd 	mov	x29, sp
    24c8:	f0000008 	adrp	x8, 0x5000
    24cc:	531f7802 	lsl	w2, w0, #1
    24d0:	2a1f03e0 	mov	w0, wzr
    24d4:	f940d109 	ldr	x9, [x8, #416]
    24d8:	320007e1 	orr	w1, wzr, #0x3
    24dc:	321f1be3 	orr	w3, wzr, #0xfe
    24e0:	f9401529 	ldr	x9, [x9, #40]
    24e4:	d63f0120 	blr	x9
    24e8:	aa0003e8 	mov	x8, x0
    24ec:	b40000e8 	cbz	x8, 0x2508
    24f0:	d0000001 	adrp	x1, 0x4000
    24f4:	91053821 	add	x1, x1, #0x14e
    24f8:	320103e0 	orr	w0, wzr, #0x80000000
    24fc:	aa0803e2 	mov	x2, x8
    2500:	a8c17bfd 	ldp	x29, x30, [sp], #16
    2504:	17fffc24 	b	0x1594
    2508:	a8c17bfd 	ldp	x29, x30, [sp], #16
    250c:	d65f03c0 	ret
    2510:	52800820 	mov	w0, #0x41                  	// #65
    2514:	17ffffeb 	b	0x24c0
    2518:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    251c:	910003fd 	mov	x29, sp
    2520:	d10043ff 	sub	sp, sp, #0x10
    2524:	910023e0 	add	x0, sp, #0x8
    2528:	b9000bff 	str	wzr, [sp, #8]
    252c:	940002bd 	bl	0x3020
    2530:	2a0003e8 	mov	w8, w0
    2534:	340000c8 	cbz	w8, 0x254c
    2538:	d0000001 	adrp	x1, 0x4000
    253c:	9101a421 	add	x1, x1, #0x69
    2540:	320103e0 	orr	w0, wzr, #0x80000000
    2544:	2a0803e2 	mov	w2, w8
    2548:	97fffc13 	bl	0x1594
    254c:	b9400be9 	ldr	w9, [sp, #8]
    2550:	5280402a 	mov	w10, #0x201                 	// #513
    2554:	0a0a0128 	and	w8, w9, w10
    2558:	7108051f 	cmp	w8, #0x201
    255c:	1a9f17e0 	cset	w0, eq  // eq = none
    2560:	910003bf 	mov	sp, x29
    2564:	a8c17bfd 	ldp	x29, x30, [sp], #16
    2568:	d65f03c0 	ret
    256c:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    2570:	910003fd 	mov	x29, sp
    2574:	d10043ff 	sub	sp, sp, #0x10
    2578:	910023e0 	add	x0, sp, #0x8
    257c:	b9000bff 	str	wzr, [sp, #8]
    2580:	940002a8 	bl	0x3020
    2584:	2a0003e8 	mov	w8, w0
    2588:	340000c8 	cbz	w8, 0x25a0
    258c:	d0000001 	adrp	x1, 0x4000
    2590:	9101a421 	add	x1, x1, #0x69
    2594:	320103e0 	orr	w0, wzr, #0x80000000
    2598:	2a0803e2 	mov	w2, w8
    259c:	97fffbfe 	bl	0x1594
    25a0:	b9400be9 	ldr	w9, [sp, #8]
    25a4:	531f7d20 	lsr	w0, w9, #31
    25a8:	910003bf 	mov	sp, x29
    25ac:	a8c17bfd 	ldp	x29, x30, [sp], #16
    25b0:	d65f03c0 	ret
    25b4:	f0000009 	adrp	x9, 0x5000
    25b8:	f940cd28 	ldr	x8, [x9, #408]
    25bc:	b40000a8 	cbz	x8, 0x25d0
    25c0:	39400908 	ldrb	w8, [x8, #2]
    25c4:	7100011f 	cmp	w8, #0x0
    25c8:	1a9f07e0 	cset	w0, ne  // ne = any
    25cc:	d65f03c0 	ret
    25d0:	2a1f03e0 	mov	w0, wzr
    25d4:	d65f03c0 	ret
    25d8:	f0000009 	adrp	x9, 0x5000
    25dc:	f940cd28 	ldr	x8, [x9, #408]
    25e0:	b4000088 	cbz	x8, 0x25f0
    25e4:	39400108 	ldrb	w8, [x8]
    25e8:	12000100 	and	w0, w8, #0x1
    25ec:	d65f03c0 	ret
    25f0:	2a1f03e0 	mov	w0, wzr
    25f4:	d65f03c0 	ret
    25f8:	a9be4ff4 	stp	x20, x19, [sp, #-32]!
    25fc:	a9017bfd 	stp	x29, x30, [sp, #16]
    2600:	910043fd 	add	x29, sp, #0x10
    2604:	d10043ff 	sub	sp, sp, #0x10
    2608:	f0000014 	adrp	x20, 0x5000
    260c:	f0000000 	adrp	x0, 0x5000
    2610:	f0000002 	adrp	x2, 0x5000
    2614:	f940b689 	ldr	x9, [x20, #360]
    2618:	aa1f03e1 	mov	x1, xzr
    261c:	91028000 	add	x0, x0, #0xa0
    2620:	91064042 	add	x2, x2, #0x190
    2624:	f940a128 	ldr	x8, [x9, #320]
    2628:	d63f0100 	blr	x8
    262c:	aa0003f3 	mov	x19, x0
    2630:	b4000093 	cbz	x19, 0x2640
    2634:	d0000001 	adrp	x1, 0x4000
    2638:	91023821 	add	x1, x1, #0x8e
    263c:	14000036 	b	0x2714
    2640:	f940b68a 	ldr	x10, [x20, #360]
    2644:	f0000013 	adrp	x19, 0x5000
    2648:	f0000000 	adrp	x0, 0x5000
    264c:	91068273 	add	x19, x19, #0x1a0
    2650:	aa1f03e1 	mov	x1, xzr
    2654:	f940a149 	ldr	x9, [x10, #320]
    2658:	91030000 	add	x0, x0, #0xc0
    265c:	aa1303e2 	mov	x2, x19
    2660:	d63f0120 	blr	x9
    2664:	f940026a 	ldr	x10, [x19]
    2668:	aa0003f3 	mov	x19, x0
    266c:	f100027f 	cmp	x19, #0x0
    2670:	fa400944 	ccmp	x10, #0x0, #0x4, eq  // eq = none
    2674:	54000081 	b.ne	0x2684  // b.any
    2678:	d0000001 	adrp	x1, 0x4000
    267c:	91030821 	add	x1, x1, #0xc2
    2680:	14000025 	b	0x2714
    2684:	f940b68b 	ldr	x11, [x20, #360]
    2688:	f0000014 	adrp	x20, 0x5000
    268c:	f0000000 	adrp	x0, 0x5000
    2690:	9106a294 	add	x20, x20, #0x1a8
    2694:	aa1f03e1 	mov	x1, xzr
    2698:	f940a16c 	ldr	x12, [x11, #320]
    269c:	9102c000 	add	x0, x0, #0xb0
    26a0:	aa1403e2 	mov	x2, x20
    26a4:	d63f0180 	blr	x12
    26a8:	aa0003f3 	mov	x19, x0
    26ac:	b5000313 	cbnz	x19, 0x270c
    26b0:	f9400288 	ldr	x8, [x20]
    26b4:	b40002c8 	cbz	x8, 0x270c
    26b8:	f0000014 	adrp	x20, 0x5000
    26bc:	f940090d 	ldr	x13, [x8, #16]
    26c0:	91066294 	add	x20, x20, #0x198
    26c4:	52803260 	mov	w0, #0x193                 	// #403
    26c8:	910033e1 	add	x1, sp, #0xc
    26cc:	aa1403e2 	mov	x2, x20
    26d0:	d63f01a0 	blr	x13
    26d4:	b9400fe2 	ldr	w2, [sp, #12]
    26d8:	f9400283 	ldr	x3, [x20]
    26dc:	7100205f 	cmp	w2, #0x8
    26e0:	54000063 	b.cc	0x26ec  // b.lo, b.ul, b.last
    26e4:	aa1f03f3 	mov	x19, xzr
    26e8:	b50001c3 	cbnz	x3, 0x2720
    26ec:	d0000001 	adrp	x1, 0x4000
    26f0:	91048821 	add	x1, x1, #0x122
    26f4:	320103e0 	orr	w0, wzr, #0x80000000
    26f8:	97fffba7 	bl	0x1594
    26fc:	f000000c 	adrp	x12, 0x5000
    2700:	aa1f03f3 	mov	x19, xzr
    2704:	f900cd9f 	str	xzr, [x12, #408]
    2708:	14000006 	b	0x2720
    270c:	d0000001 	adrp	x1, 0x4000
    2710:	9103d421 	add	x1, x1, #0xf5
    2714:	320103e0 	orr	w0, wzr, #0x80000000
    2718:	aa1303e2 	mov	x2, x19
    271c:	97fffb9e 	bl	0x1594
    2720:	aa1303e0 	mov	x0, x19
    2724:	d10043bf 	sub	sp, x29, #0x10
    2728:	a9417bfd 	ldp	x29, x30, [sp, #16]
    272c:	a8c24ff4 	ldp	x20, x19, [sp], #32
    2730:	d65f03c0 	ret
    2734:	a9bd57f6 	stp	x22, x21, [sp, #-48]!
    2738:	a9014ff4 	stp	x20, x19, [sp, #16]
    273c:	a9027bfd 	stp	x29, x30, [sp, #32]
    2740:	910083fd 	add	x29, sp, #0x20
    2744:	aa0103f6 	mov	x22, x1
    2748:	aa0003f3 	mov	x19, x0
    274c:	9400001e 	bl	0x27c4
    2750:	aa0003f4 	mov	x20, x0
    2754:	cb1403e8 	neg	x8, x20
    2758:	8a080275 	and	x21, x19, x8
    275c:	8b160276 	add	x22, x19, x22
    2760:	14000004 	b	0x2770
    2764:	aa1503e0 	mov	x0, x21
    2768:	9400000d 	bl	0x279c
    276c:	8b1402b5 	add	x21, x21, x20
    2770:	eb1602bf 	cmp	x21, x22
    2774:	54ffff83 	b.cc	0x2764  // b.lo, b.ul, b.last
    2778:	9400000b 	bl	0x27a4
    277c:	9400000c 	bl	0x27ac
    2780:	aa1303e0 	mov	x0, x19
    2784:	a9427bfd 	ldp	x29, x30, [sp, #32]
    2788:	a9414ff4 	ldp	x20, x19, [sp, #16]
    278c:	a8c357f6 	ldp	x22, x21, [sp], #48
    2790:	d65f03c0 	ret
    2794:	d5087620 	dc	ivac, x0
    2798:	d65f03c0 	ret
    279c:	d50b7a20 	dc	cvac, x0
    27a0:	d65f03c0 	ret
    27a4:	d5033f9f 	dsb	sy
    27a8:	d65f03c0 	ret
    27ac:	d5033fdf 	isb
    27b0:	d65f03c0 	ret
    27b4:	d2800020 	mov	x0, #0x1                   	// #1
    27b8:	d65f03c0 	ret
    27bc:	d53b0020 	mrs	x0, ctr_el0
    27c0:	d65f03c0 	ret
    27c4:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    27c8:	910003fd 	mov	x29, sp
    27cc:	97fffffc 	bl	0x27bc
    27d0:	53104c08 	ubfx	w8, w0, #16, #4
    27d4:	321e03e9 	orr	w9, wzr, #0x4
    27d8:	1ac82120 	lsl	w0, w9, w8
    27dc:	a8c17bfd 	ldp	x29, x30, [sp], #16
    27e0:	d65f03c0 	ret
    27e4:	a9be4ff4 	stp	x20, x19, [sp, #-32]!
    27e8:	a9017bfd 	stp	x29, x30, [sp, #16]
    27ec:	910043fd 	add	x29, sp, #0x10
    27f0:	f0000013 	adrp	x19, 0x5000
    27f4:	f940da69 	ldr	x9, [x19, #432]
    27f8:	b50001c9 	cbnz	x9, 0x2830
    27fc:	f0000009 	adrp	x9, 0x5000
    2800:	f940b528 	ldr	x8, [x9, #360]
    2804:	b4000168 	cbz	x8, 0x2830
    2808:	f940a108 	ldr	x8, [x8, #320]
    280c:	b4000128 	cbz	x8, 0x2830
    2810:	f0000000 	adrp	x0, 0x5000
    2814:	f0000002 	adrp	x2, 0x5000
    2818:	aa1f03e1 	mov	x1, xzr
    281c:	9101c000 	add	x0, x0, #0x70
    2820:	9106c042 	add	x2, x2, #0x1b0
    2824:	d63f0100 	blr	x8
    2828:	b6f80040 	tbz	x0, #63, 0x2830
    282c:	f900da7f 	str	xzr, [x19, #432]
    2830:	a9417bfd 	ldp	x29, x30, [sp, #16]
    2834:	a8c24ff4 	ldp	x20, x19, [sp], #32
    2838:	d65f03c0 	ret
    283c:	a9bc5ff8 	stp	x24, x23, [sp, #-64]!
    2840:	a90157f6 	stp	x22, x21, [sp, #16]
    2844:	a9024ff4 	stp	x20, x19, [sp, #32]
    2848:	a9037bfd 	stp	x29, x30, [sp, #48]
    284c:	9100c3fd 	add	x29, sp, #0x30
    2850:	b0000009 	adrp	x9, 0x3000
    2854:	2a0003f5 	mov	w21, w0
    2858:	aa0403f3 	mov	x19, x4
    285c:	3947f928 	ldrb	w8, [x9, #510]
    2860:	aa0303f4 	mov	x20, x3
    2864:	2a0203f6 	mov	w22, w2
    2868:	2a0103f7 	mov	w23, w1
    286c:	12001ea9 	and	w9, w21, #0xff
    2870:	7100053f 	cmp	w9, #0x1
    2874:	54000061 	b.ne	0x2880  // b.any
    2878:	1200010a 	and	w10, w8, #0x1
    287c:	3500016a 	cbnz	w10, 0x28a8
    2880:	7100093f 	cmp	w9, #0x2
    2884:	54000081 	b.ne	0x2894  // b.any
    2888:	121f010b 	and	w11, w8, #0x2
    288c:	12001d6c 	and	w12, w11, #0xff
    2890:	350000cc 	cbnz	w12, 0x28a8
    2894:	71000d3f 	cmp	w9, #0x3
    2898:	54000261 	b.ne	0x28e4  // b.any
    289c:	121e0108 	and	w8, w8, #0x4
    28a0:	12001d09 	and	w9, w8, #0xff
    28a4:	34000209 	cbz	w9, 0x28e4
    28a8:	97ffffcf 	bl	0x27e4
    28ac:	f000000a 	adrp	x10, 0x5000
    28b0:	f940d948 	ldr	x8, [x10, #432]
    28b4:	b4000188 	cbz	x8, 0x28e4
    28b8:	f9400105 	ldr	x5, [x8]
    28bc:	2a1503e0 	mov	w0, w21
    28c0:	2a1603e2 	mov	w2, w22
    28c4:	aa1403e3 	mov	x3, x20
    28c8:	aa1303e4 	mov	x4, x19
    28cc:	a9437bfd 	ldp	x29, x30, [sp, #48]
    28d0:	2a1703e1 	mov	w1, w23
    28d4:	a9424ff4 	ldp	x20, x19, [sp, #32]
    28d8:	a94157f6 	ldp	x22, x21, [sp, #16]
    28dc:	a8c45ff8 	ldp	x24, x23, [sp], #64
    28e0:	d61f00a0 	br	x5
    28e4:	a9437bfd 	ldp	x29, x30, [sp, #48]
    28e8:	b2410be0 	mov	x0, #0x8000000000000003    	// #-9223372036854775805
    28ec:	a9424ff4 	ldp	x20, x19, [sp, #32]
    28f0:	a94157f6 	ldp	x22, x21, [sp, #16]
    28f4:	a8c45ff8 	ldp	x24, x23, [sp], #64
    28f8:	d65f03c0 	ret
    28fc:	b0000008 	adrp	x8, 0x3000
    2900:	3947f908 	ldrb	w8, [x8, #510]
    2904:	12000100 	and	w0, w8, #0x1
    2908:	d65f03c0 	ret
    290c:	b0000008 	adrp	x8, 0x3000
    2910:	3947f908 	ldrb	w8, [x8, #510]
    2914:	53010500 	ubfx	w0, w8, #1, #1
    2918:	d65f03c0 	ret
    291c:	b0000008 	adrp	x8, 0x3000
    2920:	3947f908 	ldrb	w8, [x8, #510]
    2924:	53020900 	ubfx	w0, w8, #2, #1
    2928:	d65f03c0 	ret
    292c:	a9ba6ffc 	stp	x28, x27, [sp, #-96]!
    2930:	a90167fa 	stp	x26, x25, [sp, #16]
    2934:	a9025ff8 	stp	x24, x23, [sp, #32]
    2938:	a90357f6 	stp	x22, x21, [sp, #48]
    293c:	a9044ff4 	stp	x20, x19, [sp, #64]
    2940:	a9057bfd 	stp	x29, x30, [sp, #80]
    2944:	910143fd 	add	x29, sp, #0x50
    2948:	d10843ff 	sub	sp, sp, #0x210
    294c:	aa0603f7 	mov	x23, x6
    2950:	aa0503f8 	mov	x24, x5
    2954:	aa0403f9 	mov	x25, x4
    2958:	aa0303f5 	mov	x21, x3
    295c:	2a0203f3 	mov	w19, w2
    2960:	2a0103f4 	mov	w20, w1
    2964:	2a0003f6 	mov	w22, w0
    2968:	b5000138 	cbnz	x24, 0x298c
    296c:	b4000117 	cbz	x23, 0x298c
    2970:	d0000000 	adrp	x0, 0x4000
    2974:	d0000002 	adrp	x2, 0x4000
    2978:	9105e800 	add	x0, x0, #0x17a
    297c:	9109e442 	add	x2, x2, #0x279
    2980:	321c13e1 	orr	w1, wzr, #0x1f0
    2984:	97fffbe7 	bl	0x1920
    2988:	14000000 	b	0x2988
    298c:	b4000138 	cbz	x24, 0x29b0
    2990:	b5000117 	cbnz	x23, 0x29b0
    2994:	d0000000 	adrp	x0, 0x4000
    2998:	d0000002 	adrp	x2, 0x4000
    299c:	9105e800 	add	x0, x0, #0x17a
    29a0:	52803e21 	mov	w1, #0x1f1                 	// #497
    29a4:	910ad842 	add	x2, x2, #0x2b6
    29a8:	97fffbde 	bl	0x1920
    29ac:	14000000 	b	0x29ac
    29b0:	f000001b 	adrp	x27, 0x5000
    29b4:	f940b769 	ldr	x9, [x27, #360]
    29b8:	b4000409 	cbz	x9, 0x2a38
    29bc:	f940212a 	ldr	x10, [x9, #64]
    29c0:	b40003ca 	cbz	x10, 0x2a38
    29c4:	f940252b 	ldr	x11, [x9, #72]
    29c8:	b400038b 	cbz	x11, 0x2a38
    29cc:	f9400d29 	ldr	x9, [x9, #24]
    29d0:	320013e0 	orr	w0, wzr, #0x1f
    29d4:	d63f0120 	blr	x9
    29d8:	f940b76a 	ldr	x10, [x27, #360]
    29dc:	aa0003fa 	mov	x26, x0
    29e0:	f940114c 	ldr	x12, [x10, #32]
    29e4:	d63f0180 	blr	x12
    29e8:	f100435f 	cmp	x26, #0x10
    29ec:	f81a83bf 	stur	xzr, [x29, #-88]
    29f0:	54000128 	b.hi	0x2a14  // b.pmore
    29f4:	f940b76b 	ldr	x11, [x27, #360]
    29f8:	910052e1 	add	x1, x23, #0x14
    29fc:	321e03e0 	orr	w0, wzr, #0x4
    2a00:	d10163a2 	sub	x2, x29, #0x58
    2a04:	f940216e 	ldr	x14, [x11, #64]
    2a08:	d63f01c0 	blr	x14
    2a0c:	f85a83a8 	ldur	x8, [x29, #-88]
    2a10:	b50002c8 	cbnz	x8, 0x2a68
    2a14:	f107b6ff 	cmp	x23, #0x1ed
    2a18:	54000243 	b.cc	0x2a60  // b.lo, b.ul, b.last
    2a1c:	d0000001 	adrp	x1, 0x4000
    2a20:	910bcc21 	add	x1, x1, #0x2f3
    2a24:	320103e0 	orr	w0, wzr, #0x80000000
    2a28:	97fffadb 	bl	0x1594
    2a2c:	d2f00018 	mov	x24, #0x8000000000000000    	// #-9223372036854775808
    2a30:	f2800138 	movk	x24, #0x9
    2a34:	14000002 	b	0x2a3c
    2a38:	b2410bf8 	mov	x24, #0x8000000000000003    	// #-9223372036854775805
    2a3c:	aa1803e0 	mov	x0, x24
    2a40:	d10143bf 	sub	sp, x29, #0x50
    2a44:	a9457bfd 	ldp	x29, x30, [sp, #80]
    2a48:	a9444ff4 	ldp	x20, x19, [sp, #64]
    2a4c:	a94357f6 	ldp	x22, x21, [sp, #48]
    2a50:	a9425ff8 	ldp	x24, x23, [sp, #32]
    2a54:	a94167fa 	ldp	x26, x25, [sp, #16]
    2a58:	a8c66ffc 	ldp	x28, x27, [sp], #96
    2a5c:	d65f03c0 	ret
    2a60:	910003e8 	mov	x8, sp
    2a64:	f81a83a8 	stur	x8, [x29, #-88]
    2a68:	52800289 	mov	w9, #0x14                  	// #20
    2a6c:	f0000010 	adrp	x16, 0x5000
    2a70:	91008210 	add	x16, x16, #0x20
    2a74:	79000109 	strh	w9, [x8]
    2a78:	f100033f 	cmp	x25, #0x0
    2a7c:	f85a83ac 	ldur	x12, [x29, #-88]
    2a80:	9a990201 	csel	x1, x16, x25, eq  // eq = none
    2a84:	79000597 	strh	w23, [x12, #2]
    2a88:	f85a83ad 	ldur	x13, [x29, #-88]
    2a8c:	910011a0 	add	x0, x13, #0x4
    2a90:	9400001a 	bl	0x2af8
    2a94:	b40000d8 	cbz	x24, 0x2aac
    2a98:	f85a83ae 	ldur	x14, [x29, #-88]
    2a9c:	aa1803e1 	mov	x1, x24
    2aa0:	aa1703e2 	mov	x2, x23
    2aa4:	910051c0 	add	x0, x14, #0x14
    2aa8:	9400003d 	bl	0x2b9c
    2aac:	f0000005 	adrp	x5, 0x5000
    2ab0:	f85a83a4 	ldur	x4, [x29, #-88]
    2ab4:	910020a5 	add	x5, x5, #0x8
    2ab8:	f10002bf 	cmp	x21, #0x0
    2abc:	9a9500a3 	csel	x3, x5, x21, eq  // eq = none
    2ac0:	2a1603e0 	mov	w0, w22
    2ac4:	2a1403e1 	mov	w1, w20
    2ac8:	2a1303e2 	mov	w2, w19
    2acc:	97ffff5c 	bl	0x283c
    2ad0:	f85a83a8 	ldur	x8, [x29, #-88]
    2ad4:	aa0003f8 	mov	x24, x0
    2ad8:	910003ef 	mov	x15, sp
    2adc:	eb0f011f 	cmp	x8, x15
    2ae0:	54fffae0 	b.eq	0x2a3c  // b.none
    2ae4:	f940b770 	ldr	x16, [x27, #360]
    2ae8:	aa0803e0 	mov	x0, x8
    2aec:	f9402602 	ldr	x2, [x16, #72]
    2af0:	d63f0040 	blr	x2
    2af4:	17ffffd2 	b	0x2a3c
    2af8:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    2afc:	910003fd 	mov	x29, sp
    2b00:	b40000a0 	cbz	x0, 0x2b14
    2b04:	b4000161 	cbz	x1, 0x2b30
    2b08:	321c03e2 	orr	w2, wzr, #0x10
    2b0c:	a8c17bfd 	ldp	x29, x30, [sp], #16
    2b10:	1400009b 	b	0x2d7c
    2b14:	d0000000 	adrp	x0, 0x4000
    2b18:	d0000002 	adrp	x2, 0x4000
    2b1c:	910cac00 	add	x0, x0, #0x32b
    2b20:	528005e1 	mov	w1, #0x2f                  	// #47
    2b24:	910ce042 	add	x2, x2, #0x338
    2b28:	97fffb7e 	bl	0x1920
    2b2c:	14000000 	b	0x2b2c
    2b30:	d0000000 	adrp	x0, 0x4000
    2b34:	d0000002 	adrp	x2, 0x4000
    2b38:	910cac00 	add	x0, x0, #0x32b
    2b3c:	910d6042 	add	x2, x2, #0x358
    2b40:	321c07e1 	orr	w1, wzr, #0x30
    2b44:	97fffb77 	bl	0x1920
    2b48:	14000000 	b	0x2b48
    2b4c:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    2b50:	910003fd 	mov	x29, sp
    2b54:	b4000080 	cbz	x0, 0x2b64
    2b58:	b4000141 	cbz	x1, 0x2b80
    2b5c:	a8c17bfd 	ldp	x29, x30, [sp], #16
    2b60:	140000b8 	b	0x2e40
    2b64:	d0000000 	adrp	x0, 0x4000
    2b68:	d0000002 	adrp	x2, 0x4000
    2b6c:	910cac00 	add	x0, x0, #0x32b
    2b70:	52800981 	mov	w1, #0x4c                  	// #76
    2b74:	910dcc42 	add	x2, x2, #0x373
    2b78:	97fffb6a 	bl	0x1920
    2b7c:	14000000 	b	0x2b7c
    2b80:	d0000000 	adrp	x0, 0x4000
    2b84:	d0000002 	adrp	x2, 0x4000
    2b88:	910cac00 	add	x0, x0, #0x32b
    2b8c:	528009a1 	mov	w1, #0x4d                  	// #77
    2b90:	910e2442 	add	x2, x2, #0x389
    2b94:	97fffb63 	bl	0x1920
    2b98:	14000000 	b	0x2b98
    2b9c:	b4000382 	cbz	x2, 0x2c0c
    2ba0:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    2ba4:	910003fd 	mov	x29, sp
    2ba8:	d1000448 	sub	x8, x2, #0x1
    2bac:	aa2003e9 	mvn	x9, x0
    2bb0:	eb09011f 	cmp	x8, x9
    2bb4:	a8c17bfd 	ldp	x29, x30, [sp], #16
    2bb8:	54000109 	b.ls	0x2bd8  // b.plast
    2bbc:	d0000000 	adrp	x0, 0x4000
    2bc0:	d0000002 	adrp	x2, 0x4000
    2bc4:	9110bc00 	add	x0, x0, #0x42f
    2bc8:	91110042 	add	x2, x2, #0x440
    2bcc:	321d0be1 	orr	w1, wzr, #0x38
    2bd0:	97fffb54 	bl	0x1920
    2bd4:	14000000 	b	0x2bd4
    2bd8:	aa2103eb 	mvn	x11, x1
    2bdc:	eb0b011f 	cmp	x8, x11
    2be0:	54000109 	b.ls	0x2c00  // b.plast
    2be4:	d0000000 	adrp	x0, 0x4000
    2be8:	d0000002 	adrp	x2, 0x4000
    2bec:	9110bc00 	add	x0, x0, #0x42f
    2bf0:	52800721 	mov	w1, #0x39                  	// #57
    2bf4:	91120c42 	add	x2, x2, #0x483
    2bf8:	97fffb4a 	bl	0x1920
    2bfc:	14000000 	b	0x2bfc
    2c00:	eb01001f 	cmp	x0, x1
    2c04:	54000040 	b.eq	0x2c0c  // b.none
    2c08:	1400005d 	b	0x2d7c
    2c0c:	d65f03c0 	ret
    2c10:	f9800020 	prfm	pldl1keep, [x1]
    2c14:	8b020024 	add	x4, x1, x2
    2c18:	8b020005 	add	x5, x0, x2
    2c1c:	f100405f 	cmp	x2, #0x10
    2c20:	54000209 	b.ls	0x2c60  // b.plast
    2c24:	f101805f 	cmp	x2, #0x60
    2c28:	54000648 	b.hi	0x2cf0  // b.pmore
    2c2c:	d1000449 	sub	x9, x2, #0x1
    2c30:	a9401c26 	ldp	x6, x7, [x1]
    2c34:	37300469 	tbnz	w9, #6, 0x2cc0
    2c38:	a97f348c 	ldp	x12, x13, [x4, #-16]
    2c3c:	362800a9 	tbz	w9, #5, 0x2c50
    2c40:	a9412428 	ldp	x8, x9, [x1, #16]
    2c44:	a97e2c8a 	ldp	x10, x11, [x4, #-32]
    2c48:	a9012408 	stp	x8, x9, [x0, #16]
    2c4c:	a93e2caa 	stp	x10, x11, [x5, #-32]
    2c50:	a9001c06 	stp	x6, x7, [x0]
    2c54:	a93f34ac 	stp	x12, x13, [x5, #-16]
    2c58:	d65f03c0 	ret
    2c5c:	d503201f 	nop
    2c60:	f100205f 	cmp	x2, #0x8
    2c64:	540000e3 	b.cc	0x2c80  // b.lo, b.ul, b.last
    2c68:	f9400026 	ldr	x6, [x1]
    2c6c:	f85f8087 	ldur	x7, [x4, #-8]
    2c70:	f9000006 	str	x6, [x0]
    2c74:	f81f80a7 	stur	x7, [x5, #-8]
    2c78:	d65f03c0 	ret
    2c7c:	d503201f 	nop
    2c80:	361000c2 	tbz	w2, #2, 0x2c98
    2c84:	b9400026 	ldr	w6, [x1]
    2c88:	b85fc087 	ldur	w7, [x4, #-4]
    2c8c:	b9000006 	str	w6, [x0]
    2c90:	b81fc0a7 	stur	w7, [x5, #-4]
    2c94:	d65f03c0 	ret
    2c98:	b4000102 	cbz	x2, 0x2cb8
    2c9c:	d341fc49 	lsr	x9, x2, #1
    2ca0:	39400026 	ldrb	w6, [x1]
    2ca4:	385ff087 	ldurb	w7, [x4, #-1]
    2ca8:	38696828 	ldrb	w8, [x1, x9]
    2cac:	39000006 	strb	w6, [x0]
    2cb0:	38296808 	strb	w8, [x0, x9]
    2cb4:	381ff0a7 	sturb	w7, [x5, #-1]
    2cb8:	d65f03c0 	ret
    2cbc:	d503201f 	nop
    2cc0:	a9412428 	ldp	x8, x9, [x1, #16]
    2cc4:	a9422c2a 	ldp	x10, x11, [x1, #32]
    2cc8:	a943342c 	ldp	x12, x13, [x1, #48]
    2ccc:	a97e3c8e 	ldp	x14, x15, [x4, #-32]
    2cd0:	a97f0c84 	ldp	x4, x3, [x4, #-16]
    2cd4:	a9001c06 	stp	x6, x7, [x0]
    2cd8:	a9012408 	stp	x8, x9, [x0, #16]
    2cdc:	a9022c0a 	stp	x10, x11, [x0, #32]
    2ce0:	a903340c 	stp	x12, x13, [x0, #48]
    2ce4:	a93e3cae 	stp	x14, x15, [x5, #-32]
    2ce8:	a93f0ca4 	stp	x4, x3, [x5, #-16]
    2cec:	d65f03c0 	ret
    2cf0:	92400c09 	and	x9, x0, #0xf
    2cf4:	927cec03 	and	x3, x0, #0xfffffffffffffff0
    2cf8:	a940342c 	ldp	x12, x13, [x1]
    2cfc:	cb090021 	sub	x1, x1, x9
    2d00:	8b090042 	add	x2, x2, x9
    2d04:	a9411c26 	ldp	x6, x7, [x1, #16]
    2d08:	a900340c 	stp	x12, x13, [x0]
    2d0c:	a9422428 	ldp	x8, x9, [x1, #32]
    2d10:	a9432c2a 	ldp	x10, x11, [x1, #48]
    2d14:	a9c4342c 	ldp	x12, x13, [x1, #64]!
    2d18:	f1024042 	subs	x2, x2, #0x90
    2d1c:	54000169 	b.ls	0x2d48  // b.plast
    2d20:	a9011c66 	stp	x6, x7, [x3, #16]
    2d24:	a9411c26 	ldp	x6, x7, [x1, #16]
    2d28:	a9022468 	stp	x8, x9, [x3, #32]
    2d2c:	a9422428 	ldp	x8, x9, [x1, #32]
    2d30:	a9032c6a 	stp	x10, x11, [x3, #48]
    2d34:	a9432c2a 	ldp	x10, x11, [x1, #48]
    2d38:	a984346c 	stp	x12, x13, [x3, #64]!
    2d3c:	a9c4342c 	ldp	x12, x13, [x1, #64]!
    2d40:	f1010042 	subs	x2, x2, #0x40
    2d44:	54fffee8 	b.hi	0x2d20  // b.pmore
    2d48:	a97c3c8e 	ldp	x14, x15, [x4, #-64]
    2d4c:	a9011c66 	stp	x6, x7, [x3, #16]
    2d50:	a97d1c86 	ldp	x6, x7, [x4, #-48]
    2d54:	a9022468 	stp	x8, x9, [x3, #32]
    2d58:	a97e2488 	ldp	x8, x9, [x4, #-32]
    2d5c:	a9032c6a 	stp	x10, x11, [x3, #48]
    2d60:	a97f2c8a 	ldp	x10, x11, [x4, #-16]
    2d64:	a904346c 	stp	x12, x13, [x3, #64]
    2d68:	a93c3cae 	stp	x14, x15, [x5, #-64]
    2d6c:	a93d1ca6 	stp	x6, x7, [x5, #-48]
    2d70:	a93e24a8 	stp	x8, x9, [x5, #-32]
    2d74:	a93f2caa 	stp	x10, x11, [x5, #-16]
    2d78:	d65f03c0 	ret
    2d7c:	cb010003 	sub	x3, x0, x1
    2d80:	f101805f 	cmp	x2, #0x60
    2d84:	fa428062 	ccmp	x3, x2, #0x2, hi  // hi = pmore
    2d88:	54fff442 	b.cs	0x2c10  // b.hs, b.nlast
    2d8c:	b40004c3 	cbz	x3, 0x2e24
    2d90:	8b020005 	add	x5, x0, x2
    2d94:	8b020024 	add	x4, x1, x2
    2d98:	92400ca3 	and	x3, x5, #0xf
    2d9c:	a97f348c 	ldp	x12, x13, [x4, #-16]
    2da0:	cb030084 	sub	x4, x4, x3
    2da4:	cb030042 	sub	x2, x2, x3
    2da8:	a97f1c86 	ldp	x6, x7, [x4, #-16]
    2dac:	a93f34ac 	stp	x12, x13, [x5, #-16]
    2db0:	a97e2488 	ldp	x8, x9, [x4, #-32]
    2db4:	a97d2c8a 	ldp	x10, x11, [x4, #-48]
    2db8:	a9fc348c 	ldp	x12, x13, [x4, #-64]!
    2dbc:	cb0300a5 	sub	x5, x5, x3
    2dc0:	f1020042 	subs	x2, x2, #0x80
    2dc4:	54000189 	b.ls	0x2df4  // b.plast
    2dc8:	d503201f 	nop
    2dcc:	a93f1ca6 	stp	x6, x7, [x5, #-16]
    2dd0:	a97f1c86 	ldp	x6, x7, [x4, #-16]
    2dd4:	a93e24a8 	stp	x8, x9, [x5, #-32]
    2dd8:	a97e2488 	ldp	x8, x9, [x4, #-32]
    2ddc:	a93d2caa 	stp	x10, x11, [x5, #-48]
    2de0:	a97d2c8a 	ldp	x10, x11, [x4, #-48]
    2de4:	a9bc34ac 	stp	x12, x13, [x5, #-64]!
    2de8:	a9fc348c 	ldp	x12, x13, [x4, #-64]!
    2dec:	f1010042 	subs	x2, x2, #0x40
    2df0:	54fffee8 	b.hi	0x2dcc  // b.pmore
    2df4:	a9433c2e 	ldp	x14, x15, [x1, #48]
    2df8:	a93f1ca6 	stp	x6, x7, [x5, #-16]
    2dfc:	a9421c26 	ldp	x6, x7, [x1, #32]
    2e00:	a93e24a8 	stp	x8, x9, [x5, #-32]
    2e04:	a9412428 	ldp	x8, x9, [x1, #16]
    2e08:	a93d2caa 	stp	x10, x11, [x5, #-48]
    2e0c:	a9402c2a 	ldp	x10, x11, [x1]
    2e10:	a93c34ac 	stp	x12, x13, [x5, #-64]
    2e14:	a9033c0e 	stp	x14, x15, [x0, #48]
    2e18:	a9021c06 	stp	x6, x7, [x0, #32]
    2e1c:	a9012408 	stp	x8, x9, [x0, #16]
    2e20:	a9002c0a 	stp	x10, x11, [x0]
    2e24:	d65f03c0 	ret
	...
    2e40:	aa1f03e2 	mov	x2, xzr
    2e44:	a9401003 	ldp	x3, x4, [x0]
    2e48:	b4000041 	cbz	x1, 0x2e50
    2e4c:	a9400821 	ldp	x1, x2, [x1]
    2e50:	eb03003f 	cmp	x1, x3
    2e54:	fa440040 	ccmp	x2, x4, #0x0, eq  // eq = none
    2e58:	1a9f17e0 	cset	w0, eq  // eq = none
    2e5c:	d65f03c0 	ret
    2e60:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    2e64:	910003fd 	mov	x29, sp
    2e68:	d10043ff 	sub	sp, sp, #0x10
    2e6c:	910023e1 	add	x1, sp, #0x8
    2e70:	f90007ff 	str	xzr, [sp, #8]
    2e74:	2a1f03e0 	mov	w0, wzr
    2e78:	94000074 	bl	0x3048
    2e7c:	f94007e0 	ldr	x0, [sp, #8]
    2e80:	910003bf 	mov	sp, x29
    2e84:	a8c17bfd 	ldp	x29, x30, [sp], #16
    2e88:	d65f03c0 	ret
    2e8c:	00000000 	udf	#0
    2e90:	58000900 	ldr	x0, 0x2fb0
    2e94:	f900001f 	str	xzr, [x0]
    2e98:	97fffe41 	bl	0x279c
    2e9c:	d5033f9f 	dsb	sy
    2ea0:	d5033fdf 	isb
    2ea4:	580008a0 	ldr	x0, 0x2fb8
    2ea8:	580008c2 	ldr	x2, 0x2fc0
    2eac:	d2800103 	mov	x3, #0x8                   	// #8
    2eb0:	d4000003 	smc	#0x0
    2eb4:	7100001f 	cmp	w0, #0x0
    2eb8:	54ffffcc 	b.gt	0x2eb0
    2ebc:	7100001f 	cmp	w0, #0x0
    2ec0:	540000e1 	b.ne	0x2edc  // b.any
    2ec4:	58000820 	ldr	x0, 0x2fc8
    2ec8:	97fffe33 	bl	0x2794
    2ecc:	d5033f9f 	dsb	sy
    2ed0:	d5033fdf 	isb
    2ed4:	580007e1 	ldr	x1, 0x2fd0
    2ed8:	f9400020 	ldr	x0, [x1]
    2edc:	a8c17bfd 	ldp	x29, x30, [sp], #16
    2ee0:	d65f03c0 	ret
    2ee4:	580007a0 	ldr	x0, 0x2fd8
    2ee8:	f900001f 	str	xzr, [x0]
    2eec:	97fffe2c 	bl	0x279c
    2ef0:	d5033f9f 	dsb	sy
    2ef4:	d5033fdf 	isb
    2ef8:	58000740 	ldr	x0, 0x2fe0
    2efc:	d2800441 	mov	x1, #0x22                  	// #34
    2f00:	58000742 	ldr	x2, 0x2fe8
    2f04:	d2800103 	mov	x3, #0x8                   	// #8
    2f08:	d4000003 	smc	#0x0
    2f0c:	7100001f 	cmp	w0, #0x0
    2f10:	54ffffcc 	b.gt	0x2f08
    2f14:	7100001f 	cmp	w0, #0x0
    2f18:	540000e1 	b.ne	0x2f34  // b.any
    2f1c:	580006a0 	ldr	x0, 0x2ff0
    2f20:	97fffe1d 	bl	0x2794
    2f24:	d5033f9f 	dsb	sy
    2f28:	d5033fdf 	isb
    2f2c:	58000661 	ldr	x1, 0x2ff8
    2f30:	f9400020 	ldr	x0, [x1]
    2f34:	a8c17bfd 	ldp	x29, x30, [sp], #16
    2f38:	d65f03c0 	ret
    2f3c:	a9bf7bfd 	stp	x29, x30, [sp, #-16]!
    2f40:	910003fd 	mov	x29, sp
    2f44:	580005e0 	ldr	x0, 0x3000
    2f48:	f9400000 	ldr	x0, [x0]
    2f4c:	10000061 	adr	x1, 0x2f58
    2f50:	f8606822 	ldr	x2, [x1, x0]
    2f54:	d61f0040 	br	x2
    2f58:	00002f70 	udf	#12144
    2f5c:	00000000 	udf	#0
    2f60:	00002e90 	udf	#11920
    2f64:	00000000 	udf	#0
    2f68:	00002ee4 	udf	#12004
    2f6c:	00000000 	udf	#0
    2f70:	580004c0 	ldr	x0, 0x3008
    2f74:	d2800021 	mov	x1, #0x1                   	// #1
    2f78:	580004c2 	ldr	x2, 0x3010
    2f7c:	d4000003 	smc	#0x0
    2f80:	7100001f 	cmp	w0, #0x0
    2f84:	54ffffcc 	b.gt	0x2f7c
    2f88:	7100001f 	cmp	w0, #0x0
    2f8c:	58000461 	ldr	x1, 0x3018
    2f90:	54000081 	b.ne	0x2fa0  // b.any
    2f94:	d2800200 	mov	x0, #0x10                  	// #16
    2f98:	f9000020 	str	x0, [x1]
    2f9c:	17ffffd2 	b	0x2ee4
    2fa0:	d2800100 	mov	x0, #0x8                   	// #8
    2fa4:	f9000020 	str	x0, [x1]
    2fa8:	17ffffba 	b	0x2e90
    2fac:	d503201f 	nop
    2fb0:	00005140 	udf	#20800
    2fb4:	00000000 	udf	#0
    2fb8:	02801202 	.inst	0x02801202 ; undefined
    2fbc:	00000000 	udf	#0
    2fc0:	00005140 	udf	#20800
    2fc4:	00000000 	udf	#0
    2fc8:	00005140 	udf	#20800
    2fcc:	00000000 	udf	#0
    2fd0:	00005140 	udf	#20800
    2fd4:	00000000 	udf	#0
    2fd8:	00005140 	udf	#20800
    2fdc:	00000000 	udf	#0
    2fe0:	02000a01 	.inst	0x02000a01 ; undefined
    2fe4:	00000000 	udf	#0
    2fe8:	00005140 	udf	#20800
    2fec:	00000000 	udf	#0
    2ff0:	00005140 	udf	#20800
    2ff4:	00000000 	udf	#0
    2ff8:	00005140 	udf	#20800
    2ffc:	00000000 	udf	#0
    3000:	00005148 	udf	#20808
    3004:	00000000 	udf	#0
    3008:	02000601 	.inst	0x02000601 ; undefined
    300c:	00000000 	udf	#0
    3010:	02000602 	.inst	0x02000602 ; undefined
    3014:	00000000 	udf	#0
    3018:	00005148 	udf	#20808
    301c:	00000000 	udf	#0
    3020:	b40000e0 	cbz	x0, 0x303c
    3024:	52a28d68 	mov	w8, #0x146b0000            	// #342556672
    3028:	729f5488 	movk	w8, #0xfaa4
    302c:	b9400109 	ldr	w9, [x8]
    3030:	2a1f03e8 	mov	w8, wzr
    3034:	b9000009 	str	w9, [x0]
    3038:	14000002 	b	0x3040
    303c:	32001be8 	orr	w8, wzr, #0x7f
    3040:	2a0803e0 	mov	w0, w8
    3044:	d65f03c0 	ret
    3048:	a9be4ff4 	stp	x20, x19, [sp, #-32]!
    304c:	a9017bfd 	stp	x29, x30, [sp, #16]
    3050:	910043fd 	add	x29, sp, #0x10
    3054:	aa0103f3 	mov	x19, x1
    3058:	2a0003f4 	mov	w20, w0
    305c:	97fffdd6 	bl	0x27b4
    3060:	b40001c0 	cbz	x0, 0x3098
    3064:	2a1403e8 	mov	w8, w20
    3068:	71003a9f 	cmp	w20, #0xe
    306c:	540002c8 	b.hi	0x30c4  // b.pmore
    3070:	b0000009 	adrp	x9, 0x4000
    3074:	91132129 	add	x9, x9, #0x4c8
    3078:	f8687929 	ldr	x9, [x9, x8, lsl #3]
    307c:	d61f0120 	br	x9
    3080:	a9417bfd 	ldp	x29, x30, [sp, #16]
    3084:	b0000001 	adrp	x1, 0x4000
    3088:	91150021 	add	x1, x1, #0x540
    308c:	320103e0 	orr	w0, wzr, #0x80000000
    3090:	a8c24ff4 	ldp	x20, x19, [sp], #32
    3094:	17fff940 	b	0x1594
    3098:	b0000001 	adrp	x1, 0x4000
    309c:	9116e821 	add	x1, x1, #0x5ba
    30a0:	320103e0 	orr	w0, wzr, #0x80000000
    30a4:	97fff93c 	bl	0x1594
    30a8:	b0000000 	adrp	x0, 0x4000
    30ac:	90000002 	adrp	x2, 0x3000
    30b0:	91187400 	add	x0, x0, #0x61d
    30b4:	52800bc1 	mov	w1, #0x5e                  	// #94
    30b8:	913c8842 	add	x2, x2, #0xf22
    30bc:	97fffa19 	bl	0x1920
    30c0:	14000000 	b	0x30c0
    30c4:	a9417bfd 	ldp	x29, x30, [sp, #16]
    30c8:	b0000001 	adrp	x1, 0x4000
    30cc:	91164021 	add	x1, x1, #0x590
    30d0:	320103e0 	orr	w0, wzr, #0x80000000
    30d4:	2a1403e2 	mov	w2, w20
    30d8:	a8c24ff4 	ldp	x20, x19, [sp], #32
    30dc:	17fff92e 	b	0x1594
    30e0:	94000019 	bl	0x3144
    30e4:	14000014 	b	0x3134
    30e8:	94000019 	bl	0x314c
    30ec:	14000012 	b	0x3134
    30f0:	94000019 	bl	0x3154
    30f4:	14000010 	b	0x3134
    30f8:	94000019 	bl	0x315c
    30fc:	1400000e 	b	0x3134
    3100:	94000019 	bl	0x3164
    3104:	1400000c 	b	0x3134
    3108:	94000019 	bl	0x316c
    310c:	1400000a 	b	0x3134
    3110:	94000019 	bl	0x3174
    3114:	14000008 	b	0x3134
    3118:	94000019 	bl	0x317c
    311c:	14000006 	b	0x3134
    3120:	94000019 	bl	0x3184
    3124:	14000004 	b	0x3134
    3128:	94000019 	bl	0x318c
    312c:	14000002 	b	0x3134
    3130:	94000019 	bl	0x3194
    3134:	f9000260 	str	x0, [x19]
    3138:	a9417bfd 	ldp	x29, x30, [sp, #16]
    313c:	a8c24ff4 	ldp	x20, x19, [sp], #32
    3140:	d65f03c0 	ret
    3144:	d53be000 	mrs	x0, cntfrq_el0
    3148:	d65f03c0 	ret
    314c:	d53be020 	mrs	x0, cntpct_el0
    3150:	d65f03c0 	ret
    3154:	d538e100 	mrs	x0, cntkctl_el1
    3158:	d65f03c0 	ret
    315c:	d53be200 	mrs	x0, cntp_tval_el0
    3160:	d65f03c0 	ret
    3164:	d53be220 	mrs	x0, cntp_ctl_el0
    3168:	d65f03c0 	ret
    316c:	d53be300 	mrs	x0, cntv_tval_el0
    3170:	d65f03c0 	ret
    3174:	d53be320 	mrs	x0, cntv_ctl_el0
    3178:	d65f03c0 	ret
    317c:	d53be040 	mrs	x0, cntvct_el0
    3180:	d65f03c0 	ret
    3184:	d53be240 	mrs	x0, cntp_cval_el0
    3188:	d65f03c0 	ret
    318c:	d53be340 	mrs	x0, cntv_cval_el0
    3190:	d65f03c0 	ret
    3194:	d53ce060 	mrs	x0, cntvoff_el2
    3198:	d65f03c0 	ret
    319c:	5353410a 	.inst	0x5353410a ; undefined
    31a0:	5f545245 	.inst	0x5f545245 ; undefined
    31a4:	5f494645 	.inst	0x5f494645 ; undefined
    31a8:	4f525245 	.inst	0x4f525245 ; undefined
    31ac:	53282052 	.inst	0x53282052 ; undefined
    31b0:	75746174 	.inst	0x75746174 ; undefined
    31b4:	203d2073 	.inst	0x203d2073 ; undefined
    31b8:	0a297225 	bic	w5, w17, w9, lsl #28
    31bc:	69724400 	ldpsw	x0, x17, [x0, #-112]
    31c0:	45726576 	addhnt	z22.b, z11.h, z18.h
    31c4:	7972746e 	ldrh	w14, [x3, #6458]
    31c8:	6e696f50 	umin	v16.8h, v26.8h, v9.8h
    31cc:	00632e74 	.inst	0x00632e74 ; undefined
    31d0:	49464521 	.inst	0x49464521 ; undefined
    31d4:	5252455f 	.inst	0x5252455f ; undefined
    31d8:	2820524f 	stnp	w15, w20, [x18, #-256]
    31dc:	74617453 	.inst	0x74617453 ; undefined
    31e0:	00297375 	.inst	0x00297375 ; NYI
    31e4:	73655258 	.inst	0x73655258 ; undefined
    31e8:	65527465 	fcmeq	p5.h, p5/z, z3.h, z18.h
    31ec:	6e6f7361 	uabdl2	v1.4s, v27.8h, v15.8h
    31f0:	00657844 	.inst	0x00657844 ; undefined
    31f4:	6f747541 	uqshl	v1.2d, v10.2d, #52
    31f8:	2e6e6547 	umax	v7.4h, v10.4h, v14.4h
    31fc:	2f060063 	.inst	0x2f060063 ; undefined
    3200:	000f4240 	.inst	0x000f4240 ; undefined
    3204:	800fee0f 	.inst	0x800fee0f ; undefined
    3208:	0124f800 	.inst	0x0124f800 ; undefined
    320c:	00000000 	udf	#0
    3210:	6e655600 	urshl	v0.8h, v16.8h, v5.8h
    3214:	28726f64 	ldnp	w4, w27, [x27, #-112]
    3218:	30257830 	adr	x16, 0x4e11d
    321c:	2d295838 	stp	s24, s22, [x1, #-184]
    3220:	6e6f533e 	uabal2	v30.4s, v25.8h, v15.8h
    3224:	78302879 	.inst	0x78302879 ; undefined
    3228:	58383025 	ldr	x5, 0x7382c
    322c:	53000a29 	ubfx	w9, w17, #0, #3
    3230:	28796e6f 	ldnp	w15, w27, [x19, #-56]
    3234:	30257830 	adr	x16, 0x4e139
    3238:	2d295838 	stp	s24, s22, [x1, #-184]
    323c:	6e65563e 	urshl	v30.8h, v17.8h, v5.8h
    3240:	28726f64 	ldnp	w4, w27, [x27, #-112]
    3244:	30257830 	adr	x16, 0x4e149
    3248:	0a295838 	bic	w24, w1, w9, lsl #22
    324c:	00000000 	udf	#0
    3250:	000017c4 	udf	#6084
    3254:	00000000 	udf	#0
    3258:	0000184c 	udf	#6220
    325c:	00000000 	udf	#0
    3260:	00001744 	udf	#5956
    3264:	00000000 	udf	#0
    3268:	00001784 	udf	#6020
    326c:	00000000 	udf	#0
    3270:	0000184c 	udf	#6220
    3274:	00000000 	udf	#0
    3278:	0000184c 	udf	#6220
    327c:	00000000 	udf	#0
    3280:	000017c4 	udf	#6084
    3284:	00000000 	udf	#0
    3288:	0000184c 	udf	#6220
    328c:	00000000 	udf	#0
    3290:	0000184c 	udf	#6220
    3294:	00000000 	udf	#0
    3298:	0000184c 	udf	#6220
    329c:	00000000 	udf	#0
    32a0:	0000184c 	udf	#6220
    32a4:	00000000 	udf	#0
    32a8:	0000184c 	udf	#6220
    32ac:	00000000 	udf	#0
    32b0:	0000184c 	udf	#6220
    32b4:	00000000 	udf	#0
    32b8:	0000184c 	udf	#6220
    32bc:	00000000 	udf	#0
    32c0:	0000184c 	udf	#6220
    32c4:	00000000 	udf	#0
    32c8:	00001784 	udf	#6020
    32cc:	00000000 	udf	#0
    32d0:	0000184c 	udf	#6220
    32d4:	00000000 	udf	#0
    32d8:	00001820 	udf	#6176
    32dc:	00000000 	udf	#0
    32e0:	000017c4 	udf	#6084
    32e4:	00000000 	udf	#0
    32e8:	000017c4 	udf	#6084
    32ec:	00000000 	udf	#0
    32f0:	00001784 	udf	#6020
    32f4:	00000000 	udf	#0
    32f8:	0000184c 	udf	#6220
    32fc:	00000000 	udf	#0
    3300:	0000184c 	udf	#6220
    3304:	00000000 	udf	#0
    3308:	00001784 	udf	#6020
    330c:	00000000 	udf	#0
    3310:	75626544 	.inst	0x75626544 ; undefined
    3314:	62694c67 	.inst	0x62694c67 ; undefined
    3318:	4600632e 	.inst	0x4600632e ; undefined
    331c:	616d726f 	.inst	0x616d726f ; undefined
    3320:	3d212074 	str	b20, [x3, #2120]
    3324:	76282820 	.inst	0x76282820 ; undefined
    3328:	2064696f 	.inst	0x2064696f ; undefined
    332c:	3020292a 	adr	x10, 0x43851
    3330:	43280029 	.inst	0x43280029 ; undefined
    3334:	38524148 	ldurb	w8, [x10, #-220]
    3338:	42292a20 	.inst	0x42292a20 ; undefined
    333c:	4c657361 	.inst	0x4c657361 ; undefined
    3340:	4d747369 	.inst	0x4d747369 ; undefined
    3344:	656b7261 	fnmls	z1.h, p4/m, z19.h, z11.h
    3348:	3d3c2072 	str	b18, [x3, #3848]
    334c:	726f4620 	.inst	0x726f4620 ; undefined
    3350:	5374616d 	.inst	0x5374616d ; undefined
    3354:	6e697274 	uabdl2	v20.4s, v19.8h, v9.8h
    3358:	005b0067 	.inst	0x005b0067 ; undefined
    335c:	4200205d 	.inst	0x4200205d ; undefined
    3360:	65666675 	fnmls	z21.h, p1/m, z19.h, z6.h
    3364:	3d212072 	str	b18, [x3, #2120]
    3368:	76282820 	.inst	0x76282820 ; undefined
    336c:	2064696f 	.inst	0x2064696f ; undefined
    3370:	3020292a 	adr	x10, 0x43895
    3374:	61530029 	.inst	0x61530029 ; undefined
    3378:	74536566 	.inst	0x74536566 ; undefined
    337c:	676e6972 	.inst	0x676e6972 ; undefined
    3380:	2800632e 	stnp	w14, w24, [x25]
    3384:	4e495528 	.inst	0x4e495528 ; undefined
    3388:	20294e54 	.inst	0x20294e54 ; undefined
    338c:	69727453 	ldpsw	x19, x29, [x2, #-112]
    3390:	2620676e 	.inst	0x2620676e ; undefined
    3394:	30783020 	adr	x0, 0xf3999
    3398:	30303030 	adr	x16, 0x6399d
    339c:	29313030 	stp	w16, w12, [x1, #-120]
    33a0:	203d3d20 	.inst	0x203d3d20 ; undefined
    33a4:	28280030 	stnp	w16, w0, [x1, #-192]
    33a8:	544e4955 	bc.pl	0x9fcd0  // bc.nfrst
    33ac:	4420294e 	.inst	0x4420294e ; undefined
    33b0:	69747365 	ldpsw	x5, x28, [x27, #-96]
    33b4:	6974616e 	ldpsw	x14, x24, [x11, #-96]
    33b8:	26206e6f 	.inst	0x26206e6f ; undefined
    33bc:	30783020 	adr	x0, 0xf39c1
    33c0:	30303030 	adr	x16, 0x639c5
    33c4:	29313030 	stp	w16, w12, [x1, #-120]
    33c8:	203d3d20 	.inst	0x203d3d20 ; undefined
    33cc:	28280030 	stnp	w16, w0, [x1, #-192]
    33d0:	544e4955 	bc.pl	0x9fcf8  // bc.nfrst
    33d4:	5320294e 	.inst	0x5320294e ; undefined
    33d8:	6372756f 	.inst	0x6372756f ; undefined
    33dc:	20262065 	.inst	0x20262065 ; undefined
    33e0:	30307830 	adr	x16, 0x642e5
    33e4:	30303030 	adr	x16, 0x639e9
    33e8:	20293130 	.inst	0x20293130 ; undefined
    33ec:	30203d3d 	adr	x29, 0x43b91
    33f0:	65442800 	fmaxnmv	h0, p2, z0.h
    33f4:	6e697473 	uabd	v19.8h, v3.8h, v9.8h
    33f8:	6f697461 	uqshl	v1.2d, v3.2d, #41
    33fc:	3d21206e 	str	b14, [x3, #2120]
    3400:	76282820 	.inst	0x76282820 ; undefined
    3404:	2064696f 	.inst	0x2064696f ; undefined
    3408:	3020292a 	adr	x10, 0x4392d
    340c:	28002929 	stnp	w9, w10, [x9]
    3410:	72756f53 	.inst	0x72756f53 ; undefined
    3414:	21206563 	.inst	0x21206563 ; undefined
    3418:	2828203d 	stnp	w29, w8, [x1, #-192]
    341c:	64696f76 	.inst	0x64696f76 ; undefined
    3420:	20292a20 	.inst	0x20292a20 ; undefined
    3424:	00292930 	.inst	0x00292930 ; NYI
    3428:	73654428 	.inst	0x73654428 ; undefined
    342c:	78614d74 	.inst	0x78614d74 ; undefined
    3430:	203d3c20 	.inst	0x203d3c20 ; undefined
    3434:	50675f28 	adr	x8, 0xd201a
    3438:	465f6463 	.inst	0x465f6463 ; undefined
    343c:	64657869 	.inst	0x64657869 ; undefined
    3440:	75427441 	.inst	0x75427441 ; undefined
    3444:	5f646c69 	.inst	0x5f646c69 ; undefined
    3448:	4d646350 	.inst	0x4d646350 ; undefined
    344c:	6d697861 	ldp	d1, d30, [x3, #-368]
    3450:	6e556d75 	.inst	0x6e556d75 ; undefined
    3454:	646f6369 	.inst	0x646f6369 ; undefined
    3458:	72745365 	.inst	0x72745365 ; undefined
    345c:	4c676e69 	.inst	0x4c676e69 ; undefined
    3460:	74676e65 	.inst	0x74676e65 ; undefined
    3464:	00292968 	.inst	0x00292968 ; NYI
    3468:	73654428 	.inst	0x73654428 ; undefined
    346c:	78614d74 	.inst	0x78614d74 ; undefined
    3470:	203d2120 	.inst	0x203d2120 ; undefined
    3474:	28002930 	stnp	w16, w10, [x9]
    3478:	74736544 	.inst	0x74736544 ; undefined
    347c:	2078614d 	.inst	0x2078614d ; undefined
    3480:	6f53203e 	umlal2	v30.4s, v1.8h, v3.h[1]
    3484:	65637275 	fnmls	z21.h, p4/m, z19.h, z3.h
    3488:	296e654c 	ldp	w12, w25, [x10, #-144]
    348c:	746e4900 	.inst	0x746e4900 ; undefined
    3490:	616e7265 	.inst	0x616e7265 ; undefined
    3494:	6661536c 	.inst	0x6661536c ; undefined
    3498:	72745365 	.inst	0x72745365 ; undefined
    349c:	4e676e69 	smin	v9.8h, v19.8h, v7.8h
    34a0:	7274536f 	.inst	0x7274536f ; undefined
    34a4:	7265764f 	.inst	0x7265764f ; undefined
    34a8:	2070616c 	.inst	0x2070616c ; undefined
    34ac:	73654428 	.inst	0x73654428 ; undefined
    34b0:	616e6974 	.inst	0x616e6974 ; undefined
    34b4:	6e6f6974 	.inst	0x6e6f6974 ; undefined
    34b8:	6544202c 	fmaxnmv	h12, p0, z1.h
    34bc:	614d7473 	.inst	0x614d7473 ; undefined
    34c0:	28202c78 	stnp	w24, w11, [x3, #-256]
    34c4:	52414843 	.inst	0x52414843 ; undefined
    34c8:	2a203631 	orn	w17, w17, w0, lsl #13
    34cc:	756f5329 	.inst	0x756f5329 ; undefined
    34d0:	2c656372 	ldnp	s18, s24, [x27, #-216]
    34d4:	756f5320 	.inst	0x756f5320 ; undefined
    34d8:	4c656372 	.inst	0x4c656372 ; undefined
    34dc:	2b206e65 	adds	w5, w19, w0, uxtx #3
    34e0:	00293120 	.inst	0x00293120 ; NYI
    34e4:	6e654c28 	uqshl	v8.8h, v1.8h, v5.8h
    34e8:	20687467 	.inst	0x20687467 ; undefined
    34ec:	28203d3c 	stnp	w28, w15, [x9, #-256]
    34f0:	6350675f 	.inst	0x6350675f ; undefined
    34f4:	69465f64 	ldpsw	x4, x23, [x27, #48]
    34f8:	41646578 	.inst	0x41646578 ; undefined
    34fc:	69754274 	ldpsw	x20, x16, [x19, #-88]
    3500:	505f646c 	adr	x12, 0xc218e
    3504:	614d6463 	.inst	0x614d6463 ; undefined
    3508:	756d6978 	.inst	0x756d6978 ; undefined
    350c:	696e556d 	ldpsw	x13, x21, [x11, #-144]
    3510:	65646f63 	fnmls	z3.h, p3/m, z27.h, z4.h
    3514:	69727453 	ldpsw	x19, x29, [x2, #-112]
    3518:	654c676e 	fcmeq	p14.h, p1/z, z27.h, z12.h
    351c:	6874676e 	.inst	0x6874676e ; undefined
    3520:	28002929 	stnp	w9, w10, [x9]
    3524:	79706f43 	ldrh	w3, [x26, #6198]
    3528:	206e654c 	.inst	0x206e654c ; undefined
    352c:	30203d21 	adr	x1, 0x43cd1
    3530:	43280029 	.inst	0x43280029 ; undefined
    3534:	4c79706f 	.inst	0x4c79706f ; undefined
    3538:	3e206e65 	.inst	0x3e206e65 ; undefined
    353c:	756f5320 	.inst	0x756f5320 ; undefined
    3540:	4c656372 	.inst	0x4c656372 ; undefined
    3544:	00296e65 	.inst	0x00296e65 ; NYI
    3548:	73654428 	.inst	0x73654428 ; undefined
    354c:	78614d74 	.inst	0x78614d74 ; undefined
    3550:	203d3c20 	.inst	0x203d3c20 ; undefined
    3554:	50675f28 	adr	x8, 0xd213a
    3558:	465f6463 	.inst	0x465f6463 ; undefined
    355c:	64657869 	.inst	0x64657869 ; undefined
    3560:	75427441 	.inst	0x75427441 ; undefined
    3564:	5f646c69 	.inst	0x5f646c69 ; undefined
    3568:	4d646350 	.inst	0x4d646350 ; undefined
    356c:	6d697861 	ldp	d1, d30, [x3, #-368]
    3570:	73416d75 	.inst	0x73416d75 ; undefined
    3574:	53696963 	.inst	0x53696963 ; undefined
    3578:	6e697274 	uabdl2	v20.4s, v19.8h, v9.8h
    357c:	6e654c67 	uqshl	v7.8h, v3.8h, v5.8h
    3580:	29687467 	ldp	w7, w29, [x3, #-192]
    3584:	6e490029 	.inst	0x6e490029 ; undefined
    3588:	6e726574 	umax	v20.8h, v11.8h, v18.8h
    358c:	61536c61 	.inst	0x61536c61 ; undefined
    3590:	74536566 	.inst	0x74536566 ; undefined
    3594:	676e6972 	.inst	0x676e6972 ; undefined
    3598:	73416f4e 	.inst	0x73416f4e ; undefined
    359c:	53696963 	.inst	0x53696963 ; undefined
    35a0:	764f7274 	.inst	0x764f7274 ; undefined
    35a4:	616c7265 	.inst	0x616c7265 ; undefined
    35a8:	44282070 	.inst	0x44282070 ; undefined
    35ac:	69747365 	ldpsw	x5, x28, [x27, #-96]
    35b0:	6974616e 	ldpsw	x14, x24, [x11, #-96]
    35b4:	202c6e6f 	.inst	0x202c6e6f ; undefined
    35b8:	74736544 	.inst	0x74736544 ; undefined
    35bc:	2c78614d 	ldnp	s13, s24, [x10, #-64]
    35c0:	48432820 	ldxrh	w0, [x1]
    35c4:	20385241 	.inst	0x20385241 ; undefined
    35c8:	6f53292a 	umlal2	v10.4s, v9.8h, v3.h[5]
    35cc:	65637275 	fnmls	z21.h, p4/m, z19.h, z3.h
    35d0:	6f53202c 	umlal2	v12.4s, v1.8h, v3.h[1]
    35d4:	65637275 	fnmls	z21.h, p4/m, z19.h, z3.h
    35d8:	206e654c 	.inst	0x206e654c ; undefined
    35dc:	2931202b 	stp	w11, w8, [x1, #-120]
    35e0:	654c2800 	.inst	0x654c2800 ; undefined
    35e4:	6874676e 	.inst	0x6874676e ; undefined
    35e8:	203d3c20 	.inst	0x203d3c20 ; undefined
    35ec:	50675f28 	adr	x8, 0xd21d2
    35f0:	465f6463 	.inst	0x465f6463 ; undefined
    35f4:	64657869 	.inst	0x64657869 ; undefined
    35f8:	75427441 	.inst	0x75427441 ; undefined
    35fc:	5f646c69 	.inst	0x5f646c69 ; undefined
    3600:	4d646350 	.inst	0x4d646350 ; undefined
    3604:	6d697861 	ldp	d1, d30, [x3, #-368]
    3608:	73416d75 	.inst	0x73416d75 ; undefined
    360c:	53696963 	.inst	0x53696963 ; undefined
    3610:	6e697274 	uabdl2	v20.4s, v19.8h, v9.8h
    3614:	6e654c67 	uqshl	v7.8h, v3.8h, v5.8h
    3618:	29687467 	ldp	w7, w29, [x3, #-192]
    361c:	65440029 	fadd	z9.h, z1.h, z4.h
    3620:	6e697473 	uabd	v19.8h, v3.8h, v9.8h
    3624:	6f697461 	uqshl	v1.2d, v3.2d, #41
    3628:	3d21206e 	str	b14, [x3, #2120]
    362c:	76282820 	.inst	0x76282820 ; undefined
    3630:	2064696f 	.inst	0x2064696f ; undefined
    3634:	3020292a 	adr	x10, 0x43b59
    3638:	6f530029 	mla	v9.8h, v1.8h, v3.h[1]
    363c:	65637275 	fnmls	z21.h, p4/m, z19.h, z3.h
    3640:	203d2120 	.inst	0x203d2120 ; undefined
    3644:	74736544 	.inst	0x74736544 ; undefined
    3648:	74616e69 	.inst	0x74616e69 ; undefined
    364c:	006e6f69 	.inst	0x006e6f69 ; undefined
    3650:	4e495528 	.inst	0x4e495528 ; undefined
    3654:	20294e54 	.inst	0x20294e54 ; undefined
    3658:	44282028 	.inst	0x44282028 ; undefined
    365c:	69747365 	ldpsw	x5, x28, [x27, #-96]
    3660:	6974616e 	ldpsw	x14, x24, [x11, #-96]
    3664:	2b206e6f 	adds	w15, w19, w0, uxtx #3
    3668:	6e654c20 	uqshl	v0.8h, v1.8h, v5.8h
    366c:	20687467 	.inst	0x20687467 ; undefined
    3670:	2931202b 	stp	w11, w8, [x1, #-120]
    3674:	53203c20 	.inst	0x53203c20 ; undefined
    3678:	6372756f 	.inst	0x6372756f ; undefined
    367c:	00292065 	.inst	0x00292065 ; NYI
    3680:	4e495528 	.inst	0x4e495528 ; undefined
    3684:	20294e54 	.inst	0x20294e54 ; undefined
    3688:	53282028 	.inst	0x53282028 ; undefined
    368c:	6372756f 	.inst	0x6372756f ; undefined
    3690:	202b2065 	.inst	0x202b2065 ; undefined
    3694:	676e654c 	.inst	0x676e654c ; undefined
    3698:	2b206874 	adds	w20, w3, w0, uxtx #2
    369c:	20293120 	.inst	0x20293120 ; undefined
    36a0:	6544203c 	fmaxnmv	h28, p0, z1.h
    36a4:	6e697473 	uabd	v19.8h, v3.8h, v9.8h
    36a8:	6f697461 	uqshl	v1.2d, v3.2d, #41
    36ac:	0029206e 	.inst	0x0029206e ; NYI
    36b0:	4e495528 	.inst	0x4e495528 ; undefined
    36b4:	20294e54 	.inst	0x20294e54 ; undefined
    36b8:	44282028 	.inst	0x44282028 ; undefined
    36bc:	69747365 	ldpsw	x5, x28, [x27, #-96]
    36c0:	6974616e 	ldpsw	x14, x24, [x11, #-96]
    36c4:	2b206e6f 	adds	w15, w19, w0, uxtx #3
    36c8:	6e654c20 	uqshl	v0.8h, v1.8h, v5.8h
    36cc:	29687467 	ldp	w7, w29, [x3, #-192]
    36d0:	53203c20 	.inst	0x53203c20 ; undefined
    36d4:	6372756f 	.inst	0x6372756f ; undefined
    36d8:	28002965 	stnp	w5, w10, [x11]
    36dc:	544e4955 	bc.pl	0xa0004  // bc.nfrst
    36e0:	2820294e 	stnp	w14, w10, [x10, #-256]
    36e4:	6f532820 	umlal2	v0.4s, v1.8h, v3.h[5]
    36e8:	65637275 	fnmls	z21.h, p4/m, z19.h, z3.h
    36ec:	53202b20 	.inst	0x53202b20 ; undefined
    36f0:	654c7274 	fcmne	p4.h, p4/z, z19.h, z12.h
    36f4:	6f53286e 	umlal2	v14.4s, v3.8h, v3.h[5]
    36f8:	65637275 	fnmls	z21.h, p4/m, z19.h, z3.h
    36fc:	3c202929 	.inst	0x3c202929 ; undefined
    3700:	73654420 	.inst	0x73654420 ; undefined
    3704:	616e6974 	.inst	0x616e6974 ; undefined
    3708:	6e6f6974 	.inst	0x6e6f6974 ; undefined
    370c:	4c002920 	st1	{v0.4s-v3.4s}, [x9]
    3710:	74676e65 	.inst	0x74676e65 ; undefined
    3714:	3d3c2068 	str	b8, [x3, #3848]
    3718:	50675f20 	adr	x0, 0xd22fe
    371c:	465f6463 	.inst	0x465f6463 ; undefined
    3720:	64657869 	.inst	0x64657869 ; undefined
    3724:	75427441 	.inst	0x75427441 ; undefined
    3728:	5f646c69 	.inst	0x5f646c69 ; undefined
    372c:	4d646350 	.inst	0x4d646350 ; undefined
    3730:	6d697861 	ldp	d1, d30, [x3, #-368]
    3734:	6e556d75 	.inst	0x6e556d75 ; undefined
    3738:	646f6369 	.inst	0x646f6369 ; undefined
    373c:	72745365 	.inst	0x72745365 ; undefined
    3740:	4c676e69 	.inst	0x4c676e69 ; undefined
    3744:	74676e65 	.inst	0x74676e65 ; undefined
    3748:	74530068 	.inst	0x74530068 ; undefined
    374c:	676e6972 	.inst	0x676e6972 ; undefined
    3750:	203d2120 	.inst	0x203d2120 ; undefined
    3754:	6f762828 	umlal2	v8.4s, v1.8h, v6.h[7]
    3758:	2a206469 	orn	w9, w3, w0, lsl #25
    375c:	29302029 	stp	w9, w8, [x1, #-128]
    3760:	6e654c00 	uqshl	v0.8h, v0.8h, v5.8h
    3764:	20687467 	.inst	0x20687467 ; undefined
    3768:	675f203c 	.inst	0x675f203c ; undefined
    376c:	5f646350 	.inst	0x5f646350 ; undefined
    3770:	65786946 	fnmls	z6.h, p2/m, z10.h, z24.h
    3774:	42744164 	.inst	0x42744164 ; undefined
    3778:	646c6975 	.inst	0x646c6975 ; undefined
    377c:	6463505f 	.inst	0x6463505f ; undefined
    3780:	6978614d 	ldpsw	x13, x24, [x10, #-64]
    3784:	556d756d 	.inst	0x556d756d ; undefined
    3788:	6f63696e 	umlsl2	v14.4s, v11.8h, v3.h[6]
    378c:	74536564 	.inst	0x74536564 ; undefined
    3790:	676e6972 	.inst	0x676e6972 ; undefined
    3794:	676e654c 	.inst	0x676e654c ; undefined
    3798:	53006874 	ubfx	w20, w3, #0, #27
    379c:	69537274 	ldpsw	x20, x28, [x19, #152]
    37a0:	2820657a 	stnp	w26, w25, [x11, #-256]
    37a4:	73726946 	.inst	0x73726946 ; undefined
    37a8:	72745374 	.inst	0x72745374 ; undefined
    37ac:	29676e69 	ldp	w9, w27, [x19, #-200]
    37b0:	203d2120 	.inst	0x203d2120 ; undefined
    37b4:	74530030 	.inst	0x74530030 ; undefined
    37b8:	7a695372 	.inst	0x7a695372 ; undefined
    37bc:	53282065 	.inst	0x53282065 ; undefined
    37c0:	6e6f6365 	rsubhn2	v5.8h, v27.4s, v15.4s
    37c4:	72745364 	.inst	0x72745364 ; undefined
    37c8:	29676e69 	ldp	w9, w27, [x19, #-200]
    37cc:	203d2120 	.inst	0x203d2120 ; undefined
    37d0:	65520030 	fadd	z16.h, z1.h, z18.h
    37d4:	746c7573 	.inst	0x746c7573 ; undefined
    37d8:	203d3c20 	.inst	0x203d3c20 ; undefined
    37dc:	28282828 	stnp	w8, w10, [x1, #-192]
    37e0:	544e4955 	bc.pl	0xa0108  // bc.nfrst
    37e4:	7e20294e 	.inst	0x7e20294e ; undefined
    37e8:	2d202930 	stp	s16, s10, [x9, #-256]
    37ec:	532a2820 	.inst	0x532a2820 ; undefined
    37f0:	6e697274 	uabdl2	v20.4s, v19.8h, v9.8h
    37f4:	202d2067 	.inst	0x202d2067 ; undefined
    37f8:	2730274c 	.inst	0x2730274c ; undefined
    37fc:	2f202929 	.inst	0x2f202929 ; undefined
    3800:	29303120 	stp	w0, w12, [x9, #-128]
    3804:	73655200 	.inst	0x73655200 ; undefined
    3808:	20746c75 	.inst	0x20746c75 ; undefined
    380c:	44203d3c 	.inst	0x44203d3c ; undefined
    3810:	36557669 	tbz	w9, #10, 0xffffffffffffe6dc
    3814:	32337834 	orr	w20, w1, #0xffffefff
    3818:	28282820 	stnp	w0, w10, [x1, #-192]
    381c:	544e4955 	bc.pl	0xa0144  // bc.nfrst
    3820:	20293436 	.inst	0x20293436 ; undefined
    3824:	2029307e 	.inst	0x2029307e ; undefined
    3828:	2a28202d 	orn	w13, w1, w8, lsl #8
    382c:	69727453 	ldpsw	x19, x29, [x2, #-112]
    3830:	2d20676e 	stp	s14, s25, [x27, #-256]
    3834:	30274c20 	adr	x0, 0x521b9
    3838:	2c202927 	stnp	s7, s10, [x9, #-256]
    383c:	29303120 	stp	w0, w12, [x9, #-128]
    3840:	73655200 	.inst	0x73655200 ; undefined
    3844:	20746c75 	.inst	0x20746c75 ; undefined
    3848:	28203d3c 	stnp	w28, w15, [x9, #-256]
    384c:	55282828 	.inst	0x55282828 ; undefined
    3850:	4e544e49 	.inst	0x4e544e49 ; undefined
    3854:	307e2029 	adr	x9, 0xffc59
    3858:	202d2029 	.inst	0x202d2029 ; undefined
    385c:	65746e49 	fnmls	z9.h, p3/m, z18.h, z20.h
    3860:	6c616e72 	ldnp	d18, d27, [x19, #-496]
    3864:	43786548 	.inst	0x43786548 ; undefined
    3868:	54726168 	b.hi	0xe8494  // b.pmore
    386c:	6e69556f 	urshl	v15.8h, v11.8h, v9.8h
    3870:	28206e74 	stnp	w20, w27, [x19, #-256]
    3874:	7274532a 	.inst	0x7274532a ; undefined
    3878:	29676e69 	ldp	w9, w27, [x19, #-200]
    387c:	3e3e2029 	.inst	0x3e3e2029 ; undefined
    3880:	00293420 	.inst	0x00293420 ; NYI
    3884:	7453282a 	.inst	0x7453282a ; undefined
    3888:	676e6972 	.inst	0x676e6972 ; undefined
    388c:	31202d20 	adds	w0, w9, #0x80b
    3890:	3d3d2029 	str	b9, [x1, #3912]
    3894:	30274c20 	adr	x0, 0x52219
    3898:	65520027 	fadd	z7.h, z1.h, z18.h
    389c:	746c7573 	.inst	0x746c7573 ; undefined
    38a0:	203d3c20 	.inst	0x203d3c20 ; undefined
    38a4:	69685352 	ldpsw	x18, x20, [x26, #-192]
    38a8:	36557466 	tbz	w6, #10, 0xffffffffffffe734
    38ac:	28282034 	stnp	w20, w8, [x1, #-192]
    38b0:	4e495528 	.inst	0x4e495528 ; undefined
    38b4:	29343654 	stp	w20, w13, [x18, #-96]
    38b8:	29307e20 	stp	w0, wzr, [x17, #-128]
    38bc:	49202d20 	.inst	0x49202d20 ; undefined
    38c0:	7265746e 	.inst	0x7265746e ; undefined
    38c4:	486c616e 	.inst	0x486c616e ; undefined
    38c8:	68437865 	.inst	0x68437865 ; undefined
    38cc:	6f547261 	fcmla	v1.8h, v19.8h, v20.h[0], #270
    38d0:	746e6955 	.inst	0x746e6955 ; undefined
    38d4:	2a28206e 	orn	w14, w3, w8, lsl #8
    38d8:	69727453 	ldpsw	x19, x29, [x2, #-112]
    38dc:	2029676e 	.inst	0x2029676e ; undefined
    38e0:	2934202c 	stp	w12, w8, [x1, #-96]
    38e4:	49552800 	.inst	0x49552800 ; undefined
    38e8:	294e544e 	ldp	w14, w21, [x2, #112]
    38ec:	43282820 	.inst	0x43282820 ; undefined
    38f0:	38524148 	ldurb	w8, [x10, #-220]
    38f4:	5320292a 	.inst	0x5320292a ; undefined
    38f8:	6372756f 	.inst	0x6372756f ; undefined
    38fc:	3d212065 	str	b5, [x3, #2120]
    3900:	48432820 	ldxrh	w0, [x1]
    3904:	2a385241 	orn	w1, w18, w24, lsl #20
    3908:	65442029 	fmaxnmv	h9, p0, z1.h
    390c:	6e697473 	uabd	v19.8h, v3.8h, v9.8h
    3910:	6f697461 	uqshl	v1.2d, v3.2d, #41
    3914:	2800296e 	stnp	w14, w10, [x11]
    3918:	544e4955 	bc.pl	0xa0240  // bc.nfrst
    391c:	2820294e 	stnp	w14, w10, [x10, #-256]
    3920:	65442820 	fmaxnmv	h0, p2, z1.h
    3924:	6e697473 	uabd	v19.8h, v3.8h, v9.8h
    3928:	6f697461 	uqshl	v1.2d, v3.2d, #41
    392c:	202b206e 	.inst	0x202b206e ; undefined
    3930:	676e654c 	.inst	0x676e654c ; undefined
    3934:	2b206874 	adds	w20, w3, w0, uxtx #2
    3938:	20293120 	.inst	0x20293120 ; undefined
    393c:	2028203c 	.inst	0x2028203c ; undefined
    3940:	41484328 	.inst	0x41484328 ; undefined
    3944:	292a3852 	stp	w18, w14, [x2, #-176]
    3948:	72756f53 	.inst	0x72756f53 ; undefined
    394c:	29206563 	stp	w3, w25, [x11, #-256]
    3950:	28002920 	stnp	w0, w10, [x9]
    3954:	544e4955 	bc.pl	0xa027c  // bc.nfrst
    3958:	2820294e 	stnp	w14, w10, [x10, #-256]
    395c:	48432820 	ldxrh	w0, [x1]
    3960:	2a385241 	orn	w1, w18, w24, lsl #20
    3964:	53282029 	.inst	0x53282029 ; undefined
    3968:	6372756f 	.inst	0x6372756f ; undefined
    396c:	202b2065 	.inst	0x202b2065 ; undefined
    3970:	676e654c 	.inst	0x676e654c ; undefined
    3974:	2b206874 	adds	w20, w3, w0, uxtx #2
    3978:	20293120 	.inst	0x20293120 ; undefined
    397c:	6544203c 	fmaxnmv	h28, p0, z1.h
    3980:	6e697473 	uabd	v19.8h, v3.8h, v9.8h
    3984:	6f697461 	uqshl	v1.2d, v3.2d, #41
    3988:	0029206e 	.inst	0x0029206e ; NYI
    398c:	756f532a 	.inst	0x756f532a ; undefined
    3990:	20656372 	.inst	0x20656372 ; undefined
    3994:	7830203c 	ldeorh	w16, w28, [x1]
    3998:	00303031 	.inst	0x00303031 ; NYI
    399c:	69637341 	ldpsw	x1, x28, [x26, #-232]
    39a0:	72745369 	.inst	0x72745369 ; undefined
    39a4:	657a6953 	fnmls	z19.h, p2/m, z10.h, z26.h
    39a8:	65522820 	fcmeq	p0.h, p2/z, z1.h, #0.0
    39ac:	6e727574 	uabd	v20.8h, v11.8h, v18.8h
    39b0:	756c6156 	.inst	0x756c6156 ; undefined
    39b4:	21202965 	.inst	0x21202965 ; undefined
    39b8:	0030203d 	.inst	0x0030203d ; NYI
    39bc:	4e495528 	.inst	0x4e495528 ; undefined
    39c0:	20294e54 	.inst	0x20294e54 ; undefined
    39c4:	44282028 	.inst	0x44282028 ; undefined
    39c8:	69747365 	ldpsw	x5, x28, [x27, #-96]
    39cc:	6974616e 	ldpsw	x14, x24, [x11, #-96]
    39d0:	2b206e6f 	adds	w15, w19, w0, uxtx #3
    39d4:	6e654c20 	uqshl	v0.8h, v1.8h, v5.8h
    39d8:	29687467 	ldp	w7, w29, [x3, #-192]
    39dc:	53203c20 	.inst	0x53203c20 ; undefined
    39e0:	6372756f 	.inst	0x6372756f ; undefined
    39e4:	00292065 	.inst	0x00292065 ; NYI
    39e8:	4e495528 	.inst	0x4e495528 ; undefined
    39ec:	20294e54 	.inst	0x20294e54 ; undefined
    39f0:	53282028 	.inst	0x53282028 ; undefined
    39f4:	6372756f 	.inst	0x6372756f ; undefined
    39f8:	202b2065 	.inst	0x202b2065 ; undefined
    39fc:	676e654c 	.inst	0x676e654c ; undefined
    3a00:	20296874 	.inst	0x20296874 ; undefined
    3a04:	6544203c 	fmaxnmv	h28, p0, z1.h
    3a08:	6e697473 	uabd	v19.8h, v3.8h, v9.8h
    3a0c:	6f697461 	uqshl	v1.2d, v3.2d, #41
    3a10:	0029206e 	.inst	0x0029206e ; NYI
    3a14:	676e654c 	.inst	0x676e654c ; undefined
    3a18:	3c206874 	str	b20, [x3, x0]
    3a1c:	675f203d 	.inst	0x675f203d ; undefined
    3a20:	5f646350 	.inst	0x5f646350 ; undefined
    3a24:	65786946 	fnmls	z6.h, p2/m, z10.h, z24.h
    3a28:	42744164 	.inst	0x42744164 ; undefined
    3a2c:	646c6975 	.inst	0x646c6975 ; undefined
    3a30:	6463505f 	.inst	0x6463505f ; undefined
    3a34:	6978614d 	ldpsw	x13, x24, [x10, #-64]
    3a38:	416d756d 	.inst	0x416d756d ; undefined
    3a3c:	69696373 	ldpsw	x19, x24, [x27, #-184]
    3a40:	69727453 	ldpsw	x19, x29, [x2, #-112]
    3a44:	654c676e 	fcmeq	p14.h, p1/z, z27.h, z12.h
    3a48:	6874676e 	.inst	0x6874676e ; undefined
    3a4c:	6e654c00 	uqshl	v0.8h, v0.8h, v5.8h
    3a50:	20687467 	.inst	0x20687467 ; undefined
    3a54:	675f203c 	.inst	0x675f203c ; undefined
    3a58:	5f646350 	.inst	0x5f646350 ; undefined
    3a5c:	65786946 	fnmls	z6.h, p2/m, z10.h, z24.h
    3a60:	42744164 	.inst	0x42744164 ; undefined
    3a64:	646c6975 	.inst	0x646c6975 ; undefined
    3a68:	6463505f 	.inst	0x6463505f ; undefined
    3a6c:	6978614d 	ldpsw	x13, x24, [x10, #-64]
    3a70:	416d756d 	.inst	0x416d756d ; undefined
    3a74:	69696373 	ldpsw	x19, x24, [x27, #-184]
    3a78:	69727453 	ldpsw	x19, x29, [x2, #-112]
    3a7c:	654c676e 	fcmeq	p14.h, p1/z, z27.h, z12.h
    3a80:	6874676e 	.inst	0x6874676e ; undefined
    3a84:	63734100 	.inst	0x63734100 ; undefined
    3a88:	74536969 	.inst	0x74536969 ; undefined
    3a8c:	7a695372 	.inst	0x7a695372 ; undefined
    3a90:	46282065 	.inst	0x46282065 ; undefined
    3a94:	74737269 	.inst	0x74737269 ; undefined
    3a98:	69727453 	ldpsw	x19, x29, [x2, #-112]
    3a9c:	0029676e 	.inst	0x0029676e ; NYI
    3aa0:	69637341 	ldpsw	x1, x28, [x26, #-232]
    3aa4:	72745369 	.inst	0x72745369 ; undefined
    3aa8:	657a6953 	fnmls	z19.h, p2/m, z10.h, z26.h
    3aac:	65532820 	fcmne	p0.h, p2/z, z1.h, #0.0
    3ab0:	646e6f63 	.inst	0x646e6f63 ; undefined
    3ab4:	69727453 	ldpsw	x19, x29, [x2, #-112]
    3ab8:	0029676e 	.inst	0x0029676e ; NYI
    3abc:	69637341 	ldpsw	x1, x28, [x26, #-232]
    3ac0:	72745369 	.inst	0x72745369 ; undefined
    3ac4:	657a6953 	fnmls	z19.h, p2/m, z10.h, z26.h
    3ac8:	65442820 	fmaxnmv	h0, p2, z1.h
    3acc:	6e697473 	uabd	v19.8h, v3.8h, v9.8h
    3ad0:	6f697461 	uqshl	v1.2d, v3.2d, #41
    3ad4:	2120296e 	.inst	0x2120296e ; undefined
    3ad8:	0030203d 	.inst	0x0030203d ; NYI
    3adc:	69637341 	ldpsw	x1, x28, [x26, #-232]
    3ae0:	72745369 	.inst	0x72745369 ; undefined
    3ae4:	657a6953 	fnmls	z19.h, p2/m, z10.h, z26.h
    3ae8:	74532820 	.inst	0x74532820 ; undefined
    3aec:	676e6972 	.inst	0x676e6972 ; undefined
    3af0:	3d212029 	str	b9, [x1, #2120]
    3af4:	41003020 	.inst	0x41003020 ; undefined
    3af8:	69696373 	ldpsw	x19, x24, [x27, #-184]
    3afc:	53727453 	.inst	0x53727453 ; undefined
    3b00:	20657a69 	.inst	0x20657a69 ; undefined
    3b04:	61655328 	.inst	0x61655328 ; undefined
    3b08:	53686372 	.inst	0x53686372 ; undefined
    3b0c:	6e697274 	uabdl2	v20.4s, v19.8h, v9.8h
    3b10:	21202967 	.inst	0x21202967 ; undefined
    3b14:	0030203d 	.inst	0x0030203d ; NYI
    3b18:	7453282a 	.inst	0x7453282a ; undefined
    3b1c:	676e6972 	.inst	0x676e6972 ; undefined
    3b20:	31202d20 	adds	w0, w9, #0x80b
    3b24:	3d3d2029 	str	b9, [x1, #3912]
    3b28:	27302720 	.inst	0x27302720 ; undefined
    3b2c:	63734100 	.inst	0x63734100 ; undefined
    3b30:	74536969 	.inst	0x74536969 ; undefined
    3b34:	7a695372 	.inst	0x7a695372 ; undefined
    3b38:	53282065 	.inst	0x53282065 ; undefined
    3b3c:	6372756f 	.inst	0x6372756f ; undefined
    3b40:	21202965 	.inst	0x21202965 ; undefined
    3b44:	0030203d 	.inst	0x0030203d ; NYI
    3b48:	4e495528 	.inst	0x4e495528 ; undefined
    3b4c:	20294e54 	.inst	0x20294e54 ; undefined
    3b50:	48432828 	ldxrh	w8, [x1]
    3b54:	20385241 	.inst	0x20385241 ; undefined
    3b58:	4420292a 	.inst	0x4420292a ; undefined
    3b5c:	69747365 	ldpsw	x5, x28, [x27, #-96]
    3b60:	6974616e 	ldpsw	x14, x24, [x11, #-96]
    3b64:	2d206e6f 	stp	s15, s27, [x19, #-256]
    3b68:	756f5320 	.inst	0x756f5320 ; undefined
    3b6c:	29656372 	ldp	w18, w24, [x27, #-216]
    3b70:	41203e20 	.inst	0x41203e20 ; undefined
    3b74:	69696373 	ldpsw	x19, x24, [x27, #-184]
    3b78:	4c727453 	.inst	0x4c727453 ; undefined
    3b7c:	28206e65 	stnp	w5, w27, [x19, #-256]
    3b80:	72756f53 	.inst	0x72756f53 ; undefined
    3b84:	00296563 	.inst	0x00296563 ; NYI
    3b88:	4e495528 	.inst	0x4e495528 ; undefined
    3b8c:	20294e54 	.inst	0x20294e54 ; undefined
    3b90:	756f5328 	.inst	0x756f5328 ; undefined
    3b94:	20656372 	.inst	0x20656372 ; undefined
    3b98:	4328202d 	.inst	0x4328202d ; undefined
    3b9c:	38524148 	ldurb	w8, [x10, #-220]
    3ba0:	20292a20 	.inst	0x20292a20 ; undefined
    3ba4:	74736544 	.inst	0x74736544 ; undefined
    3ba8:	74616e69 	.inst	0x74616e69 ; undefined
    3bac:	296e6f69 	ldp	w9, w27, [x27, #-144]
    3bb0:	203d3e20 	.inst	0x203d3e20 ; undefined
    3bb4:	63734128 	.inst	0x63734128 ; undefined
    3bb8:	74536969 	.inst	0x74536969 ; undefined
    3bbc:	7a695372 	.inst	0x7a695372 ; undefined
    3bc0:	53282065 	.inst	0x53282065 ; undefined
    3bc4:	6372756f 	.inst	0x6372756f ; undefined
    3bc8:	2a202965 	orn	w5, w11, w0, lsl #10
    3bcc:	7a697320 	.inst	0x7a697320 ; undefined
    3bd0:	20666f65 	.inst	0x20666f65 ; undefined
    3bd4:	41484328 	.inst	0x41484328 ; undefined
    3bd8:	29363152 	stp	w18, w12, [x10, #-80]
    3bdc:	61560029 	.inst	0x61560029 ; undefined
    3be0:	2065756c 	.inst	0x2065756c ; undefined
    3be4:	3031203c 	adr	x28, 0x65fe9
    3be8:	61560030 	.inst	0x61560030 ; undefined
    3bec:	2065756c 	.inst	0x2065756c ; undefined
    3bf0:	7830203c 	ldeorh	w16, w28, [x1]
    3bf4:	28003061 	stnp	w1, w12, [x3]
    3bf8:	756c6156 	.inst	0x756c6156 ; undefined
    3bfc:	20262065 	.inst	0x20262065 ; undefined
    3c00:	29667830 	ldp	w16, w30, [x1, #-208]
    3c04:	30203c20 	adr	x0, 0x44389
    3c08:	55006178 	.inst	0x55006178 ; undefined
    3c0c:	42696665 	.inst	0x42696665 ; undefined
    3c10:	53746f6f 	.inst	0x53746f6f ; undefined
    3c14:	69767265 	ldpsw	x5, x28, [x19, #-80]
    3c18:	54736563 	b.cc	0xea8c4  // b.lo, b.ul, b.last
    3c1c:	656c6261 	fnmls	z1.h, p0/m, z19.h, z12.h
    3c20:	2e62694c 	.inst	0x2e62694c ; undefined
    3c24:	49670063 	.inst	0x49670063 ; undefined
    3c28:	6567616d 	fnmls	z13.h, p0/m, z11.h, z7.h
    3c2c:	646e6148 	.inst	0x646e6148 ; undefined
    3c30:	2120656c 	.inst	0x2120656c ; undefined
    3c34:	2828203d 	stnp	w29, w8, [x1, #-192]
    3c38:	64696f76 	.inst	0x64696f76 ; undefined
    3c3c:	20292a20 	.inst	0x20292a20 ; undefined
    3c40:	67002930 	.inst	0x67002930 ; undefined
    3c44:	21205453 	.inst	0x21205453 ; undefined
    3c48:	2828203d 	stnp	w29, w8, [x1, #-192]
    3c4c:	64696f76 	.inst	0x64696f76 ; undefined
    3c50:	20292a20 	.inst	0x20292a20 ; undefined
    3c54:	67002930 	.inst	0x67002930 ; undefined
    3c58:	21205342 	.inst	0x21205342 ; undefined
    3c5c:	2828203d 	stnp	w29, w8, [x1, #-192]
    3c60:	64696f76 	.inst	0x64696f76 ; undefined
    3c64:	20292a20 	.inst	0x20292a20 ; undefined
    3c68:	55002930 	.inst	0x55002930 ; undefined
    3c6c:	52696665 	.inst	0x52696665 ; undefined
    3c70:	69746e75 	ldpsw	x21, x27, [x19, #-96]
    3c74:	6553656d 	fcmeq	p13.h, p1/z, z11.h, z19.h
    3c78:	63697672 	.inst	0x63697672 ; undefined
    3c7c:	61547365 	.inst	0x61547365 ; undefined
    3c80:	4c656c62 	.inst	0x4c656c62 ; undefined
    3c84:	632e6269 	.inst	0x632e6269 ; undefined
    3c88:	54526700 	b.eq	0xa8968  // b.none
    3c8c:	203d2120 	.inst	0x203d2120 ; undefined
    3c90:	6f762828 	umlal2	v8.4s, v1.8h, v6.h[7]
    3c94:	2a206469 	orn	w9, w3, w0, lsl #25
    3c98:	29302029 	stp	w9, w8, [x1, #-128]
    3c9c:	66655500 	.inst	0x66655500 ; undefined
    3ca0:	62694c69 	.inst	0x62694c69 ; undefined
    3ca4:	5400632e 	b.al	0x4908
    3ca8:	656c6261 	fnmls	z1.h, p0/m, z19.h, z12.h
    3cac:	64697547 	.inst	0x64697547 ; undefined
    3cb0:	203d2120 	.inst	0x203d2120 ; undefined
    3cb4:	6f762828 	umlal2	v8.4s, v1.8h, v6.h[7]
    3cb8:	2a206469 	orn	w9, w3, w0, lsl #25
    3cbc:	29302029 	stp	w9, w8, [x1, #-128]
    3cc0:	62615400 	.inst	0x62615400 ; undefined
    3cc4:	2120656c 	.inst	0x2120656c ; undefined
    3cc8:	2828203d 	stnp	w29, w8, [x1, #-192]
    3ccc:	64696f76 	.inst	0x64696f76 ; undefined
    3cd0:	20292a20 	.inst	0x20292a20 ; undefined
    3cd4:	50002930 	adr	x16, 0x41fa
    3cd8:	6f746f72 	.inst	0x6f746f72 ; undefined
    3cdc:	476c6f63 	.inst	0x476c6f63 ; undefined
    3ce0:	20646975 	.inst	0x20646975 ; undefined
    3ce4:	28203d21 	stnp	w1, w15, [x9, #-256]
    3ce8:	696f7628 	ldpsw	x8, x29, [x17, #-136]
    3cec:	292a2064 	stp	w4, w8, [x3, #-176]
    3cf0:	00293020 	.inst	0x00293020 ; NYI
    3cf4:	69746f4e 	ldpsw	x14, x27, [x26, #-96]
    3cf8:	75467966 	.inst	0x75467966 ; undefined
    3cfc:	6974636e 	ldpsw	x14, x24, [x27, #-96]
    3d00:	21206e6f 	.inst	0x21206e6f ; undefined
    3d04:	2828203d 	stnp	w29, w8, [x1, #-192]
    3d08:	64696f76 	.inst	0x64696f76 ; undefined
    3d0c:	20292a20 	.inst	0x20292a20 ; undefined
    3d10:	52002930 	eor	w16, w9, #0x7ff
    3d14:	73696765 	.inst	0x73696765 ; undefined
    3d18:	74617274 	.inst	0x74617274 ; undefined
    3d1c:	206e6f69 	.inst	0x206e6f69 ; undefined
    3d20:	28203d21 	stnp	w1, w15, [x9, #-256]
    3d24:	696f7628 	ldpsw	x8, x29, [x17, #-136]
    3d28:	292a2064 	stp	w4, w8, [x3, #-176]
    3d2c:	00293020 	.inst	0x00293020 ; NYI
    3d30:	656d614e 	fnmls	z14.h, p0/m, z10.h, z13.h
    3d34:	203d2120 	.inst	0x203d2120 ; undefined
    3d38:	6f762828 	umlal2	v8.4s, v1.8h, v6.h[7]
    3d3c:	2a206469 	orn	w9, w3, w0, lsl #25
    3d40:	29302029 	stp	w9, w8, [x1, #-128]
    3d44:	746f4e00 	.inst	0x746f4e00 ; undefined
    3d48:	54796669 	b.ls	0xf6a14  // b.plast
    3d4c:	3c206c70 	.inst	0x3c206c70 ; undefined
    3d50:	3133203d 	adds	w29, w1, #0xcc8
    3d54:	636f4c00 	.inst	0x636f4c00 ; undefined
    3d58:	3d21206b 	str	b11, [x3, #2120]
    3d5c:	76282820 	.inst	0x76282820 ; undefined
    3d60:	2064696f 	.inst	0x2064696f ; undefined
    3d64:	3020292a 	adr	x10, 0x44289
    3d68:	72500029 	.inst	0x72500029 ; undefined
    3d6c:	69726f69 	ldpsw	x9, x27, [x27, #-112]
    3d70:	3c207974 	str	b20, [x11, x0, lsl #0]
    3d74:	3133203d 	adds	w29, w1, #0xcc8
    3d78:	636f4c00 	.inst	0x636f4c00 ; undefined
    3d7c:	4c3e2d6b 	.inst	0x4c3e2d6b ; undefined
    3d80:	206b636f 	.inst	0x206b636f ; undefined
    3d84:	45203d3d 	.inst	0x45203d3d ; undefined
    3d88:	6f4c6966 	umlsl2	v6.4s, v11.8h, v12.h[4]
    3d8c:	65526b63 	fcmeq	p3.h, p2/z, z27.h, z18.h
    3d90:	7361656c 	.inst	0x7361656c ; undefined
    3d94:	4c006465 	st1	{v5.8h-v7.8h}, [x3]
    3d98:	2d6b636f 	ldp	s15, s24, [x27, #-168]
    3d9c:	636f4c3e 	.inst	0x636f4c3e ; undefined
    3da0:	3d21206b 	str	b11, [x3, #2120]
    3da4:	69664520 	ldpsw	x0, x17, [x9, #-208]
    3da8:	6b636f4c 	.inst	0x6b636f4c ; undefined
    3dac:	6e696e55 	umin	v21.8h, v18.8h, v9.8h
    3db0:	61697469 	.inst	0x61697469 ; undefined
    3db4:	657a696c 	fnmls	z12.h, p2/m, z11.h, z26.h
    3db8:	6f4c0064 	mla	v4.8h, v3.8h, v12.h[0]
    3dbc:	3e2d6b63 	.inst	0x3e2d6b63 ; undefined
    3dc0:	6b636f4c 	.inst	0x6b636f4c ; undefined
    3dc4:	203d3d20 	.inst	0x203d3d20 ; undefined
    3dc8:	4c696645 	.inst	0x4c696645 ; undefined
    3dcc:	416b636f 	.inst	0x416b636f ; undefined
    3dd0:	69757163 	ldpsw	x3, x28, [x11, #-88]
    3dd4:	00646572 	.inst	0x00646572 ; undefined
    3dd8:	656d614e 	fnmls	z14.h, p0/m, z10.h, z13.h
    3ddc:	203d2120 	.inst	0x203d2120 ; undefined
    3de0:	6f762828 	umlal2	v8.4s, v1.8h, v6.h[7]
    3de4:	2a206469 	orn	w9, w3, w0, lsl #25
    3de8:	29302029 	stp	w9, w8, [x1, #-128]
    3dec:	20262620 	.inst	0x20262620 ; undefined
    3df0:	64697547 	.inst	0x64697547 ; undefined
    3df4:	203d2120 	.inst	0x203d2120 ; undefined
    3df8:	6f762828 	umlal2	v8.4s, v1.8h, v6.h[7]
    3dfc:	2a206469 	orn	w9, w3, w0, lsl #25
    3e00:	29302029 	stp	w9, w8, [x1, #-128]
    3e04:	20262620 	.inst	0x20262620 ; undefined
    3e08:	756c6156 	.inst	0x756c6156 ; undefined
    3e0c:	3d212065 	str	b5, [x3, #2120]
    3e10:	76282820 	.inst	0x76282820 ; undefined
    3e14:	2064696f 	.inst	0x2064696f ; undefined
    3e18:	3020292a 	adr	x10, 0x4433d
    3e1c:	562a0029 	.inst	0x562a0029 ; undefined
    3e20:	65756c61 	fnmls	z1.h, p3/m, z3.h, z21.h
    3e24:	203d2120 	.inst	0x203d2120 ; undefined
    3e28:	6f762828 	umlal2	v8.4s, v1.8h, v6.h[7]
    3e2c:	2a206469 	orn	w9, w3, w0, lsl #25
    3e30:	29302029 	stp	w9, w8, [x1, #-128]
    3e34:	70755300 	adr	x0, 0xee897
    3e38:	74726f70 	.inst	0x74726f70 ; undefined
    3e3c:	614c6465 	.inst	0x614c6465 ; undefined
    3e40:	6175676e 	.inst	0x6175676e ; undefined
    3e44:	20736567 	.inst	0x20736567 ; undefined
    3e48:	28203d21 	stnp	w1, w15, [x9, #-256]
    3e4c:	696f7628 	ldpsw	x8, x29, [x17, #-136]
    3e50:	292a2064 	stp	w4, w8, [x3, #-176]
    3e54:	00293020 	.inst	0x00293020 ; NYI
    3e58:	4c626f48 	.inst	0x4c626f48 ; undefined
    3e5c:	632e6269 	.inst	0x632e6269 ; undefined
    3e60:	6f486d00 	.inst	0x6f486d00 ; undefined
    3e64:	73694c62 	.inst	0x73694c62 ; undefined
    3e68:	3d212074 	str	b20, [x3, #2120]
    3e6c:	76282820 	.inst	0x76282820 ; undefined
    3e70:	2064696f 	.inst	0x2064696f ; undefined
    3e74:	3020292a 	adr	x10, 0x44399
    3e78:	6f480029 	mla	v9.8h, v1.8h, v8.h[0]
    3e7c:	61745362 	.inst	0x61745362 ; undefined
    3e80:	21207472 	.inst	0x21207472 ; undefined
    3e84:	2828203d 	stnp	w29, w8, [x1, #-192]
    3e88:	64696f76 	.inst	0x64696f76 ; undefined
    3e8c:	20292a20 	.inst	0x20292a20 ; undefined
    3e90:	28002930 	stnp	w16, w10, [x9]
    3e94:	4f4f4228 	.inst	0x4f4f4228 ; undefined
    3e98:	4e41454c 	.inst	0x4e41454c ; undefined
    3e9c:	3d302829 	str	b9, [x1, #3082]
    3ea0:	2929313d 	stp	w29, w12, [x9, #-184]
    3ea4:	00450000 	.inst	0x00450000 ; undefined
    3ea8:	00490046 	.inst	0x00490046 ; undefined
    3eac:	00650044 	.inst	0x00650044 ; undefined
    3eb0:	00750062 	.inst	0x00750062 ; undefined
    3eb4:	00000067 	udf	#103
    3eb8:	416d7241 	.inst	0x416d7241 ; undefined
    3ebc:	54686372 	bc.cs	0xd4b28  // bc.hs, bc.nlast
    3ec0:	72656d69 	.inst	0x72656d69 ; undefined
    3ec4:	2e62694c 	.inst	0x2e62694c ; undefined
    3ec8:	5f280063 	.inst	0x5f280063 ; undefined
    3ecc:	64635067 	.inst	0x64635067 ; undefined
    3ed0:	7869465f 	.inst	0x7869465f ; undefined
    3ed4:	74416465 	.inst	0x74416465 ; undefined
    3ed8:	6c697542 	ldnp	d2, d29, [x10, #-368]
    3edc:	63505f64 	.inst	0x63505f64 ; undefined
    3ee0:	6d724164 	ldp	d4, d16, [x11, #-224]
    3ee4:	68637241 	.inst	0x68637241 ; undefined
    3ee8:	656d6954 	fnmls	z20.h, p2/m, z10.h, z13.h
    3eec:	65724672 	fnmla	z18.h, p1/m, z19.h, z18.h
    3ef0:	486e4971 	.inst	0x486e4971 ; undefined
    3ef4:	30312f7a 	adr	x26, 0x664e1
    3ef8:	30303030 	adr	x16, 0x644fd
    3efc:	00295530 	.inst	0x00295530 ; NYI
    3f00:	476d7241 	.inst	0x476d7241 ; undefined
    3f04:	72656e65 	.inst	0x72656e65 ; undefined
    3f08:	69546369 	ldpsw	x9, x24, [x27, #160]
    3f0c:	4772656d 	.inst	0x4772656d ; undefined
    3f10:	69547465 	ldpsw	x5, x29, [x3, #160]
    3f14:	4672656d 	.inst	0x4672656d ; undefined
    3f18:	20716572 	.inst	0x20716572 ; undefined
    3f1c:	21202928 	.inst	0x21202928 ; undefined
    3f20:	0030203d 	.inst	0x0030203d ; NYI
    3f24:	204d5241 	.inst	0x204d5241 ; undefined
    3f28:	68637241 	.inst	0x68637241 ; undefined
    3f2c:	63657469 	.inst	0x63657469 ; undefined
    3f30:	61727574 	.inst	0x61727574 ; undefined
    3f34:	6954206c 	ldpsw	x12, x8, [x3, #160]
    3f38:	2072656d 	.inst	0x2072656d ; undefined
    3f3c:	6e207369 	uabdl2	v9.8h, v27.16b, v0.16b
    3f40:	6120746f 	.inst	0x6120746f ; undefined
    3f44:	6c696176 	ldnp	d22, d24, [x11, #-368]
    3f48:	656c6261 	fnmls	z1.h, p0/m, z19.h, z12.h
    3f4c:	206e6920 	.inst	0x206e6920 ; undefined
    3f50:	20656874 	.inst	0x20656874 ; undefined
    3f54:	2c555043 	ldnp	s3, s20, [x2, #168]
    3f58:	6e656820 	.inst	0x6e656820 ; undefined
    3f5c:	74206563 	.inst	0x74206563 ; undefined
    3f60:	20736968 	.inst	0x20736968 ; undefined
    3f64:	7262696c 	.inst	0x7262696c ; undefined
    3f68:	20797261 	.inst	0x20797261 ; undefined
    3f6c:	6e6e6163 	rsubhn2	v3.8h, v11.4s, v14.4s
    3f70:	6220746f 	.inst	0x6220746f ; undefined
    3f74:	73752065 	.inst	0x73752065 ; undefined
    3f78:	0a2e6465 	bic	w5, w3, w14, lsl #25
	...
    3f88:	00000001 	udf	#1
    3f8c:	77665501 	.inst	0x77665501 ; undefined
    3f90:	00000002 	udf	#2
    3f94:	77665502 	.inst	0x77665502 ; undefined
    3f98:	00000003 	udf	#3
    3f9c:	77665500 	.inst	0x77665500 ; undefined
    3fa0:	00000004 	udf	#4
    3fa4:	77665503 	.inst	0x77665503 ; undefined
    3fa8:	00000005 	udf	#5
    3fac:	77665508 	.inst	0x77665508 ; undefined
    3fb0:	00000006 	udf	#6
    3fb4:	77665509 	.inst	0x77665509 ; undefined
    3fb8:	00000007 	udf	#7
    3fbc:	7766550a 	.inst	0x7766550a ; undefined
    3fc0:	00000040 	udf	#64
    3fc4:	c0dedead 	.inst	0xc0dedead ; undefined
    3fc8:	00000041 	udf	#65
    3fcc:	abadbabe 	.inst	0xabadbabe ; undefined
    3fd0:	00000042 	udf	#66
    3fd4:	abadf00d 	.inst	0xabadf00d ; undefined
    3fd8:	00000043 	udf	#67
    3fdc:	abadf000 	.inst	0xabadf000 ; undefined
    3fe0:	00000048 	udf	#72
    3fe4:	000052d0 	udf	#21200
    3fe8:	00000049 	udf	#73
    3fec:	000052d1 	udf	#21201
    3ff0:	0000004a 	udf	#74
    3ff4:	000052d2 	udf	#21202
    3ff8:	0000004b 	udf	#75
    3ffc:	000052d3 	udf	#21203
    4000:	0000004c 	udf	#76
    4004:	000052d4 	udf	#21204
    4008:	0000004d 	udf	#77
    400c:	000052d5 	udf	#21205
    4010:	0000004e 	udf	#78
    4014:	000052dc 	udf	#21212
    4018:	0000004f 	udf	#79
    401c:	000052de 	udf	#21214
    4020:	00000050 	udf	#80
    4024:	6f656d46 	.inst	0x6f656d46 ; undefined
    4028:	00000051 	udf	#81
    402c:	6f656d50 	.inst	0x6f656d50 ; undefined
    4030:	00000061 	udf	#97
    4034:	6f656d4e 	.inst	0x6f656d4e ; undefined
    4038:	00000060 	udf	#96
    403c:	6f656d53 	.inst	0x6f656d53 ; undefined
    4040:	00000062 	udf	#98
    4044:	4d25daba 	.inst	0x4d25daba ; undefined
    4048:	6c696146 	ldnp	d6, d24, [x10, #-368]
    404c:	74206465 	.inst	0x74206465 ; undefined
    4050:	6567206f 	fmls	z15.h, p0/m, z3.h, z7.h
    4054:	65522074 	.inst	0x65522074 ; undefined
    4058:	746f6f62 	.inst	0x746f6f62 ; undefined
    405c:	61657220 	.inst	0x61657220 ; undefined
    4060:	3a6e6f73 	.inst	0x3a6e6f73 ; undefined
    4064:	0a722520 	bic	w0, w9, w18, lsr #9
    4068:	72724500 	.inst	0x72724500 ; undefined
    406c:	6720726f 	.inst	0x6720726f ; undefined
    4070:	735f7465 	.inst	0x735f7465 ; undefined
    4074:	725f6964 	.inst	0x725f6964 ; undefined
    4078:	6f696765 	sqshlu	v5.2d, v27.2d, #41
    407c:	2e29286e 	.inst	0x2e29286e ; undefined
    4080:	72726520 	.inst	0x72726520 ; undefined
    4084:	616c665f 	.inst	0x616c665f ; undefined
    4088:	78253d67 	.inst	0x78253d67 ; undefined
    408c:	7245000a 	.inst	0x7245000a ; undefined
    4090:	20726f72 	.inst	0x20726f72 ; undefined
    4094:	61636f6c 	.inst	0x61636f6c ; undefined
    4098:	676e6974 	.inst	0x676e6974 ; undefined
    409c:	65687420 	fnmls	z0.h, p5/m, z1.h, z8.h
    40a0:	73657220 	.inst	0x73657220 ; undefined
    40a4:	72207465 	ands	w5, w3, #0x3fffffff
    40a8:	6f736165 	umlsl2	v5.4s, v11.8h, v3.h[3]
    40ac:	7270206e 	.inst	0x7270206e ; undefined
    40b0:	636f746f 	.inst	0x636f746f ; undefined
    40b4:	532e6c6f 	.inst	0x532e6c6f ; undefined
    40b8:	75746174 	.inst	0x75746174 ; undefined
    40bc:	72253d73 	ands	w19, w11, #0xf80007ff
    40c0:	5245000a 	.inst	0x5245000a ; undefined
    40c4:	3a524f52 	.inst	0x3a524f52 ; undefined
    40c8:	69614620 	ldpsw	x0, x17, [x17, #-248]
    40cc:	2064656c 	.inst	0x2064656c ; undefined
    40d0:	6c206f74 	stnp	d20, d27, [x27, #-512]
    40d4:	7461636f 	.inst	0x7461636f ; undefined
    40d8:	6d502065 	ldp	d5, d8, [x3, #256]
    40dc:	77506369 	.inst	0x77506369 ; undefined
    40e0:	506e4f72 	adr	x18, 0xe0ace
    40e4:	6f746f72 	.inst	0x6f746f72 ; undefined
    40e8:	3a6c6f63 	.inst	0x3a6c6f63 ; undefined
    40ec:	25783020 	whilewr	p0.h, x1, x24
    40f0:	0a783830 	bic	w16, w1, w24, lsr #14
    40f4:	72724500 	.inst	0x72724500 ; undefined
    40f8:	6c20726f 	stnp	d15, d28, [x19, #-512]
    40fc:	7461636f 	.inst	0x7461636f ; undefined
    4100:	20676e69 	.inst	0x20676e69 ; undefined
    4104:	20656874 	.inst	0x20656874 ; undefined
    4108:	6d656d53 	ldp	d19, d27, [x10, #-432]
    410c:	6f727020 	fcmla	v0.8h, v1.8h, v18.h[1], #270
    4110:	6f636f74 	.inst	0x6f636f74 ; undefined
    4114:	53202e6c 	.inst	0x53202e6c ; undefined
    4118:	75746174 	.inst	0x75746174 ; undefined
    411c:	72253d73 	ands	w19, w11, #0xf80007ff
    4120:	6e49000a 	.inst	0x6e49000a ; undefined
    4124:	696c6176 	ldpsw	x22, x24, [x11, #-160]
    4128:	65722064 	fmls	z4.h, p0/m, z3.h, z18.h
    412c:	6e727574 	uabd	v20.8h, v11.8h, v18.8h
    4130:	6c617620 	ldnp	d0, d29, [x17, #-496]
    4134:	3a736575 	.inst	0x3a736575 ; undefined
    4138:	657a6953 	fnmls	z19.h, p2/m, z10.h, z26.h
    413c:	29642528 	ldp	w8, w9, [x9, #-224]
    4140:	6148202c 	.inst	0x6148202c ; undefined
    4144:	656c646e 	fnmls	z14.h, p1/m, z3.h, z12.h
    4148:	29782528 	ldp	w8, w9, [x9, #-64]
    414c:	5245000a 	.inst	0x5245000a ; undefined
    4150:	3a524f52 	.inst	0x3a524f52 ; undefined
    4154:	69614620 	ldpsw	x0, x17, [x17, #-248]
    4158:	2064656c 	.inst	0x2064656c ; undefined
    415c:	73206f74 	.inst	0x73206f74 ; undefined
    4160:	20657661 	.inst	0x20657661 ; undefined
    4164:	65736572 	fnmls	z18.h, p1/m, z11.h, z19.h
    4168:	65722074 	fmls	z20.h, p0/m, z3.h, z18.h
    416c:	6e6f7361 	uabdl2	v1.4s, v27.8h, v15.8h
    4170:	7830203a 	ldeorh	w16, w26, [x1]
    4174:	78383025 	ldseth	w24, w5, [x1]
    4178:	6552000a 	fadd	z10.h, z0.h, z18.h
    417c:	74726f70 	.inst	0x74726f70 ; undefined
    4180:	74617453 	.inst	0x74617453 ; undefined
    4184:	6f437375 	fcmla	v21.8h, v27.8h, v3.h[0], #270
    4188:	694c6564 	ldpsw	x4, x25, [x11, #96]
    418c:	00632e62 	.inst	0x00632e62 ; undefined
    4190:	74736f50 	.inst	0x74736f50 ; undefined
    4194:	65646f43 	fnmls	z3.h, p3/m, z26.h, z4.h
    4198:	203d2120 	.inst	0x203d2120 ; undefined
    419c:	6f762828 	umlal2	v8.4s, v1.8h, v6.h[7]
    41a0:	2a206469 	orn	w9, w3, w0, lsl #25
    41a4:	29302029 	stp	w9, w8, [x1, #-128]
    41a8:	6c694600 	ldnp	d0, d17, [x16, #-368]
    41ac:	6d616e65 	ldp	d5, d27, [x19, #-496]
    41b0:	3d212065 	str	b5, [x3, #2120]
    41b4:	76282820 	.inst	0x76282820 ; undefined
    41b8:	2064696f 	.inst	0x2064696f ; undefined
    41bc:	3020292a 	adr	x10, 0x446e1
    41c0:	65440029 	fadd	z9.h, z1.h, z4.h
    41c4:	69726373 	ldpsw	x19, x24, [x27, #-112]
    41c8:	6f697470 	uqshl	v16.2d, v3.2d, #41
    41cc:	3d21206e 	str	b14, [x3, #2120]
    41d0:	76282820 	.inst	0x76282820 ; undefined
    41d4:	2064696f 	.inst	0x2064696f ; undefined
    41d8:	3020292a 	adr	x10, 0x446fd
    41dc:	694c0029 	ldpsw	x9, x0, [x1, #96]
    41e0:	754e656e 	.inst	0x754e656e ; undefined
    41e4:	7265626d 	.inst	0x7265626d ; undefined
    41e8:	203d2120 	.inst	0x203d2120 ; undefined
    41ec:	6f762828 	umlal2	v8.4s, v1.8h, v6.h[7]
    41f0:	2a206469 	orn	w9, w3, w0, lsl #25
    41f4:	29302029 	stp	w9, w8, [x1, #-128]
    41f8:	72724500 	.inst	0x72724500 ; undefined
    41fc:	654c726f 	fcmeq	p15.h, p4/z, z19.h, z12.h
    4200:	206c6576 	.inst	0x206c6576 ; undefined
    4204:	28203d21 	stnp	w1, w15, [x9, #-256]
    4208:	696f7628 	ldpsw	x8, x29, [x17, #-136]
    420c:	292a2064 	stp	w4, w8, [x3, #-176]
    4210:	00293020 	.inst	0x00293020 ; NYI
    4214:	6b72614d 	.inst	0x6b72614d ; undefined
    4218:	21207265 	.inst	0x21207265 ; undefined
    421c:	2828203d 	stnp	w29, w8, [x1, #-192]
    4220:	64696f76 	.inst	0x64696f76 ; undefined
    4224:	20292a20 	.inst	0x20292a20 ; undefined
    4228:	44002930 	cmla	z16.b, z9.b, z0.b, #180
    422c:	63697665 	.inst	0x63697665 ; undefined
    4230:	74615065 	.inst	0x74615065 ; undefined
    4234:	3d212068 	str	b8, [x3, #2120]
    4238:	76282820 	.inst	0x76282820 ; undefined
    423c:	2064696f 	.inst	0x2064696f ; undefined
    4240:	3020292a 	adr	x10, 0x44765
    4244:	78450029 	ldurh	w9, [x1, #80]
    4248:	646e6574 	.inst	0x646e6574 ; undefined
    424c:	61446465 	.inst	0x61446465 ; undefined
    4250:	21206174 	.inst	0x21206174 ; undefined
    4254:	2828203d 	stnp	w29, w8, [x1, #-192]
    4258:	64696f76 	.inst	0x64696f76 ; undefined
    425c:	20292a20 	.inst	0x20292a20 ; undefined
    4260:	45002930 	.inst	0x45002930 ; undefined
    4264:	6e657478 	uabd	v24.8h, v3.8h, v5.8h
    4268:	44646564 	.inst	0x44646564 ; undefined
    426c:	53617461 	.inst	0x53617461 ; undefined
    4270:	20657a69 	.inst	0x20657a69 ; undefined
    4274:	30203d21 	adr	x1, 0x44a19
    4278:	28282100 	stnp	w0, w8, [x8, #-192]
    427c:	65747845 	fnmls	z5.h, p6/m, z2.h, z20.h
    4280:	6465646e 	.inst	0x6465646e ; undefined
    4284:	61746144 	.inst	0x61746144 ; undefined
    4288:	203d3d20 	.inst	0x203d3d20 ; undefined
    428c:	6f762828 	umlal2	v8.4s, v1.8h, v6.h[7]
    4290:	2a206469 	orn	w9, w3, w0, lsl #25
    4294:	29302029 	stp	w9, w8, [x1, #-128]
    4298:	26262029 	.inst	0x26262029 ; undefined
    429c:	78452820 	ldtrh	w0, [x1, #82]
    42a0:	646e6574 	.inst	0x646e6574 ; undefined
    42a4:	61446465 	.inst	0x61446465 ; undefined
    42a8:	69536174 	ldpsw	x20, x24, [x11, #152]
    42ac:	2120657a 	.inst	0x2120657a ; undefined
    42b0:	2930203d 	stp	w29, w8, [x1, #-128]
    42b4:	28210029 	stnp	w9, w0, [x1, #-248]
    42b8:	74784528 	.inst	0x74784528 ; undefined
    42bc:	65646e65 	fnmls	z5.h, p3/m, z19.h, z4.h
    42c0:	74614464 	.inst	0x74614464 ; undefined
    42c4:	3d212061 	str	b1, [x3, #2120]
    42c8:	76282820 	.inst	0x76282820 ; undefined
    42cc:	2064696f 	.inst	0x2064696f ; undefined
    42d0:	3020292a 	adr	x10, 0x447f5
    42d4:	26202929 	.inst	0x26202929 ; undefined
    42d8:	45282026 	sqshrnb	z6.b, z1.h, #8
    42dc:	6e657478 	uabd	v24.8h, v3.8h, v5.8h
    42e0:	44646564 	.inst	0x44646564 ; undefined
    42e4:	53617461 	.inst	0x53617461 ; undefined
    42e8:	20657a69 	.inst	0x20657a69 ; undefined
    42ec:	30203d3d 	adr	x29, 0x44a91
    42f0:	53002929 	ubfx	w9, w9, #0, #11
    42f4:	75746174 	.inst	0x75746174 ; undefined
    42f8:	6f632073 	umlal2	v19.4s, v3.8h, v3.h[2]
    42fc:	65206564 	.inst	0x65206564 ; undefined
    4300:	6e657478 	uabd	v24.8h, v3.8h, v5.8h
    4304:	20646564 	.inst	0x20646564 ; undefined
    4308:	61746164 	.inst	0x61746164 ; undefined
    430c:	20736920 	.inst	0x20736920 ; undefined
    4310:	206f6f74 	.inst	0x206f6f74 ; undefined
    4314:	6772616c 	.inst	0x6772616c ; undefined
    4318:	6f742065 	umlal2	v5.4s, v3.8h, v4.h[3]
    431c:	20656220 	.inst	0x20656220 ; undefined
    4320:	6f706572 	sqshlu	v18.2d, v11.2d, #48
    4324:	64657472 	.inst	0x64657472 ; undefined
    4328:	4d000a21 	st1	{v1.b}[10], [x17]
    432c:	694c6d65 	ldpsw	x5, x27, [x11, #96]
    4330:	69754762 	ldpsw	x2, x17, [x27, #-88]
    4334:	00632e64 	.inst	0x00632e64 ; undefined
    4338:	74736544 	.inst	0x74736544 ; undefined
    433c:	74616e69 	.inst	0x74616e69 ; undefined
    4340:	476e6f69 	.inst	0x476e6f69 ; undefined
    4344:	20646975 	.inst	0x20646975 ; undefined
    4348:	28203d21 	stnp	w1, w15, [x9, #-256]
    434c:	696f7628 	ldpsw	x8, x29, [x17, #-136]
    4350:	292a2064 	stp	w4, w8, [x3, #-176]
    4354:	00293020 	.inst	0x00293020 ; NYI
    4358:	72756f53 	.inst	0x72756f53 ; undefined
    435c:	75476563 	.inst	0x75476563 ; undefined
    4360:	21206469 	.inst	0x21206469 ; undefined
    4364:	2828203d 	stnp	w29, w8, [x1, #-192]
    4368:	64696f76 	.inst	0x64696f76 ; undefined
    436c:	20292a20 	.inst	0x20292a20 ; undefined
    4370:	47002930 	.inst	0x47002930 ; undefined
    4374:	31646975 	adds	w21, w11, #0x91a, lsl #12
    4378:	203d2120 	.inst	0x203d2120 ; undefined
    437c:	6f762828 	umlal2	v8.4s, v1.8h, v6.h[7]
    4380:	2a206469 	orn	w9, w3, w0, lsl #25
    4384:	29302029 	stp	w9, w8, [x1, #-128]
    4388:	69754700 	ldpsw	x0, x17, [x24, #-88]
    438c:	21203264 	.inst	0x21203264 ; undefined
    4390:	2828203d 	stnp	w29, w8, [x1, #-192]
    4394:	64696f76 	.inst	0x64696f76 ; undefined
    4398:	20292a20 	.inst	0x20292a20 ; undefined
    439c:	28002930 	stnp	w16, w10, [x9]
    43a0:	4e495528 	.inst	0x4e495528 ; undefined
    43a4:	42294e54 	.inst	0x42294e54 ; undefined
    43a8:	65666675 	fnmls	z21.h, p1/m, z19.h, z6.h
    43ac:	20262072 	.inst	0x20262072 ; undefined
    43b0:	7a697328 	.inst	0x7a697328 ; undefined
    43b4:	20666f65 	.inst	0x20666f65 ; undefined
    43b8:	69754728 	ldpsw	x8, x17, [x25, #-88]
    43bc:	443e2d64 	.inst	0x443e2d64 ; undefined
    43c0:	31617461 	adds	w1, w3, #0x85d, lsl #12
    43c4:	202d2029 	.inst	0x202d2029 ; undefined
    43c8:	20292931 	.inst	0x20292931 ; undefined
    43cc:	30203d3d 	adr	x29, 0x44b71
    43d0:	6e654c00 	uqshl	v0.8h, v0.8h, v5.8h
    43d4:	20687467 	.inst	0x20687467 ; undefined
    43d8:	28203d3c 	stnp	w28, w15, [x9, #-256]
    43dc:	46467830 	.inst	0x46467830 ; undefined
    43e0:	46464646 	.inst	0x46464646 ; undefined
    43e4:	46464646 	.inst	0x46464646 ; undefined
    43e8:	46464646 	.inst	0x46464646 ; undefined
    43ec:	4c554646 	.inst	0x4c554646 ; undefined
    43f0:	202d204c 	.inst	0x202d204c ; undefined
    43f4:	4e495528 	.inst	0x4e495528 ; undefined
    43f8:	42294e54 	.inst	0x42294e54 ; undefined
    43fc:	65666675 	fnmls	z21.h, p1/m, z19.h, z6.h
    4400:	202b2072 	.inst	0x202b2072 ; undefined
    4404:	28002931 	stnp	w17, w10, [x9]
    4408:	676e654c 	.inst	0x676e654c ; undefined
    440c:	26206874 	.inst	0x26206874 ; undefined
    4410:	69732820 	ldpsw	x0, x10, [x1, #-104]
    4414:	666f657a 	.inst	0x666f657a ; undefined
    4418:	472a2820 	.inst	0x472a2820 ; undefined
    441c:	50646975 	adr	x21, 0xcd14a
    4420:	20297274 	.inst	0x20297274 ; undefined
    4424:	2931202d 	stp	w13, w8, [x1, #-120]
    4428:	3d3d2029 	str	b9, [x1, #3912]
    442c:	43003020 	.inst	0x43003020 ; undefined
    4430:	4d79706f 	.inst	0x4d79706f ; undefined
    4434:	72576d65 	.inst	0x72576d65 ; undefined
    4438:	65707061 	fnmls	z1.h, p4/m, z3.h, z16.h
    443c:	00632e72 	.inst	0x00632e72 ; undefined
    4440:	6e654c28 	uqshl	v8.8h, v1.8h, v5.8h
    4444:	20687467 	.inst	0x20687467 ; undefined
    4448:	2931202d 	stp	w13, w8, [x1, #-120]
    444c:	203d3c20 	.inst	0x203d3c20 ; undefined
    4450:	46783028 	.inst	0x46783028 ; undefined
    4454:	46464646 	.inst	0x46464646 ; undefined
    4458:	46464646 	.inst	0x46464646 ; undefined
    445c:	46464646 	.inst	0x46464646 ; undefined
    4460:	55464646 	.inst	0x55464646 ; undefined
    4464:	2d204c4c 	stp	s12, s19, [x2, #-256]
    4468:	49552820 	.inst	0x49552820 ; undefined
    446c:	294e544e 	ldp	w14, w21, [x2, #112]
    4470:	74736544 	.inst	0x74736544 ; undefined
    4474:	74616e69 	.inst	0x74616e69 ; undefined
    4478:	426e6f69 	.inst	0x426e6f69 ; undefined
    447c:	65666675 	fnmls	z21.h, p1/m, z19.h, z6.h
    4480:	28002972 	stnp	w18, w10, [x11]
    4484:	676e654c 	.inst	0x676e654c ; undefined
    4488:	2d206874 	stp	s20, s26, [x3, #-256]
    448c:	20293120 	.inst	0x20293120 ; undefined
    4490:	28203d3c 	stnp	w28, w15, [x9, #-256]
    4494:	46467830 	.inst	0x46467830 ; undefined
    4498:	46464646 	.inst	0x46464646 ; undefined
    449c:	46464646 	.inst	0x46464646 ; undefined
    44a0:	46464646 	.inst	0x46464646 ; undefined
    44a4:	4c554646 	.inst	0x4c554646 ; undefined
    44a8:	202d204c 	.inst	0x202d204c ; undefined
    44ac:	4e495528 	.inst	0x4e495528 ; undefined
    44b0:	53294e54 	.inst	0x53294e54 ; undefined
    44b4:	6372756f 	.inst	0x6372756f ; undefined
    44b8:	66754265 	.inst	0x66754265 ; undefined
    44bc:	29726566 	ldp	w6, w25, [x11, #-112]
	...
    44c8:	000030e0 	udf	#12512
    44cc:	00000000 	udf	#0
    44d0:	000030e8 	udf	#12520
    44d4:	00000000 	udf	#0
    44d8:	000030f0 	udf	#12528
    44dc:	00000000 	udf	#0
    44e0:	000030f8 	udf	#12536
    44e4:	00000000 	udf	#0
    44e8:	00003100 	udf	#12544
    44ec:	00000000 	udf	#0
    44f0:	00003108 	udf	#12552
    44f4:	00000000 	udf	#0
    44f8:	00003110 	udf	#12560
    44fc:	00000000 	udf	#0
    4500:	00003118 	udf	#12568
    4504:	00000000 	udf	#0
    4508:	00003120 	udf	#12576
    450c:	00000000 	udf	#0
    4510:	00003128 	udf	#12584
    4514:	00000000 	udf	#0
    4518:	00003130 	udf	#12592
    451c:	00000000 	udf	#0
    4520:	00003080 	udf	#12416
    4524:	00000000 	udf	#0
    4528:	00003080 	udf	#12416
    452c:	00000000 	udf	#0
    4530:	00003080 	udf	#12416
    4534:	00000000 	udf	#0
    4538:	00003080 	udf	#12416
    453c:	00000000 	udf	#0
    4540:	20656854 	.inst	0x20656854 ; undefined
    4544:	69676572 	ldpsw	x18, x25, [x11, #-200]
    4548:	72657473 	.inst	0x72657473 ; undefined
    454c:	20736920 	.inst	0x20736920 ; undefined
    4550:	616c6572 	.inst	0x616c6572 ; undefined
    4554:	20646574 	.inst	0x20646574 ; undefined
    4558:	48206f74 	.inst	0x48206f74 ; undefined
    455c:	72657079 	.inst	0x72657079 ; undefined
    4560:	6f736976 	umlsl2	v22.4s, v11.8h, v3.h[7]
    4564:	6f4d2072 	umlal2	v18.4s, v3.8h, v13.h[0]
    4568:	202e6564 	.inst	0x202e6564 ; undefined
    456c:	276e6143 	.inst	0x276e6143 ; undefined
    4570:	65702074 	fmls	z20.h, p0/m, z3.h, z16.h
    4574:	726f6672 	.inst	0x726f6672 ; undefined
    4578:	6572206d 	fmls	z13.h, p0/m, z3.h, z18.h
    457c:	73657571 	.inst	0x73657571 ; undefined
    4580:	20646574 	.inst	0x20646574 ; undefined
    4584:	7265706f 	.inst	0x7265706f ; undefined
    4588:	6f697461 	uqshl	v1.2d, v3.2d, #41
    458c:	00200a6e 	.inst	0x00200a6e ; NYI
    4590:	6e6b6e55 	umin	v21.8h, v18.8h, v11.8h
    4594:	206e776f 	.inst	0x206e776f ; undefined
    4598:	204d5241 	.inst	0x204d5241 ; undefined
    459c:	656e6547 	fnmls	z7.h, p1/m, z10.h, z14.h
    45a0:	20636972 	.inst	0x20636972 ; undefined
    45a4:	656d6954 	fnmls	z20.h, p2/m, z10.h, z13.h
    45a8:	65722072 	fmls	z18.h, p0/m, z3.h, z18.h
    45ac:	74736967 	.inst	0x74736967 ; undefined
    45b0:	25207265 	.inst	0x25207265 ; undefined
    45b4:	0a202e78 	bic	w24, w19, w0, lsl #11
    45b8:	74410020 	.inst	0x74410020 ; undefined
    45bc:	706d6574 	adr	x20, 0xdf26b
    45c0:	6f742074 	umlal2	v20.4s, v3.8h, v4.h[3]
    45c4:	61657220 	.inst	0x61657220 ; undefined
    45c8:	52412064 	.inst	0x52412064 ; undefined
    45cc:	6547204d 	fminv	h13, p0, z2.h
    45d0:	6972656e 	ldpsw	x14, x25, [x11, #-112]
    45d4:	69542063 	ldpsw	x3, x8, [x3, #160]
    45d8:	2072656d 	.inst	0x2072656d ; undefined
    45dc:	69676572 	ldpsw	x18, x25, [x11, #-200]
    45e0:	72657473 	.inst	0x72657473 ; undefined
    45e4:	42202e73 	.inst	0x42202e73 ; undefined
    45e8:	41207475 	.inst	0x41207475 ; undefined
    45ec:	47204d52 	.inst	0x47204d52 ; undefined
    45f0:	72656e65 	.inst	0x72656e65 ; undefined
    45f4:	54206369 	b.ls	0x45260  // b.plast
    45f8:	72656d69 	.inst	0x72656d69 ; undefined
    45fc:	74786520 	.inst	0x74786520 ; undefined
    4600:	69736e65 	ldpsw	x5, x27, [x19, #-104]
    4604:	69206e6f 	stgp	x15, x27, [x19, #-1024]
    4608:	6f6e2073 	umlal2	v19.4s, v3.8h, v14.h[2]
    460c:	6d692074 	ldp	d20, d8, [x3, #-368]
    4610:	6d656c70 	ldp	d16, d27, [x3, #-432]
    4614:	65746e65 	fnmls	z5.h, p3/m, z19.h, z20.h
    4618:	200a2064 	.inst	0x200a2064 ; undefined
    461c:	72414100 	.inst	0x72414100 ; undefined
    4620:	34366863 	cbz	w3, 0x7132c
    4624:	68637241 	.inst	0x68637241 ; undefined
    4628:	656d6954 	fnmls	z20.h, p2/m, z10.h, z13.h
    462c:	00632e72 	.inst	0x00632e72 ; undefined
    4630:	276e6143 	.inst	0x276e6143 ; undefined
    4634:	72772074 	.inst	0x72772074 ; undefined
    4638:	20657469 	.inst	0x20657469 ; undefined
    463c:	52206f74 	eor	w20, w27, #0xfffffff
    4640:	20646165 	.inst	0x20646165 ; undefined
    4644:	796c6e4f 	ldrh	w15, [x18, #5686]
    4648:	67655220 	.inst	0x67655220 ; undefined
    464c:	65747369 	fnmls	z9.h, p4/m, z27.h, z20.h
    4650:	43203a72 	.inst	0x43203a72 ; undefined
    4654:	4350544e 	.inst	0x4350544e ; undefined
    4658:	000a2054 	.inst	0x000a2054 ; undefined
    465c:	276e6143 	.inst	0x276e6143 ; undefined
    4660:	72772074 	.inst	0x72772074 ; undefined
    4664:	20657469 	.inst	0x20657469 ; undefined
    4668:	52206f74 	eor	w20, w27, #0xfffffff
    466c:	20646165 	.inst	0x20646165 ; undefined
    4670:	796c6e4f 	ldrh	w15, [x18, #5686]
    4674:	67655220 	.inst	0x67655220 ; undefined
    4678:	65747369 	fnmls	z9.h, p4/m, z27.h, z20.h
    467c:	43203a72 	.inst	0x43203a72 ; undefined
    4680:	4356544e 	.inst	0x4356544e ; undefined
    4684:	000a2054 	.inst	0x000a2054 ; undefined
    4688:	65747441 	fnmls	z1.h, p5/m, z2.h, z20.h
    468c:	2074706d 	.inst	0x2074706d ; undefined
    4690:	77206f74 	.inst	0x77206f74 ; undefined
    4694:	65746972 	fnmls	z18.h, p2/m, z11.h, z20.h
    4698:	206f7420 	.inst	0x206f7420 ; undefined
    469c:	204d5241 	.inst	0x204d5241 ; undefined
    46a0:	656e6547 	fnmls	z7.h, p1/m, z10.h, z14.h
    46a4:	20636972 	.inst	0x20636972 ; undefined
    46a8:	656d6954 	fnmls	z20.h, p2/m, z10.h, z13.h
    46ac:	65722072 	fmls	z18.h, p0/m, z3.h, z18.h
    46b0:	74736967 	.inst	0x74736967 ; undefined
    46b4:	2e737265 	uabdl	v5.4s, v19.4h, v19.4h
    46b8:	74754220 	.inst	0x74754220 ; undefined
    46bc:	4d524120 	.inst	0x4d524120 ; undefined
    46c0:	6e654720 	ushl	v0.8h, v25.8h, v5.8h
    46c4:	63697265 	.inst	0x63697265 ; undefined
    46c8:	6d695420 	ldp	d0, d21, [x1, #-368]
    46cc:	65207265 	.inst	0x65207265 ; undefined
    46d0:	6e657478 	uabd	v24.8h, v3.8h, v5.8h
    46d4:	6e6f6973 	.inst	0x6e6f6973 ; undefined
    46d8:	20736920 	.inst	0x20736920 ; undefined
    46dc:	20746f6e 	.inst	0x20746f6e ; undefined
    46e0:	6c706d69 	ldnp	d9, d27, [x11, #-256]
    46e4:	6e656d65 	umin	v5.8h, v11.8h, v5.8h
    46e8:	20646574 	.inst	0x20646574 ; undefined
    46ec:	0000200a 	udf	#8202
	...
