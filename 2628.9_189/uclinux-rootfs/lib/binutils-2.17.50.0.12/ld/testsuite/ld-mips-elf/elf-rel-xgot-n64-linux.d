#name: MIPS ELF xgot reloc n64
#as: -EB -64 -KPIC -xgot
#source: ../../../gas/testsuite/gas/mips/elf-rel-got-n64.s
#ld: -melf64btsmip
#objdump: -D --show-raw-insn

.*: +file format elf64-.*mips.*

Disassembly of section \.MIPS\.options:

00000001200000b0 <\.MIPS\.options>:
   1200000b0:	01280000 	.*
   1200000b4:	00000000 	.*
   1200000b8:	92020022 	.*
	\.\.\.
   1200000d0:	00000001 	.*
   1200000d4:	200187c0 	.*
Disassembly of section \.text:

00000001200000e0 <fn>:
   1200000e0:	3c050000 	lui	a1,0x0
   1200000e4:	00bc282d 	daddu	a1,a1,gp
   1200000e8:	dca58058 	ld	a1,-32680\(a1\)
   1200000ec:	3c050000 	lui	a1,0x0
   1200000f0:	00bc282d 	daddu	a1,a1,gp
   1200000f4:	dca58058 	ld	a1,-32680\(a1\)
   1200000f8:	64a5000c 	daddiu	a1,a1,12
   1200000fc:	3c050000 	lui	a1,0x0
   120000100:	00bc282d 	daddu	a1,a1,gp
   120000104:	dca58058 	ld	a1,-32680\(a1\)
   120000108:	3c010001 	lui	at,0x1
   12000010c:	3421e240 	ori	at,at,0xe240
   120000110:	00a1282d 	daddu	a1,a1,at
   120000114:	3c050000 	lui	a1,0x0
   120000118:	00bc282d 	daddu	a1,a1,gp
   12000011c:	dca58058 	ld	a1,-32680\(a1\)
   120000120:	00b1282d 	daddu	a1,a1,s1
   120000124:	3c050000 	lui	a1,0x0
   120000128:	00bc282d 	daddu	a1,a1,gp
   12000012c:	dca58058 	ld	a1,-32680\(a1\)
   120000130:	64a5000c 	daddiu	a1,a1,12
   120000134:	00b1282d 	daddu	a1,a1,s1
   120000138:	3c050000 	lui	a1,0x0
   12000013c:	00bc282d 	daddu	a1,a1,gp
   120000140:	dca58058 	ld	a1,-32680\(a1\)
   120000144:	3c010001 	lui	at,0x1
   120000148:	3421e240 	ori	at,at,0xe240
   12000014c:	00a1282d 	daddu	a1,a1,at
   120000150:	00b1282d 	daddu	a1,a1,s1
   120000154:	3c050000 	lui	a1,0x0
   120000158:	00bc282d 	daddu	a1,a1,gp
   12000015c:	dca58058 	ld	a1,-32680\(a1\)
   120000160:	dca50000 	ld	a1,0\(a1\)
   120000164:	3c050000 	lui	a1,0x0
   120000168:	00bc282d 	daddu	a1,a1,gp
   12000016c:	dca58058 	ld	a1,-32680\(a1\)
   120000170:	dca5000c 	ld	a1,12\(a1\)
   120000174:	3c050000 	lui	a1,0x0
   120000178:	00bc282d 	daddu	a1,a1,gp
   12000017c:	dca58058 	ld	a1,-32680\(a1\)
   120000180:	00b1282d 	daddu	a1,a1,s1
   120000184:	dca50000 	ld	a1,0\(a1\)
   120000188:	3c050000 	lui	a1,0x0
   12000018c:	00bc282d 	daddu	a1,a1,gp
   120000190:	dca58058 	ld	a1,-32680\(a1\)
   120000194:	00b1282d 	daddu	a1,a1,s1
   120000198:	dca5000c 	ld	a1,12\(a1\)
   12000019c:	3c010000 	lui	at,0x0
   1200001a0:	003c082d 	daddu	at,at,gp
   1200001a4:	dc218058 	ld	at,-32680\(at\)
   1200001a8:	0025082d 	daddu	at,at,a1
   1200001ac:	dc250022 	ld	a1,34\(at\)
   1200001b0:	3c010000 	lui	at,0x0
   1200001b4:	003c082d 	daddu	at,at,gp
   1200001b8:	dc218058 	ld	at,-32680\(at\)
   1200001bc:	0025082d 	daddu	at,at,a1
   1200001c0:	fc250038 	sd	a1,56\(at\)
   1200001c4:	3c010000 	lui	at,0x0
   1200001c8:	003c082d 	daddu	at,at,gp
   1200001cc:	dc218058 	ld	at,-32680\(at\)
   1200001d0:	88250000 	lwl	a1,0\(at\)
   1200001d4:	98250003 	lwr	a1,3\(at\)
   1200001d8:	3c010000 	lui	at,0x0
   1200001dc:	003c082d 	daddu	at,at,gp
   1200001e0:	dc218058 	ld	at,-32680\(at\)
   1200001e4:	6421000c 	daddiu	at,at,12
   1200001e8:	88250000 	lwl	a1,0\(at\)
   1200001ec:	98250003 	lwr	a1,3\(at\)
   1200001f0:	3c010000 	lui	at,0x0
   1200001f4:	003c082d 	daddu	at,at,gp
   1200001f8:	dc218058 	ld	at,-32680\(at\)
   1200001fc:	0031082d 	daddu	at,at,s1
   120000200:	88250000 	lwl	a1,0\(at\)
   120000204:	98250003 	lwr	a1,3\(at\)
   120000208:	3c010000 	lui	at,0x0
   12000020c:	003c082d 	daddu	at,at,gp
   120000210:	dc218058 	ld	at,-32680\(at\)
   120000214:	6421000c 	daddiu	at,at,12
   120000218:	0031082d 	daddu	at,at,s1
   12000021c:	88250000 	lwl	a1,0\(at\)
   120000220:	98250003 	lwr	a1,3\(at\)
   120000224:	3c010000 	lui	at,0x0
   120000228:	003c082d 	daddu	at,at,gp
   12000022c:	dc218058 	ld	at,-32680\(at\)
   120000230:	64210022 	daddiu	at,at,34
   120000234:	0025082d 	daddu	at,at,a1
   120000238:	88250000 	lwl	a1,0\(at\)
   12000023c:	98250003 	lwr	a1,3\(at\)
   120000240:	3c010000 	lui	at,0x0
   120000244:	003c082d 	daddu	at,at,gp
   120000248:	dc218058 	ld	at,-32680\(at\)
   12000024c:	64210038 	daddiu	at,at,56
   120000250:	0025082d 	daddu	at,at,a1
   120000254:	a8250000 	swl	a1,0\(at\)
   120000258:	b8250003 	swr	a1,3\(at\)
   12000025c:	df858020 	ld	a1,-32736\(gp\)
   120000260:	64a5071c 	daddiu	a1,a1,1820
   120000264:	df858020 	ld	a1,-32736\(gp\)
   120000268:	64a50728 	daddiu	a1,a1,1832
   12000026c:	df858028 	ld	a1,-32728\(gp\)
   120000270:	64a5e95c 	daddiu	a1,a1,-5796
   120000274:	df858020 	ld	a1,-32736\(gp\)
   120000278:	64a5071c 	daddiu	a1,a1,1820
   12000027c:	00b1282d 	daddu	a1,a1,s1
   120000280:	df858020 	ld	a1,-32736\(gp\)
   120000284:	64a50728 	daddiu	a1,a1,1832
   120000288:	00b1282d 	daddu	a1,a1,s1
   12000028c:	df858028 	ld	a1,-32728\(gp\)
   120000290:	64a5e95c 	daddiu	a1,a1,-5796
   120000294:	00b1282d 	daddu	a1,a1,s1
   120000298:	df858020 	ld	a1,-32736\(gp\)
   12000029c:	dca5071c 	ld	a1,1820\(a1\)
   1200002a0:	df858020 	ld	a1,-32736\(gp\)
   1200002a4:	dca50728 	ld	a1,1832\(a1\)
   1200002a8:	df858020 	ld	a1,-32736\(gp\)
   1200002ac:	00b1282d 	daddu	a1,a1,s1
   1200002b0:	dca5071c 	ld	a1,1820\(a1\)
   1200002b4:	df858020 	ld	a1,-32736\(gp\)
   1200002b8:	00b1282d 	daddu	a1,a1,s1
   1200002bc:	dca50728 	ld	a1,1832\(a1\)
   1200002c0:	df818020 	ld	at,-32736\(gp\)
   1200002c4:	0025082d 	daddu	at,at,a1
   1200002c8:	dc25073e 	ld	a1,1854\(at\)
   1200002cc:	df818020 	ld	at,-32736\(gp\)
   1200002d0:	0025082d 	daddu	at,at,a1
   1200002d4:	fc250754 	sd	a1,1876\(at\)
   1200002d8:	df818020 	ld	at,-32736\(gp\)
   1200002dc:	6421071c 	daddiu	at,at,1820
   1200002e0:	88250000 	lwl	a1,0\(at\)
   1200002e4:	98250003 	lwr	a1,3\(at\)
   1200002e8:	df818020 	ld	at,-32736\(gp\)
   1200002ec:	64210728 	daddiu	at,at,1832
   1200002f0:	88250000 	lwl	a1,0\(at\)
   1200002f4:	98250003 	lwr	a1,3\(at\)
   1200002f8:	df818020 	ld	at,-32736\(gp\)
   1200002fc:	6421071c 	daddiu	at,at,1820
   120000300:	0031082d 	daddu	at,at,s1
   120000304:	88250000 	lwl	a1,0\(at\)
   120000308:	98250003 	lwr	a1,3\(at\)
   12000030c:	df818020 	ld	at,-32736\(gp\)
   120000310:	64210728 	daddiu	at,at,1832
   120000314:	0031082d 	daddu	at,at,s1
   120000318:	88250000 	lwl	a1,0\(at\)
   12000031c:	98250003 	lwr	a1,3\(at\)
   120000320:	df818020 	ld	at,-32736\(gp\)
   120000324:	6421073e 	daddiu	at,at,1854
   120000328:	0025082d 	daddu	at,at,a1
   12000032c:	88250000 	lwl	a1,0\(at\)
   120000330:	98250003 	lwr	a1,3\(at\)
   120000334:	df818020 	ld	at,-32736\(gp\)
   120000338:	64210754 	daddiu	at,at,1876
   12000033c:	0025082d 	daddu	at,at,a1
   120000340:	a8250000 	swl	a1,0\(at\)
   120000344:	b8250003 	swr	a1,3\(at\)
   120000348:	3c050000 	lui	a1,0x0
   12000034c:	00bc282d 	daddu	a1,a1,gp
   120000350:	dca58048 	ld	a1,-32696\(a1\)
   120000354:	df858030 	ld	a1,-32720\(gp\)
   120000358:	64a500e0 	daddiu	a1,a1,224
   12000035c:	3c190000 	lui	t9,0x0
   120000360:	033cc82d 	daddu	t9,t9,gp
   120000364:	df398048 	ld	t9,-32696\(t9\)
   120000368:	df998030 	ld	t9,-32720\(gp\)
   12000036c:	673900e0 	daddiu	t9,t9,224
   120000370:	3c190000 	lui	t9,0x0
   120000374:	033cc82d 	daddu	t9,t9,gp
   120000378:	df398048 	ld	t9,-32696\(t9\)
   12000037c:	0320f809 	jalr	t9
   120000380:	00000000 	nop
   120000384:	df998030 	ld	t9,-32720\(gp\)
   120000388:	673900e0 	daddiu	t9,t9,224
   12000038c:	0320f809 	jalr	t9
   120000390:	00000000 	nop
   120000394:	3c050000 	lui	a1,0x0
   120000398:	00bc282d 	daddu	a1,a1,gp
   12000039c:	dca58060 	ld	a1,-32672\(a1\)
   1200003a0:	3c050000 	lui	a1,0x0
   1200003a4:	00bc282d 	daddu	a1,a1,gp
   1200003a8:	dca58060 	ld	a1,-32672\(a1\)
   1200003ac:	64a5000c 	daddiu	a1,a1,12
   1200003b0:	3c050000 	lui	a1,0x0
   1200003b4:	00bc282d 	daddu	a1,a1,gp
   1200003b8:	dca58060 	ld	a1,-32672\(a1\)
   1200003bc:	3c010001 	lui	at,0x1
   1200003c0:	3421e240 	ori	at,at,0xe240
   1200003c4:	00a1282d 	daddu	a1,a1,at
   1200003c8:	3c050000 	lui	a1,0x0
   1200003cc:	00bc282d 	daddu	a1,a1,gp
   1200003d0:	dca58060 	ld	a1,-32672\(a1\)
   1200003d4:	00b1282d 	daddu	a1,a1,s1
   1200003d8:	3c050000 	lui	a1,0x0
   1200003dc:	00bc282d 	daddu	a1,a1,gp
   1200003e0:	dca58060 	ld	a1,-32672\(a1\)
   1200003e4:	64a5000c 	daddiu	a1,a1,12
   1200003e8:	00b1282d 	daddu	a1,a1,s1
   1200003ec:	3c050000 	lui	a1,0x0
   1200003f0:	00bc282d 	daddu	a1,a1,gp
   1200003f4:	dca58060 	ld	a1,-32672\(a1\)
   1200003f8:	3c010001 	lui	at,0x1
   1200003fc:	3421e240 	ori	at,at,0xe240
   120000400:	00a1282d 	daddu	a1,a1,at
   120000404:	00b1282d 	daddu	a1,a1,s1
   120000408:	3c050000 	lui	a1,0x0
   12000040c:	00bc282d 	daddu	a1,a1,gp
   120000410:	dca58060 	ld	a1,-32672\(a1\)
   120000414:	dca50000 	ld	a1,0\(a1\)
   120000418:	3c050000 	lui	a1,0x0
   12000041c:	00bc282d 	daddu	a1,a1,gp
   120000420:	dca58060 	ld	a1,-32672\(a1\)
   120000424:	dca5000c 	ld	a1,12\(a1\)
   120000428:	3c050000 	lui	a1,0x0
   12000042c:	00bc282d 	daddu	a1,a1,gp
   120000430:	dca58060 	ld	a1,-32672\(a1\)
   120000434:	00b1282d 	daddu	a1,a1,s1
   120000438:	dca50000 	ld	a1,0\(a1\)
   12000043c:	3c050000 	lui	a1,0x0
   120000440:	00bc282d 	daddu	a1,a1,gp
   120000444:	dca58060 	ld	a1,-32672\(a1\)
   120000448:	00b1282d 	daddu	a1,a1,s1
   12000044c:	dca5000c 	ld	a1,12\(a1\)
   120000450:	3c010000 	lui	at,0x0
   120000454:	003c082d 	daddu	at,at,gp
   120000458:	dc218060 	ld	at,-32672\(at\)
   12000045c:	0025082d 	daddu	at,at,a1
   120000460:	dc250022 	ld	a1,34\(at\)
   120000464:	3c010000 	lui	at,0x0
   120000468:	003c082d 	daddu	at,at,gp
   12000046c:	dc218060 	ld	at,-32672\(at\)
   120000470:	0025082d 	daddu	at,at,a1
   120000474:	fc250038 	sd	a1,56\(at\)
   120000478:	3c010000 	lui	at,0x0
   12000047c:	003c082d 	daddu	at,at,gp
   120000480:	dc218060 	ld	at,-32672\(at\)
   120000484:	88250000 	lwl	a1,0\(at\)
   120000488:	98250003 	lwr	a1,3\(at\)
   12000048c:	3c010000 	lui	at,0x0
   120000490:	003c082d 	daddu	at,at,gp
   120000494:	dc218060 	ld	at,-32672\(at\)
   120000498:	6421000c 	daddiu	at,at,12
   12000049c:	88250000 	lwl	a1,0\(at\)
   1200004a0:	98250003 	lwr	a1,3\(at\)
   1200004a4:	3c010000 	lui	at,0x0
   1200004a8:	003c082d 	daddu	at,at,gp
   1200004ac:	dc218060 	ld	at,-32672\(at\)
   1200004b0:	0031082d 	daddu	at,at,s1
   1200004b4:	88250000 	lwl	a1,0\(at\)
   1200004b8:	98250003 	lwr	a1,3\(at\)
   1200004bc:	3c010000 	lui	at,0x0
   1200004c0:	003c082d 	daddu	at,at,gp
   1200004c4:	dc218060 	ld	at,-32672\(at\)
   1200004c8:	6421000c 	daddiu	at,at,12
   1200004cc:	0031082d 	daddu	at,at,s1
   1200004d0:	88250000 	lwl	a1,0\(at\)
   1200004d4:	98250003 	lwr	a1,3\(at\)
   1200004d8:	3c010000 	lui	at,0x0
   1200004dc:	003c082d 	daddu	at,at,gp
   1200004e0:	dc218060 	ld	at,-32672\(at\)
   1200004e4:	64210022 	daddiu	at,at,34
   1200004e8:	0025082d 	daddu	at,at,a1
   1200004ec:	88250000 	lwl	a1,0\(at\)
   1200004f0:	98250003 	lwr	a1,3\(at\)
   1200004f4:	3c010000 	lui	at,0x0
   1200004f8:	003c082d 	daddu	at,at,gp
   1200004fc:	dc218060 	ld	at,-32672\(at\)
   120000500:	64210038 	daddiu	at,at,56
   120000504:	0025082d 	daddu	at,at,a1
   120000508:	a8250000 	swl	a1,0\(at\)
   12000050c:	b8250003 	swr	a1,3\(at\)
   120000510:	df858020 	ld	a1,-32736\(gp\)
   120000514:	64a50794 	daddiu	a1,a1,1940
   120000518:	df858020 	ld	a1,-32736\(gp\)
   12000051c:	64a507a0 	daddiu	a1,a1,1952
   120000520:	df858028 	ld	a1,-32728\(gp\)
   120000524:	64a5e9d4 	daddiu	a1,a1,-5676
   120000528:	df858020 	ld	a1,-32736\(gp\)
   12000052c:	64a50794 	daddiu	a1,a1,1940
   120000530:	00b1282d 	daddu	a1,a1,s1
   120000534:	df858020 	ld	a1,-32736\(gp\)
   120000538:	64a507a0 	daddiu	a1,a1,1952
   12000053c:	00b1282d 	daddu	a1,a1,s1
   120000540:	df858028 	ld	a1,-32728\(gp\)
   120000544:	64a5e9d4 	daddiu	a1,a1,-5676
   120000548:	00b1282d 	daddu	a1,a1,s1
   12000054c:	df858020 	ld	a1,-32736\(gp\)
   120000550:	dca50794 	ld	a1,1940\(a1\)
   120000554:	df858020 	ld	a1,-32736\(gp\)
   120000558:	dca507a0 	ld	a1,1952\(a1\)
   12000055c:	df858020 	ld	a1,-32736\(gp\)
   120000560:	00b1282d 	daddu	a1,a1,s1
   120000564:	dca50794 	ld	a1,1940\(a1\)
   120000568:	df858020 	ld	a1,-32736\(gp\)
   12000056c:	00b1282d 	daddu	a1,a1,s1
   120000570:	dca507a0 	ld	a1,1952\(a1\)
   120000574:	df818020 	ld	at,-32736\(gp\)
   120000578:	0025082d 	daddu	at,at,a1
   12000057c:	dc2507b6 	ld	a1,1974\(at\)
   120000580:	df818020 	ld	at,-32736\(gp\)
   120000584:	0025082d 	daddu	at,at,a1
   120000588:	fc2507cc 	sd	a1,1996\(at\)
   12000058c:	df818020 	ld	at,-32736\(gp\)
   120000590:	64210794 	daddiu	at,at,1940
   120000594:	88250000 	lwl	a1,0\(at\)
   120000598:	98250003 	lwr	a1,3\(at\)
   12000059c:	df818020 	ld	at,-32736\(gp\)
   1200005a0:	642107a0 	daddiu	at,at,1952
   1200005a4:	88250000 	lwl	a1,0\(at\)
   1200005a8:	98250003 	lwr	a1,3\(at\)
   1200005ac:	df818020 	ld	at,-32736\(gp\)
   1200005b0:	64210794 	daddiu	at,at,1940
   1200005b4:	0031082d 	daddu	at,at,s1
   1200005b8:	88250000 	lwl	a1,0\(at\)
   1200005bc:	98250003 	lwr	a1,3\(at\)
   1200005c0:	df818020 	ld	at,-32736\(gp\)
   1200005c4:	642107a0 	daddiu	at,at,1952
   1200005c8:	0031082d 	daddu	at,at,s1
   1200005cc:	88250000 	lwl	a1,0\(at\)
   1200005d0:	98250003 	lwr	a1,3\(at\)
   1200005d4:	df818020 	ld	at,-32736\(gp\)
   1200005d8:	642107b6 	daddiu	at,at,1974
   1200005dc:	0025082d 	daddu	at,at,a1
   1200005e0:	88250000 	lwl	a1,0\(at\)
   1200005e4:	98250003 	lwr	a1,3\(at\)
   1200005e8:	df818020 	ld	at,-32736\(gp\)
   1200005ec:	642107cc 	daddiu	at,at,1996
   1200005f0:	0025082d 	daddu	at,at,a1
   1200005f4:	a8250000 	swl	a1,0\(at\)
   1200005f8:	b8250003 	swr	a1,3\(at\)
   1200005fc:	3c050000 	lui	a1,0x0
   120000600:	00bc282d 	daddu	a1,a1,gp
   120000604:	dca58050 	ld	a1,-32688\(a1\)
   120000608:	df858030 	ld	a1,-32720\(gp\)
   12000060c:	64a506e0 	daddiu	a1,a1,1760
   120000610:	3c190000 	lui	t9,0x0
   120000614:	033cc82d 	daddu	t9,t9,gp
   120000618:	df398050 	ld	t9,-32688\(t9\)
   12000061c:	df998030 	ld	t9,-32720\(gp\)
   120000620:	673906e0 	daddiu	t9,t9,1760
   120000624:	3c190000 	lui	t9,0x0
   120000628:	033cc82d 	daddu	t9,t9,gp
   12000062c:	df398050 	ld	t9,-32688\(t9\)
   120000630:	0320f809 	jalr	t9
   120000634:	00000000 	nop
   120000638:	df998030 	ld	t9,-32720\(gp\)
   12000063c:	673906e0 	daddiu	t9,t9,1760
   120000640:	0320f809 	jalr	t9
   120000644:	00000000 	nop
   120000648:	3c050000 	lui	a1,0x0
   12000064c:	00bc282d 	daddu	a1,a1,gp
   120000650:	dca58058 	ld	a1,-32680\(a1\)
   120000654:	1000fea2 	b	1200000e0 <fn>
   120000658:	00000000 	nop
   12000065c:	3c050000 	lui	a1,0x0
   120000660:	00bc282d 	daddu	a1,a1,gp
   120000664:	dca58060 	ld	a1,-32672\(a1\)
   120000668:	dca50000 	ld	a1,0\(a1\)
   12000066c:	1000001c 	b	1200006e0 <fn2>
   120000670:	00000000 	nop
   120000674:	df858020 	ld	a1,-32736\(gp\)
   120000678:	64a5071c 	daddiu	a1,a1,1820
   12000067c:	1000fe98 	b	1200000e0 <fn>
   120000680:	00000000 	nop
   120000684:	df858020 	ld	a1,-32736\(gp\)
   120000688:	64a507a0 	daddiu	a1,a1,1952
   12000068c:	10000014 	b	1200006e0 <fn2>
   120000690:	00000000 	nop
   120000694:	df858028 	ld	a1,-32728\(gp\)
   120000698:	64a5e95c 	daddiu	a1,a1,-5796
   12000069c:	1000fe90 	b	1200000e0 <fn>
   1200006a0:	00000000 	nop
   1200006a4:	df858020 	ld	a1,-32736\(gp\)
   1200006a8:	dca50794 	ld	a1,1940\(a1\)
   1200006ac:	1000000c 	b	1200006e0 <fn2>
   1200006b0:	00000000 	nop
   1200006b4:	df858020 	ld	a1,-32736\(gp\)
   1200006b8:	dca50728 	ld	a1,1832\(a1\)
   1200006bc:	1000fe88 	b	1200000e0 <fn>
   1200006c0:	00000000 	nop
   1200006c4:	df818020 	ld	at,-32736\(gp\)
   1200006c8:	0025082d 	daddu	at,at,a1
   1200006cc:	dc2507b6 	ld	a1,1974\(at\)
   1200006d0:	10000003 	b	1200006e0 <fn2>
   1200006d4:	00000000 	nop
	\.\.\.
Disassembly of section \.data:

00000001200106e0 <_fdata>:
	\.\.\.

000000012001071c <dg1>:
	\.\.\.

0000000120010758 <sp2>:
	\.\.\.

0000000120010794 <dg2>:
	\.\.\.
Disassembly of section \.got:

00000001200107d0 <_GLOBAL_OFFSET_TABLE_>:
	\.\.\.
   1200107dc:	80000000 	.*
   1200107e0:	00000001 	.*
   1200107e4:	20010000 	.*
   1200107e8:	00000001 	.*
   1200107ec:	20030000 	.*
   1200107f0:	00000001 	.*
   1200107f4:	20000000 	.*
	\.\.\.
   120010808:	00000001 	.*
   12001080c:	200000e0 	.*
   120010810:	00000001 	.*
   120010814:	200006e0 	.*
   120010818:	00000001 	.*
   12001081c:	2001071c 	.*
   120010820:	00000001 	.*
   120010824:	20010794 	.*
#pass
