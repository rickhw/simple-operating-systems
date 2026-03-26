
跑起來後直接 crash 了，底下是 core dump & source.

我猜是否是 idt.c 裡面 pic_remap 有錯？


```bash
❯ make clean; make; make run
❯ make debug
qemu-system-i386 -cdrom myos.iso -d int -no-reboot -hda hdd.img -boot d
WARNING: Image format was not specified for 'hdd.img' and probing guessed raw.
         Automatically detecting the format is dangerous for raw images, write operations on block 0 will be restricted.
         Specify the 'raw' format explicitly to remove the restrictions.
SMM: enter
EAX=00000000 EBX=00000000 ECX=02000000 EDX=02000628
ESI=0000000b EDI=02000000 EBP=06feb010 ESP=00006d5c
EIP=000ebede EFL=00000046 [---Z-P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000044 CCD=ffffffff CCO=EFLAGS
EFER=0000000000000000
SMM: after RSM
EAX=00000000 EBX=00000000 ECX=02000000 EDX=02000628
ESI=0000000b EDI=02000000 EBP=06feb010 ESP=00006d5c
EIP=000ebede EFL=00000046 [---Z-P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000044 CCD=00000000 CCO=EFLAGS
EFER=0000000000000000
SMM: enter
EAX=000000b5 EBX=000f7d2d ECX=00001234 EDX=00006cff
ESI=00006cb8 EDI=06ffee71 EBP=00006c78 ESP=00006c78
EIP=00007d2a EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =da80 000da800 ffffffff 008f9300
CS =f000 000f0000 ffffffff 008f9b00
SS =0000 00000000 ffffffff 008f9300
DS =0000 00000000 ffffffff 008f9300
FS =0000 00000000 ffffffff 008f9300
GS =0000 00000000 ffffffff 008f9300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000000 CCD=00006c78 CCO=ADDL
EFER=0000000000000000
SMM: after RSM
EAX=000000b5 EBX=000f7d2d ECX=00001234 EDX=00006cff
ESI=00006cb8 EDI=06ffee71 EBP=00006c78 ESP=00006c78
EIP=000f7d2d EFL=00000046 [---Z-P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000044 CCD=00000000 CCO=EFLAGS
EFER=0000000000000000
SMM: enter
EAX=000000b5 EBX=00007d47 ECX=00005678 EDX=06fffe88
ESI=000e9780 EDI=06ffee71 EBP=00006c78 ESP=00006c78
EIP=000f7d44 EFL=00000012 [----A--] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000008 CCD=00006c64 CCO=ADDL
EFER=0000000000000000
SMM: after RSM
EAX=000000b5 EBX=00007d47 ECX=00005678 EDX=06fffe88
ESI=000e9780 EDI=06ffee71 EBP=00006c78 ESP=00006c78
EIP=00007d47 EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =da80 000da800 ffffffff 00809300
CS =f000 000f0000 ffffffff 00809b00
SS =0000 00000000 ffffffff 00809300
DS =0000 00000000 ffffffff 00809300
FS =0000 00000000 ffffffff 00809300
GS =0000 00000000 ffffffff 00809300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000004 CCD=00000001 CCO=EFLAGS
EFER=0000000000000000
SMM: enter
EAX=000000b5 EBX=000f7d2d ECX=00001234 EDX=000069ff
ESI=000069a2 EDI=06ffee71 EBP=00006962 ESP=00006962
EIP=00007d2a EFL=00000002 [-------] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =da80 000da800 ffffffff 008f9300
CS =f000 000f0000 ffffffff 008f9b00
SS =0000 00000000 ffffffff 008f9300
DS =0000 00000000 ffffffff 008f9300
FS =0000 00000000 ffffffff 008f9300
GS =ca00 000ca000 ffffffff 008f9300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000000 CCD=00006962 CCO=ADDL
EFER=0000000000000000
SMM: after RSM
EAX=000000b5 EBX=000f7d2d ECX=00001234 EDX=000069ff
ESI=000069a2 EDI=06ffee71 EBP=00006962 ESP=00006962
EIP=000f7d2d EFL=00000046 [---Z-P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000044 CCD=00000000 CCO=EFLAGS
EFER=0000000000000000
SMM: enter
EAX=000000b5 EBX=00007d47 ECX=00005678 EDX=00000005
ESI=00000000 EDI=06ffee71 EBP=00006962 ESP=00006962
EIP=000f7d44 EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000008 CCD=0000694e CCO=ADDL
EFER=0000000000000000
SMM: after RSM
EAX=000000b5 EBX=00007d47 ECX=00005678 EDX=00000005
ESI=00000000 EDI=06ffee71 EBP=00006962 ESP=00006962
EIP=00007d47 EFL=00000002 [-------] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =da80 000da800 ffffffff 00809300
CS =f000 000f0000 ffffffff 00809b00
SS =0000 00000000 ffffffff 00809300
DS =0000 00000000 ffffffff 00809300
FS =0000 00000000 ffffffff 00809300
GS =ca00 000ca000 ffffffff 00809300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000000 CCD=00000001 CCO=EFLAGS
EFER=0000000000000000
SMM: enter
EAX=000000b5 EBX=000f7d2d ECX=00001234 EDX=000069ff
ESI=0000699c EDI=06ffee71 EBP=0000695c ESP=0000695c
EIP=00007d2a EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =da80 000da800 ffffffff 00809300
CS =f000 000f0000 ffffffff 00809b00
SS =0000 00000000 ffffffff 00809300
DS =0000 00000000 ffffffff 00809300
FS =0000 00000000 ffffffff 00809300
GS =ca00 000ca000 ffffffff 00809300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000000 CCD=0000695c CCO=ADDL
EFER=0000000000000000
SMM: after RSM
EAX=000000b5 EBX=000f7d2d ECX=00001234 EDX=000069ff
ESI=0000699c EDI=06ffee71 EBP=0000695c ESP=0000695c
EIP=000f7d2d EFL=00000046 [---Z-P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000044 CCD=00000000 CCO=EFLAGS
EFER=0000000000000000
SMM: enter
EAX=000000b5 EBX=00007d47 ECX=00005678 EDX=00000003
ESI=06fd1fb0 EDI=06ffee71 EBP=0000695c ESP=0000695c
EIP=000f7d44 EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000008 CCD=00006948 CCO=ADDL
EFER=0000000000000000
SMM: after RSM
EAX=000000b5 EBX=00007d47 ECX=00005678 EDX=00000003
ESI=06fd1fb0 EDI=06ffee71 EBP=0000695c ESP=0000695c
EIP=00007d47 EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =da80 000da800 ffffffff 00809300
CS =f000 000f0000 ffffffff 00809b00
SS =0000 00000000 ffffffff 00809300
DS =0000 00000000 ffffffff 00809300
FS =0000 00000000 ffffffff 00809300
GS =ca00 000ca000 ffffffff 00809300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000004 CCD=00000001 CCO=EFLAGS
EFER=0000000000000000
SMM: enter
EAX=000000b5 EBX=000f7d2d ECX=00001234 EDX=000069ff
ESI=000069a2 EDI=06ffee71 EBP=00006962 ESP=00006962
EIP=00007d2a EFL=00000002 [-------] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =da80 000da800 ffffffff 00809300
CS =f000 000f0000 ffffffff 00809b00
SS =0000 00000000 ffffffff 00809300
DS =0000 00000000 ffffffff 00809300
FS =0000 00000000 ffffffff 00809300
GS =ca00 000ca000 ffffffff 00809300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000000 CCD=00006962 CCO=ADDL
EFER=0000000000000000
SMM: after RSM
EAX=000000b5 EBX=000f7d2d ECX=00001234 EDX=000069ff
ESI=000069a2 EDI=06ffee71 EBP=00006962 ESP=00006962
EIP=000f7d2d EFL=00000046 [---Z-P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000044 CCD=00000000 CCO=EFLAGS
EFER=0000000000000000
SMM: enter
EAX=000000b5 EBX=00007d47 ECX=00005678 EDX=00000005
ESI=00000000 EDI=06ffee71 EBP=00006962 ESP=00006962
EIP=000f7d44 EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000008 CCD=0000694e CCO=ADDL
EFER=0000000000000000
SMM: after RSM
EAX=000000b5 EBX=00007d47 ECX=00005678 EDX=00000005
ESI=00000000 EDI=06ffee71 EBP=00006962 ESP=00006962
EIP=00007d47 EFL=00000002 [-------] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =da80 000da800 ffffffff 00809300
CS =f000 000f0000 ffffffff 00809b00
SS =0000 00000000 ffffffff 00809300
DS =0000 00000000 ffffffff 00809300
FS =0000 00000000 ffffffff 00809300
GS =ca00 000ca000 ffffffff 00809300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000000 CCD=00000001 CCO=EFLAGS
EFER=0000000000000000
SMM: enter
EAX=000000b5 EBX=000f7d2d ECX=00001234 EDX=000069ff
ESI=0000699c EDI=06ffee71 EBP=0000695c ESP=0000695c
EIP=00007d2a EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =da80 000da800 ffffffff 00809300
CS =f000 000f0000 ffffffff 00809b00
SS =0000 00000000 ffffffff 00809300
DS =0000 00000000 ffffffff 00809300
FS =0000 00000000 ffffffff 00809300
GS =ca00 000ca000 ffffffff 00809300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000000 CCD=0000695c CCO=ADDL
EFER=0000000000000000
SMM: after RSM
EAX=000000b5 EBX=000f7d2d ECX=00001234 EDX=000069ff
ESI=0000699c EDI=06ffee71 EBP=0000695c ESP=0000695c
EIP=000f7d2d EFL=00000046 [---Z-P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000044 CCD=00000000 CCO=EFLAGS
EFER=0000000000000000
SMM: enter
EAX=000000b5 EBX=00007d47 ECX=00005678 EDX=00000003
ESI=06f31fb0 EDI=06ffee71 EBP=0000695c ESP=0000695c
EIP=000f7d44 EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00c09b00 DPL=0 CS32 [-RA]
SS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00c09300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     000f6220 00000037
IDT=     000f625e 00000000
CR0=00000011 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000008 CCD=00006948 CCO=ADDL
EFER=0000000000000000
SMM: after RSM
EAX=000000b5 EBX=00007d47 ECX=00005678 EDX=00000003
ESI=06f31fb0 EDI=06ffee71 EBP=0000695c ESP=0000695c
EIP=00007d47 EFL=00000006 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =da80 000da800 ffffffff 00809300
CS =f000 000f0000 ffffffff 00809b00
SS =0000 00000000 ffffffff 00809300
DS =0000 00000000 ffffffff 00809300
FS =0000 00000000 ffffffff 00809300
GS =ca00 000ca000 ffffffff 00809300
LDT=0000 00000000 0000ffff 00008200
TR =0000 00000000 0000ffff 00008b00
GDT=     00000000 00000000
IDT=     00000000 000003ff
CR0=00000010 CR2=00000000 CR3=00000000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000004 CCD=00000001 CCO=EFLAGS
EFER=0000000000000000
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x08
Servicing hardware INT=0x09
Servicing hardware INT=0x08
Servicing hardware INT=0x09
Servicing hardware INT=0x08
Servicing hardware INT=0xf9
     0: v=f9 e=0000 i=0 cpl=0 IP=0008:00100dc9 pc=00100dc9 SP=0010:0010bfd8 env->regs[R_EAX]=00105ffa
EAX=00105ffa EBX=001070a8 ECX=00000135 EDX=00000199
ESI=00000000 EDI=00000000 EBP=00000000 ESP=0010bfd8
EIP=00100dc9 EFL=00200212 [----A--] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 0010c000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     0010c080 0000002f
IDT=     0010c2a0 000007ff
CR0=80000011 CR2=00000000 CR3=00144000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000018 CCD=0010bfd0 CCO=ADDL
EFER=0000000000000000
check_exception old: 0xffffffff new 0xd
     1: v=0d e=07ca i=0 cpl=0 IP=0008:00100dc9 pc=00100dc9 SP=0010:0010bfd8 env->regs[R_EAX]=00105ffa
EAX=00105ffa EBX=001070a8 ECX=00000135 EDX=00000199
ESI=00000000 EDI=00000000 EBP=00000000 ESP=0010bfd8
EIP=00100dc9 EFL=00200212 [----A--] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 0010c000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     0010c080 0000002f
IDT=     0010c2a0 000007ff
CR0=80000011 CR2=00000000 CR3=00144000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000018 CCD=0010bfd0 CCO=ADDL
EFER=0000000000000000
check_exception old: 0xd new 0xd
     2: v=08 e=0000 i=0 cpl=0 IP=0008:00100dc9 pc=00100dc9 SP=0010:0010bfd8 env->regs[R_EAX]=00105ffa
EAX=00105ffa EBX=001070a8 ECX=00000135 EDX=00000199
ESI=00000000 EDI=00000000 EBP=00000000 ESP=0010bfd8
EIP=00100dc9 EFL=00200212 [----A--] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 0010c000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     0010c080 0000002f
IDT=     0010c2a0 000007ff
CR0=80000011 CR2=00000000 CR3=00144000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000018 CCD=0010bfd0 CCO=ADDL
EFER=0000000000000000
check_exception old: 0x8 new 0xd
```

---
kernel.c
```c
#include "tty.h"
#include "utils.h"
#include "gdt.h"
#include "idt.h"
#include "paging.h"
#include "timer.h"
#include "pmm.h"
#include "kheap.h"
#include "ata.h"
#include "mbr.h"
#include "vfs.h"
#include "simplefs.h"
#include "elf.h"
#include "task.h"
#include "multiboot.h"
#include "gfx.h"
#include "mouse.h"

void setup_filesystem(uint32_t part_lba, multiboot_info_t* mbd) {
    kprintf("[Kernel] Setting up SimpleFS environment...\n");
    simplefs_mount(part_lba);
    simplefs_format(part_lba, 10000);
    simplefs_create_file(1, "hello.txt", "This is the content of the very first file ever created on Simple OS!\n", 70);

    // 【修改】自動巡覽所有模組並寫入 HDD
    if (mbd->mods_count > 0) {
        multiboot_module_t* mod = (multiboot_module_t*)mbd->mods_addr;
        char* filenames[] = {"shell.elf", "echo.elf", "cat.elf", "ping.elf", "pong.elf", "touch.elf", "ls.elf", "rm.elf", "mkdir.elf"};
        uint32_t expected_mods = sizeof(filenames) / sizeof(filenames[0]);

        for(uint32_t i = 0; i < mbd->mods_count && i < expected_mods; i++) {
            uint32_t size = mod[i].mod_end - mod[i].mod_start;
            kprintf("[Kernel] Installing [%s] to HDD (Size: %d bytes)...\n", filenames[i], size);
            simplefs_create_file(1, filenames[i], (char*)mod[i].mod_start, size);
        }
    }

    // simplefs_list_files();

}

void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    (void)magic; // 忽略未使用的警告

    terminal_initialize();
    kprintf("=== Simple OS Booting ===\n");

    // --- 系統底層基礎建設 ---
    init_gdt();
    init_idt();
    init_paging();
    init_pmm(16384);
    init_kheap();

    // 啟動圖形引擎！
    init_gfx(mbd);
    init_mouse();

    __asm__ volatile ("sti");

    kprintf("=== OS Subsystems Ready ===\n\n");

    while (1) { __asm__ volatile ("hlt"); }
}
```


---


idt.c
```c
// IDT: Interrupt Descriptor Table
// Intel 8259: https://zh.wikipedia.org/zh-tw/Intel_8259
// PIC: [Programmable Interrupt Controller](https://en.wikipedia.org/wiki/Programmable_interrupt_controller)
#include "idt.h"
#include "io.h"

extern void kprintf(const char* format, ...);

// 宣告一個長度為 256 的 IDT 陣列
idt_entry_t idt_entries[256];
idt_ptr_t   idt_ptr;

// 外部組合語言函式：載入 IDT 與中斷處理入口
extern void idt_flush(uint32_t);
extern void isr0();     // 第 0  號中斷：的 Assembly 進入點
extern void isr32();    // 第 32 號中斷：Timer IRQ 0
extern void isr33();    // 第 33 號中斷：宣告組合語言的鍵盤跳板
extern void isr44();    // 第 44 號中斷：Mouse Handler
extern void isr128();   // 第 128 號中斷：system calls

// 設定單一 IDT 條目的輔助函式
static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt_entries[num].base_lo = base & 0xFFFF;
    idt_entries[num].base_hi = (base >> 16) & 0xFFFF;
    idt_entries[num].sel     = sel;
    idt_entries[num].always0 = 0;
    // flags 0x8E 代表：這是一個 32-bit 的中斷閘 (Interrupt Gate)，運行在 Ring 0，且此條目有效(Present)
    idt_entries[num].flags   = flags;
}

// 這是當「除以零」發生時，實際會執行的 C 語言函式
void isr0_handler(void) {
    kprintf("\n[KERNEL PANIC] Exception 0: Divide by Zero!\n");
    kprintf("System Halted.\n");
    // 發生嚴重錯誤，直接把系統凍結
    __asm__ volatile ("cli; hlt");
}

// 初始化 8259 PIC，將 IRQ 0~15 重映射到 IDT 的 32~47
// Intel 8259: https://zh.wikipedia.org/zh-tw/Intel_8259
void pic_remap() {
    // 儲存原本的遮罩 (Masks)
    uint8_t a1 = inb(0x21);
    uint8_t a2 = inb(0xA1);
    (void)a1; (void)a2; // 消除 unused variable 警告

    // 開始初始化序列 (ICW1)
    outb(0x20, 0x11);
    outb(0xA0, 0x11);

    // [Day53] change -- start
    // [關鍵設定] 遮罩設定：0 代表開啟，1 代表屏蔽
    // 為了讓 IRQ 12 (滑鼠) 通過，我們必須：
    // 1. 開啟 Master PIC 的 IRQ 2 (讓 Slave 的訊號能傳遞過來)
    //    0xF8 的二進位是 1111 1000 (開啟 0, 1, 2)
    outb(0x21, 0xF8);

    // 2. 開啟 Slave PIC 的 IRQ 12 (對應 Slave 的第 4 根腳位，8+4=12)
    //    0xEF 的二進位是 1110 1111 (只開啟第 4 個 bit)
    outb(0xA1, 0xEF);
    // [Day53] change -- end

    // ICW3: 告訴 Master PIC 有一個 Slave 接在 IRQ2
    outb(0x21, 0x04);
    // ICW3: 告訴 Slave PIC 它的串聯身份
    outb(0xA1, 0x02);

    // ICW4: 設定為 8086 模式
    outb(0x21, 0x01);
    outb(0xA1, 0x01);

    // [關鍵設定] 遮罩設定：0 代表開啟，1 代表屏蔽
    // 我們目前只開啟 IRQ1 (鍵盤)，關閉其他所有硬體中斷 (避免 Timer 狂噴中斷干擾我們)
    // 0xFD 的二進位是 1111 1101 (第 1 個 bit 是 0，代表開啟鍵盤)
    // 0xFC 的二進位是 1111 1100 (第 0 和第 1 個 bit 都是 0，代表開啟 Timer 與 Keyboard)
    outb(0x21, 0xFC); // [修改] 從 0xFD 變成 0xFC
    outb(0xA1, 0xFF);
}


// --- 公開 API ---

void init_idt(void) {
    idt_ptr.limit = sizeof(idt_entry_t) * 256 - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    // 先把 256 個中斷全部清空 (避免指到未知的記憶體)
    // 這裡我們簡單用迴圈清零 (你也可以 include 昨天寫的 memset)
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }

    // 掛載第 0 號中斷：除以零
    // 0x08 是我們昨天在 GDT 設定的 Kernel Code Segment
    idt_set_gate(0, (uint32_t)isr0, 0x08, 0x8E);

    // [新增] 重新映射 PIC
    pic_remap();

    // [新增] 掛載第 32 號中斷 (IRQ0 Timer)
    idt_set_gate(32, (uint32_t)isr32, 0x08, 0x8E);

    // [新增] 掛載第 33 號中斷 (IRQ1 鍵盤)
    idt_set_gate(33, (uint32_t)isr33, 0x08, 0x8E);

    // [Day53] 掛載第 44 號中斷 (IRQ1 Mouse)
    idt_set_gate(44, (uint32_t)isr44, 0x08, 0x8E);

    // [新增] 掛載第 128 號中斷 (System Call)
    // 注意！旗標是 0xEE (允許 Ring 3 呼叫)
    idt_set_gate(128, (uint32_t)isr128, 0x08, 0xEE);

    // 呼叫組合語言，正式套用新的 IDT
    idt_flush((uint32_t)&idt_ptr);
}
```


---
mouse.c
```c
#include <stdint.h>
#include "io.h"
#include "mouse.h"
#include "gfx.h"
#include "tty.h"

// 宣告你現有的 IO 函式 (假設你有 inb 和 outb，如果沒有請在 utils.c 補上)
extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t data);

// 滑鼠狀態
static uint8_t mouse_cycle = 0;
static int8_t  mouse_byte[3];
static int mouse_x = 400; // 預設在螢幕正中間
static int mouse_y = 300;

// 等待鍵盤控制器就緒
static void mouse_wait(uint8_t a_type) {
    uint32_t timeout = 100000;
    if (a_type == 0) {
        while (timeout--) { if ((inb(0x64) & 1) == 1) return; }
    } else {
        while (timeout--) { if ((inb(0x64) & 2) == 0) return; }
    }
}

// 寫入指令給滑鼠
static void mouse_write(uint8_t write) {
    mouse_wait(1);
    outb(0x64, 0xD4); // 告訴控制器，下一個 byte 要送給滑鼠
    mouse_wait(1);
    outb(0x60, write);
    mouse_wait(0);
    inb(0x60); // 讀取 ACK 回應
}

// 初始化 PS/2 滑鼠
void init_mouse(void) {
    kprintf("[Mouse] Initializing PS/2 Mouse...\n");

    // 1. 啟用附屬裝置 (Mouse)
    mouse_wait(1);
    outb(0x64, 0xA8);

    // 2. 啟用中斷 (IRQ 12)
    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    uint8_t status = (inb(0x60) | 2);
    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);

    // 3. 設定滑鼠預設值並開始回報資料
    mouse_write(0xF6);
    mouse_write(0xF4);

    // 畫出初始游標
    draw_cursor(mouse_x, mouse_y);
}

// 【滑鼠中斷處理程式】當滑鼠移動或點擊時，IRQ 12 會呼叫這裡！
void mouse_handler(void) {
    uint8_t status = inb(0x64);
    while (status & 0x01) { // 檢查是否有資料可讀
        int8_t mouse_in = inb(0x60);

        // 滑鼠每次傳送 3 個 bytes (封包)
        switch (mouse_cycle) {
            case 0:
                if (mouse_in & 0x08) { // 檢查封包合法性 (bit 3 必須是 1)
                    mouse_byte[0] = mouse_in;
                    mouse_cycle++;
                }
                break;
            case 1:
                mouse_byte[1] = mouse_in;
                mouse_cycle++;
                break;
            case 2:
                mouse_byte[2] = mouse_in;
                mouse_cycle = 0;

                // 取出 X 和 Y 的位移量
                int dx = mouse_byte[1];
                int dy = mouse_byte[2];

                // 更新座標 (Y 軸在電腦螢幕是往下遞增，所以要減 dy)
                mouse_x += dx;
                mouse_y -= dy;

                // 邊界碰撞測試 (避免游標跑出螢幕)
                if (mouse_x < 0) mouse_x = 0;
                if (mouse_x > 790) mouse_x = 790; // 保留 10 像素給游標本身
                if (mouse_y < 0) mouse_y = 0;
                if (mouse_y > 590) mouse_y = 590;

                // 重畫游標！
                draw_cursor(mouse_x, mouse_y);
                break;
        }
        status = inb(0x64); // 繼續讀取直到清空
    }

    // ==========================================
    // 【關鍵新增】發送 EOI (End of Interrupt) 給 PIC
    // ==========================================
    // 因為 IRQ 12 在 Slave PIC，所以必須同時通知 Slave 和 Master！
    outb(0xA0, 0x20); // 通知 Slave PIC
    outb(0x20, 0x20); // 通知 Master PIC
}
```
