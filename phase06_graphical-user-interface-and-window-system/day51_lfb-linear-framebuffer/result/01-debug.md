
跑起來之後有問題，底下是 core dump 以及修改的 source code.

grub.cfg 需要改嗎？

```bash
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
Servicing hardware INT=0x09
Servicing hardware INT=0x08
Servicing hardware INT=0x09
Servicing hardware INT=0x20
     0: v=20 e=0000 i=0 cpl=0 IP=0008:001009a5 pc=001009a5 SP=0010:0010afe8 env->regs[R_EAX]=fd000000
EAX=fd000000 EBX=00106084 ECX=00105865 EDX=00000000
ESI=001d4c00 EDI=001d5000 EBP=00000000 ESP=0010afe8
EIP=001009a5 EFL=00200287 [--S--PC] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 0010b000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     0010b080 0000002f
IDT=     0010b0e0 000007ff
CR0=80000011 CR2=00000000 CR3=00142000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=001d5000 CCD=fffffc00 CCO=SUBL
EFER=0000000000000000
check_exception old: 0xffffffff new 0xe
     1: v=0e e=0002 i=0 cpl=0 IP=0008:001009c3 pc=001009c3 SP=0010:0010afe8 CR2=fd04e390
EAX=fd000000 EBX=00106084 ECX=00000190 EDX=0004e390
ESI=00000064 EDI=001d5000 EBP=00000000 ESP=0010afe8
EIP=001009c3 EFL=00200206 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 0010b000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     0010b080 0000002f
IDT=     0010b0e0 000007ff
CR0=80000011 CR2=fd04e390 CR3=00142000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000190 CCD=0004e390 CCO=ADDL
EFER=0000000000000000
check_exception old: 0xe new 0xd
     2: v=08 e=0000 i=0 cpl=0 IP=0008:001009c3 pc=001009c3 SP=0010:0010afe8 env->regs[R_EAX]=fd000000
EAX=fd000000 EBX=00106084 ECX=00000190 EDX=0004e390
ESI=00000064 EDI=001d5000 EBP=00000000 ESP=0010afe8
EIP=001009c3 EFL=00200206 [-----P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =002b 0010b000 00000068 0000e900 DPL=3 TSS32-avl
GDT=     0010b080 0000002f
IDT=     0010b0e0 000007ff
CR0=80000011 CR2=fd04e390 CR3=00142000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000
DR6=ffff0ff0 DR7=00000400
CCS=00000190 CCD=0004e390 CCO=ADDL
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

// 紀錄畫布資訊
uint8_t* fb_addr = 0;
uint32_t fb_pitch = 0;

// 【畫筆魔法】在螢幕上的 x, y 座標點上一個顏色！
void put_pixel(int x, int y, uint32_t color) {
    if (fb_addr == 0) return;

    // 計算記憶體偏移量：Y * 每一行的寬度 + X * 每個像素的寬度(4 Bytes)
    uint32_t offset = (y * fb_pitch) + (x * 4);

    // 32-bit 色彩格式通常是 BGRA (小端序)
    fb_addr[offset]     = color & 0xFF;         // Blue
    fb_addr[offset + 1] = (color >> 8) & 0xFF;  // Green
    fb_addr[offset + 2] = (color >> 16) & 0xFF; // Red
    // fb_addr[offset + 3] 是 Alpha (透明度)，通常不用管
}

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

    // [Day51] add -- start
    // 【繪圖基礎建設】映射 Framebuffer 實體記憶體
    if (mbd->flags & (1 << 12)) { // 檢查 GRUB 是否成功回傳 Framebuffer
        fb_addr = (uint8_t*) (uint32_t) mbd->framebuffer_addr;
        fb_pitch = mbd->framebuffer_pitch;

        // 算出畫布總容量 (約 1.92 MB)，把它逐頁 Map 進 Kernel 宇宙裡！
        uint32_t fb_size = mbd->framebuffer_pitch * mbd->framebuffer_height;
        for (uint32_t i = 0; i < fb_size; i += 4096) {
            // 實體位址與虛擬位址 1:1 映射，權限設為 3 (Ring 0/3 皆可存取)
            map_page((uint32_t)fb_addr + i, (uint32_t)fb_addr + i, 3);
        }
    }
    // [Day51] add -- end

    __asm__ volatile ("sti");

    // [Day51] add -- start
    // ==============================================================
    // 🎨 畫圖時間！畫一個 200x200 的藍色正方形！
    // ==============================================================
    for (int y = 100; y < 300; y++) {
        for (int x = 100; x < 300; x++) {
            put_pixel(x, y, 0x0000FF); // 0x0000FF = 藍色
        }
    }

    // 畫一顆紅色的太陽在右上角
    for (int y = 50; y < 150; y++) {
        for (int x = 600; x < 700; x++) {
            put_pixel(x, y, 0xFF0000); // 0xFF0000 = 紅色
        }
    }
    // [Day51] add -- end

    // kprintf("=== OS Subsystems Ready ===\n\n");

    // // --- 儲存與檔案系統 ---
    // uint32_t part_lba = parse_mbr();
    // if (part_lba == 0) {
    //     kprintf("Disk Error: No partition found.\n");
    //     while(1) __asm__ volatile("hlt");
    // }

    // // 呼叫我們重構好的函式！
    // setup_filesystem(part_lba, mbd);

    // // [Day35] start
    // // --- 應用程式載入與排程 ---
    // kprintf("[Kernel] Fetching 'shell.elf' from Virtual File System...\n");
    // fs_node_t* app_node = simplefs_find(1, "shell.elf");

    // if (app_node != 0) {
    //     uint8_t* app_buffer = (uint8_t*) kmalloc(app_node->length);
    //     vfs_read(app_node, 0, app_node->length, app_buffer);
    //     uint32_t entry_point = elf_load((elf32_ehdr_t*)app_buffer);

    //     if (entry_point != 0) {
    //         // kprintf("Creating ONE Initial User Task (Init Process)...\n\n");
    //         kprintf("Creating Initial Process...\n\n");

    //         init_multitasking();

    //         // 為唯一的 Shell 分配 User Stack
    //         uint32_t ustack1_phys = (uint32_t) pmm_alloc_page();
    //         map_page(0x083FF000, ustack1_phys, 7);

    //         // 建立 Ring 3 主任務
    //         create_user_task(entry_point, 0x083FF000 + 4096);

    //         kprintf("[Kernel] Kernel dropping to idle state (Ring0 -> Ring3).\n");

    //         // 【關鍵修復】Kernel 必須功成身退，把自己宣告死亡！
    //         // 否則排程器會以為 Kernel 還活著，就會一直把 CPU 切給 Kernel 的 hlt！
    //         exit_task();
    //     }
    // } else {
    //     kprintf("[Kernel] Error: Shell (shell.elf) not found on disk.\n");
    // }
    // // [Day35] end

    while (1) { __asm__ volatile ("hlt"); }
}
```

---

multiboot.h
```c
#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>

// GRUB 載入完畢後，eax 暫存器一定會存放這個魔法數字，用來證明它是合法的 Bootloader
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

// 模組 (Module) 的結構
typedef struct {
    uint32_t mod_start;  // 這個檔案被 GRUB 載入到的起始實體位址
    uint32_t mod_end;    // 這個檔案的結束位址
    uint32_t string;     // 模組名稱字串的位址
    uint32_t reserved;
} multiboot_module_t;

// Multiboot 資訊結構 (MBI)
typedef struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;

    // ==========================================
    // 【新增】Multiboot Framebuffer 資訊
    // ==========================================
    uint64_t framebuffer_addr;   // 畫布的實體記憶體起點
    uint32_t framebuffer_pitch;  // 每一橫列的位元組數 (Bytes per line)
    uint32_t framebuffer_width;  // 畫布寬度 (Pixels)
    uint32_t framebuffer_height; // 畫布高度 (Pixels)
    uint8_t  framebuffer_bpp;    // 色彩深度 (Bits per pixel，例如 32)
    uint8_t  framebuffer_type;
    uint8_t  color_info[6];
} __attribute__((packed)) multiboot_info_t;

#endif
```


---

boot.S
```asm
; [目的] 核心進入點與環境初始化
; [流程]
; GRUB 載入器 -> 檢查 Multiboot Header -> 跳轉至 _start
;                                        |
;    +-----------------------------------+
;    V
; [ _start ]
;    |--> [ 設定 ESP ] (指向 16KB 堆疊頂部)
;    |--> [ 壓入參數 ] (eax=Magic, ebx=MBI)
;    |--> [ call kernel_main ] (進入 C 語言世界)
;    |--> [ 核心停止 ] (若 C 返回，則進入 cli/hlt 無限迴圈)


; ==========================================
; Multiboot Header 升級為圖形模式
; ==========================================
MAGIC       equ 0x1BADB002
; Flag Bit 0: Align (對齊)
; Flag Bit 1: MemInfo (記憶體資訊)
; Flag Bit 2: VideoMode (影片模式)  <-- 【新增】我們需要這個！
FLAGS       equ 0x00000007
CHECKSUM    equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM
    ; AOUT kludge 位址欄位 (不使用，但因為啟用了 Bit 2，必須填 5 個 0 佔位)
    dd 0, 0, 0, 0, 0

    ; 【新增】VBE (VESA BIOS Extensions) 圖形模式設定
    dd 0    ; mode_type: 0 代表線性圖形模式 (Linear Graphics)
    dd 800  ; width: 寬度 800 像素
    dd 600  ; height: 高度 600 像素
    dd 32   ; depth: 色彩深度 32-bit (ARGB)


; === 定義 Multiboot 1 的常數 ===
; MBALIGN  equ  1 << 0            ; 請求 GRUB 將載入的模組對齊 4KB 邊界
; MEMINFO  equ  1 << 1            ; 請求 GRUB 提供記憶體資訊 (包含模組位址)
; MAGIC    equ  0x1BADB002        ; [關鍵] Multiboot 1 的專屬魔法數字
; FLAGS    equ  MBALIGN | MEMINFO ; 組合旗標 (通常是 3)
; CHECKSUM equ -(MAGIC + FLAGS)   ; 檢查碼，證明這是一個合法的標頭

; section .multiboot
; align 4
;     dd MAGIC
;     dd FLAGS
;     dd CHECKSUM

; === 定義未初始化的資料區段 (BSS) 來存放堆疊 ===
section .bss
align 16
stack_bottom:
    resb 16384 ; 保留 16 KB 的空間 (16 * 1024 bytes)
stack_top:

; === 系統進入點 ===
section .text
extern kernel_main

global _start
_start:
    ; 1. 設定堆疊指標
    mov esp, stack_top

    ; 2. 將 GRUB 傳遞的參數壓入堆疊 (Multiboot 1 規範)
    ; eax 會包含 0x2BADB002，ebx 會包含 MBI 結構的位址
    push ebx    ; 參數 2: multiboot_info_t* (MBI 結構指標)
    push eax    ; 參數 1: magic number (魔法數字)

    ; 3. 呼叫 C 語言的進入點
    call kernel_main

    ; 4. 清理堆疊 (兩個參數共 8 bytes)
    add esp, 8

    ; 5. 安全降落區：如果 Kernel 意外結束，就在這裡無限休眠
    cli             ; Clear Interrupts
.hang:
    hlt             ; https://en.wikipedia.org/wiki/HLT_(x86_instruction)
    jmp .hang
```
