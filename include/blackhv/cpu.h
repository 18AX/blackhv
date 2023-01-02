#ifndef CPU_HEADER
#define CPU_HEADER

/** GDT **/

#define SEGMENT_SELECTOR(index, ti, rpl)                                       \
    ((index << 3) | ((ti & 0x1) << 2) | (rpl & 0x3))

#define GDT_TYPE_ACCESSED 0x1
// For data segment
#define GDT_TYPE_WR (0x1 << 1)
#define GDT_TYPE_EXP (0x1 << 2) // For expansion

// For code segment
#define GDT_TYPE_RD (0x1 << 1)
#define GDT_TYPE_CFMG (0x1 << 2) // Conforming
#define GDT_TYPE_EXEC (0x1 << 3)

/** CR0 **/

#define CR0_PE (1 << 0) // Protection enable
#define CR0_MP (1 << 1) // Monitor coprocessor
#define CR0_EM (1 << 2)
#define CR0_TS (1 << 3)
#define CR0_ET (1 << 4)
#define CR0_NE (1 << 5)
#define CR0_WP (1 << 16)
#define CR0_AM (1 << 18)
#define CR0_NW (1 << 29)
#define CR0_CD (1 << 30)
#define CR0_PG (1 << 31)

#endif