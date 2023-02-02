#include <blackhv/atapi.h>
#include <blackhv/io.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define PRIMARY_DCR 0x3F6
#define SECONDARY_DCR 0x376

/* Drive selection bytes */
#define ATA_PORT_MASTER 0x00
#define ATA_PORT_SLAVE 0x10

/* ATA bus IO ports */
#define PRIMARY_REG 0x1F0
#define SECONDARY_REG 0x170

/* ATA I/O registers */
#define ATA_REG_DATA(PORT) (PORT)
#define ATA_REG_FEATURES(PORT) ((PORT) + 1)
#define ATA_REG_ERROR_INFO(PORT) ((PORT) + 1)
#define ATA_REG_SECTOR_COUNT(PORT) ((PORT) + 2)
#define ATA_REG_SECTOR_NB(PORT) ((PORT) + 3)
#define ATA_REG_LBA_LO(PORT) ((PORT) + 3)
#define ATA_REG_CYLINDER_LOW(PORT) ((PORT) + 4)
#define ATA_REG_LBA_MI(PORT) ((PORT) + 4)
#define ATA_REG_CYLINDER_HIGH(PORT) ((PORT) + 5)
#define ATA_REG_LBA_HI(PORT) ((PORT) + 5)
#define ATA_REG_DRIVE(PORT) ((PORT) + 6)
#define ATA_REG_HEAD(PORT) ((PORT) + 6)
#define ATA_REG_COMMAND(PORT) ((PORT) + 7)
#define ATA_REG_STATUS(PORT) ((PORT) + 7)

/* Status bits */
#define ERR (1 << 0)
#define DRQ (1 << 3)
#define SRV (1 << 4)
#define DF (1 << 5)
#define RDY (1 << 6)
#define BSY (1 << 7)

#define ABRT (1 << 2)

/* ATAPI signature */
#define ATAPI_SIG_SC 0x01
#define ATAPI_SIG_LBA_LO 0x01
#define ATAPI_SIG_LBA_MI 0x14
#define ATAPI_SIG_LBA_HI 0xEB

/* ATA commands */
#define IDENTIFY_PACKET_DEVICE 0xA1
#define PACKET 0xA0

/* SCSI commands */
#define READ_12 0xA8

#define DPO (1 << 4)

#define ATAPI_BLK_CACHE_SZ 256
#define CD_BLOCK_SZ 2048
#define PACKET_SZ 12

#define PACKET_AWAIT_COMMAND 1
#define PACKET_DATA_TRANSMIT 2
#define PACKET_COMMAND_COMPLETE 3

struct SCSI_packet
{
    u8 op_code;
    u8 flags_lo;
    u8 lba_hi;
    u8 lba_mihi;
    u8 lba_milo;
    u8 lba_lo;
    u8 transfer_length_hi;
    u8 transfer_length_mihi;
    u8 transfer_length_milo;
    u8 transfer_length_lo;
    u8 flags_hi;
    u8 control;
} __packed;

static u8 selected_drive = 0;

static void ignore_outb(u16 port, u8 data, void *params)
{
    (void)port;
    (void)data;
    (void)params;
    return;
}

static void select_outb(u16 port, u8 data, void *params)
{
    (void)params;
    (void)port;
    selected_drive = data;
}

static struct SCSI_packet curr_pkt = { 0 };
static u32 byte_read = 0;

static u8 to_send[CD_BLOCK_SZ];
static u32 byte_sent = 0;

static void handle_scsi_packet(int disk_fd)
{
    if (curr_pkt.op_code != READ_12)
    {
        fprintf(stderr, "SCSI Op code not supported: %d\n", curr_pkt.op_code);
        return;
    }

    u32 lba = curr_pkt.lba_lo | (curr_pkt.lba_milo << 8)
        | (curr_pkt.lba_mihi << 16) | (curr_pkt.lba_hi) << 24;
    lseek(disk_fd, lba * CD_BLOCK_SZ, SEEK_SET);
    read(disk_fd, to_send, CD_BLOCK_SZ);
    byte_sent = 0;
}

static int receiving = 0;
static int sending = 0;

static u8 signature_inb(u16 port, void *params)
{
    // Only supporting one drive on PRIMARY PORT MASTER
    if (selected_drive != ATA_PORT_MASTER)
    {
        return 0;
    }

    switch (port)
    {
    case ATA_REG_SECTOR_COUNT(PRIMARY_REG):
        if (sending)
        {
            byte_sent = 0;
            sending = 0;
            return PACKET_COMMAND_COMPLETE;
        }
        if (receiving)
        {
            int disk_fd = (int)(u64)params;
            handle_scsi_packet(disk_fd);
            byte_read = 0;
            receiving = 0;
            return PACKET_DATA_TRANSMIT;
        }
        return ATAPI_SIG_SC;
    case ATA_REG_LBA_LO(PRIMARY_REG):
        return ATAPI_SIG_LBA_LO;
    case ATA_REG_LBA_MI(PRIMARY_REG):
        return ATAPI_SIG_LBA_MI;
    case ATA_REG_LBA_HI(PRIMARY_REG):
        return ATAPI_SIG_LBA_HI;
    default:
        return 0;
    }
}

static u8 status_inb(u16 port, void *params)
{
    (void)port;
    (void)params;
    // Always ready to receive data and never busy
    return DRQ;
}

static void data_outw(u16 port, u16 data, void *params)
{
    (void)port;
    (void)params;
    if (byte_read > sizeof(struct SCSI_packet) - 2)
    {
        return;
    }

    receiving = 1;
    *((u16 *)((u8 *)&curr_pkt + byte_read)) = data;
    byte_read += 2;
}

static u16 data_inw(u16 port, void *params)
{
    (void)port;
    (void)params;
    if (byte_sent >= CD_BLOCK_SZ)
    {
        return 0;
    }

    sending = 1;
    u16 word = *((u16 *)(to_send + byte_sent));
    byte_sent += 2;
    return word;
}

void atapi_init(vm_t *vm, int disk_fd)
{
    (void)vm; // TODO handle per-vm IO

    struct handler ignore_outb_handler = { .outb_handler = ignore_outb };
    io_register_handler(PRIMARY_DCR, ignore_outb_handler);
    io_register_handler(SECONDARY_DCR, ignore_outb_handler);
    io_register_handler(ATA_REG_FEATURES(PRIMARY_REG), ignore_outb_handler);
    io_register_handler(ATA_REG_FEATURES(SECONDARY_REG), ignore_outb_handler);
    io_register_handler(ATA_REG_SECTOR_COUNT(PRIMARY_REG), ignore_outb_handler);
    io_register_handler(ATA_REG_SECTOR_COUNT(SECONDARY_REG),
                        ignore_outb_handler);

    struct handler select_outb_handler = { .outb_handler = select_outb };
    io_register_handler(ATA_REG_DRIVE(PRIMARY_REG), select_outb_handler);
    io_register_handler(ATA_REG_DRIVE(SECONDARY_REG), select_outb_handler);

    struct handler signature_inb_handler = {
        .inb_handler = signature_inb,
        .outb_handler = ignore_outb,
        .params = (void *)(u64)disk_fd,
    };

    for (int i = 2; i <= 5; i++)
    {
        io_register_handler(PRIMARY_REG + i, signature_inb_handler);
        io_register_handler(SECONDARY_REG + i, signature_inb_handler);
    }

    struct handler data_handler = {
        .inw_handler = data_inw,
        .outw_handler = data_outw,
    };
    io_register_handler(ATA_REG_DATA(PRIMARY_REG), data_handler);

    struct handler status_handler = {
        .inb_handler = status_inb,
        .outb_handler = ignore_outb,
    };
    io_register_handler(ATA_REG_STATUS(PRIMARY_REG), status_handler);
}
