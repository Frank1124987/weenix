/******************************************************************************/
/* Important Fall 2024 CSCI 402 usage information:                            */
/*                                                                            */
/* This fils is part of CSCI 402 kernel programming assignments at USC.       */
/*         53616c7465645f5fd1e93dbf35cbffa3aef28f8c01d8cf2ffc51ef62b26a       */
/*         f9bda5a68e5ed8c972b17bab0f42e24b19daa7bd408305b1f7bd6c7208c1       */
/*         0e36230e913039b3046dd5fd0ba706a624d33dbaa4d6aab02c82fe09f561       */
/*         01b0fd977b0051f0b0ce0c69f7db857b1b5e007be2db6d42894bf93de848       */
/*         806d9152bd5715e9                                                   */
/* Please understand that you are NOT permitted to distribute or publically   */
/*         display a copy of this file (or ANY PART of it) for any reason.    */
/* If anyone (including your prospective employer) asks you to post the code, */
/*         you must inform them that you do NOT have permissions to do so.    */
/* You are also NOT permitted to remove or alter this comment block.          */
/* If this comment block is removed or altered in a submitted file, 20 points */
/*         will be deducted.                                                  */
/******************************************************************************/

#pragma once

/* Vendor-strings. */
#define CPUID_VENDOR_AMD          "AuthenticAMD"
#define CPUID_VENDOR_INTEL        "GenuineIntel"
#define CPUID_VENDOR_VIA          "CentaurHauls"
#define CPUID_VENDOR_OLDTRANSMETA "TransmetaCPU"
#define CPUID_VENDOR_TRANSMETA    "GenuineTMx86"
#define CPUID_VENDOR_CYRIX        "CyrixInstead"
#define CPUID_VENDOR_CENTAUR      "CentaurHauls"
#define CPUID_VENDOR_NEXGEN       "NexGenDriven"
#define CPUID_VENDOR_UMC          "UMC UMC UMC "
#define CPUID_VENDOR_SIS          "SiS SiS SiS "
#define CPUID_VENDOR_NSC          "Geode by NSC"
#define CPUID_VENDOR_RISE         "RiseRiseRise"

enum {
        CPUID_FEAT_ECX_SSE3         = 1 << 0,
        CPUID_FEAT_ECX_PCLMUL       = 1 << 1,
        CPUID_FEAT_ECX_DTES64       = 1 << 2,
        CPUID_FEAT_ECX_MONITOR      = 1 << 3,
        CPUID_FEAT_ECX_DS_CPL       = 1 << 4,
        CPUID_FEAT_ECX_VMX          = 1 << 5,
        CPUID_FEAT_ECX_SMX          = 1 << 6,
        CPUID_FEAT_ECX_EST          = 1 << 7,
        CPUID_FEAT_ECX_TM2          = 1 << 8,
        CPUID_FEAT_ECX_SSSE3        = 1 << 9,
        CPUID_FEAT_ECX_CID          = 1 << 10,
        CPUID_FEAT_ECX_FMA          = 1 << 12,
        CPUID_FEAT_ECX_CX16         = 1 << 13,
        CPUID_FEAT_ECX_ETPRD        = 1 << 14,
        CPUID_FEAT_ECX_PDCM         = 1 << 15,
        CPUID_FEAT_ECX_DCA          = 1 << 18,
        CPUID_FEAT_ECX_SSE4_1       = 1 << 19,
        CPUID_FEAT_ECX_SSE4_2       = 1 << 20,
        CPUID_FEAT_ECX_x2APIC       = 1 << 21,
        CPUID_FEAT_ECX_MOVBE        = 1 << 22,
        CPUID_FEAT_ECX_POPCNT       = 1 << 23,
        CPUID_FEAT_ECX_XSAVE        = 1 << 26,
        CPUID_FEAT_ECX_OSXSAVE      = 1 << 27,
        CPUID_FEAT_ECX_AVX          = 1 << 28,

        CPUID_FEAT_EDX_FPU          = 1 << 0,
        CPUID_FEAT_EDX_VME          = 1 << 1,
        CPUID_FEAT_EDX_DE           = 1 << 2,
        CPUID_FEAT_EDX_PSE          = 1 << 3,
        CPUID_FEAT_EDX_TSC          = 1 << 4,
        CPUID_FEAT_EDX_MSR          = 1 << 5,
        CPUID_FEAT_EDX_PAE          = 1 << 6,
        CPUID_FEAT_EDX_MCE          = 1 << 7,
        CPUID_FEAT_EDX_CX8          = 1 << 8,
        CPUID_FEAT_EDX_APIC         = 1 << 9,
        CPUID_FEAT_EDX_SEP          = 1 << 11,
        CPUID_FEAT_EDX_MTRR         = 1 << 12,
        CPUID_FEAT_EDX_PGE          = 1 << 13,
        CPUID_FEAT_EDX_MCA          = 1 << 14,
        CPUID_FEAT_EDX_CMOV         = 1 << 15,
        CPUID_FEAT_EDX_PAT          = 1 << 16,
        CPUID_FEAT_EDX_PSE36        = 1 << 17,
        CPUID_FEAT_EDX_PSN          = 1 << 18,
        CPUID_FEAT_EDX_CLF          = 1 << 19,
        CPUID_FEAT_EDX_DTES         = 1 << 21,
        CPUID_FEAT_EDX_ACPI         = 1 << 22,
        CPUID_FEAT_EDX_MMX          = 1 << 23,
        CPUID_FEAT_EDX_FXSR         = 1 << 24,
        CPUID_FEAT_EDX_SSE          = 1 << 25,
        CPUID_FEAT_EDX_SSE2         = 1 << 26,
        CPUID_FEAT_EDX_SS           = 1 << 27,
        CPUID_FEAT_EDX_HTT          = 1 << 28,
        CPUID_FEAT_EDX_TM1          = 1 << 29,
        CPUID_FEAT_EDX_IA64         = 1 << 30,
        CPUID_FEAT_EDX_PBE          = 1 << 31
};

enum cpuid_requests {
        CPUID_GETVENDORSTRING,
        CPUID_GETFEATURES,
        CPUID_GETTLB,
        CPUID_GETSERIAL,

        CPUID_INTELEXTENDED = 0x80000000,
        CPUID_INTELFEATURES,
        CPUID_INTELBRANDSTRING,
        CPUID_INTELBRANDSTRINGMORE,
        CPUID_INTELBRANDSTRINGEND,
};

static inline void cpuid(int request, uint32_t *a, uint32_t *d)
{
        __asm__ volatile("cpuid":"=a"(*a), "=d"(*d):"0"(request));
}

static inline void cpuid_get_msr(uint32_t msr, uint32_t* lo, uint32_t* hi)
{
	__asm__ volatile("rdmsr":"=a"(*lo),"=d"(*hi):"c"(msr));
}

static inline void cpuid_set_msr(uint32_t msr, uint32_t lo, uint32_t hi)
{
	__asm__ volatile("wrmsr"::"a"(lo),"d"(hi),"c"(msr));
}

static inline void io_wait(void)
{
	__asm__ volatile("jmp 1f\n\t"
			 "1:jmp 2f\n\t"
			 "2:" );
}
