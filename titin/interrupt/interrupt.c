#include <stdint.h>
#include "interrupt.h"
#include "../input/mouse.h"

#define PIC_MASTER_COMMAND 0x20
#define PIC_MASTER_DATA 0x21
#define PIC_SLAVE_COMMAND 0xA0
#define PIC_SLAVE_DATA 0xA1

#define PIC_ICW1 0x11
#define PIC_ICW4 0x01

#define PIC_MASTER_VECTOR 32
#define PIC_SLAVE_VECTOR 40

#define PIC_EOI 0x20

#define IDT_PRESENT 0x80
#define IDT_INTERRUPT_GATE 0x0E

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;
    uint8_t type;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t reserved;
} __attribute__((packed)) idt_entry;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_pointer;

static idt_entry idt[256];

static idt_pointer idt_descriptor;

extern void interrupt_irq1(void);
extern void interrupt_irq12(void);

static void port_write(
    uint16_t port,
    uint8_t value
)
{
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(value),
          "Nd"(port)
    );
}

static uint8_t port_read(
    uint16_t port
)
{
    uint8_t value;

    __asm__ volatile (
        "inb %1, %0"
        : "=a"(value)
        : "Nd"(port)
    );

    return value;
}

static void io_wait(void)
{
    __asm__ volatile (
        "outb %%al, $0x80"
        :
        : "a"(0)
    );
}

static void idt_set(
    uint8_t vector,
    void (*handler)(void)
)
{
    uint64_t address =
        (uint64_t)(uintptr_t)handler;

    idt[vector].offset_low =
        (uint16_t)(address & 0xFFFF);

    idt[vector].selector =
        0x08;

    idt[vector].ist =
        0;

    idt[vector].type =
        IDT_PRESENT |
        IDT_INTERRUPT_GATE;

    idt[vector].offset_middle =
        (uint16_t)(
            (address >> 16) &
            0xFFFF
        );

    idt[vector].offset_high =
        (uint32_t)(
            address >> 32
        );

    idt[vector].reserved = 0;
}

static void pic_remap(void)
{
    uint8_t master_mask =
        port_read(PIC_MASTER_DATA);

    uint8_t slave_mask =
        port_read(PIC_SLAVE_DATA);

    port_write(
        PIC_MASTER_COMMAND,
        PIC_ICW1
    );

    io_wait();

    port_write(
        PIC_SLAVE_COMMAND,
        PIC_ICW1
    );

    io_wait();

    port_write(
        PIC_MASTER_DATA,
        PIC_MASTER_VECTOR
    );

    io_wait();

    port_write(
        PIC_SLAVE_DATA,
        PIC_SLAVE_VECTOR
    );

    io_wait();

    port_write(
        PIC_MASTER_DATA,
        0x04
    );

    io_wait();

    port_write(
        PIC_SLAVE_DATA,
        0x02
    );

    io_wait();

    port_write(
        PIC_MASTER_DATA,
        PIC_ICW4
    );

    io_wait();

    port_write(
        PIC_SLAVE_DATA,
        PIC_ICW4
    );

    io_wait();

    port_write(
        PIC_MASTER_DATA,
        master_mask
    );

    port_write(
        PIC_SLAVE_DATA,
        slave_mask
    );
}

static void pic_set_masks(void)
{
    uint8_t master_mask = 0xFF;
    uint8_t slave_mask = 0xFF;

    master_mask &=
        (uint8_t)~(1 << 1);

    slave_mask &=
        (uint8_t)~(1 << 4);

    master_mask &=
        (uint8_t)~(1 << 2);

    port_write(
        PIC_MASTER_DATA,
        master_mask
    );

    port_write(
        PIC_SLAVE_DATA,
        slave_mask
    );
}

static void pic_end_of_interrupt(
    uint64_t vector
)
{
    if (vector >= 40) {
        port_write(
            PIC_SLAVE_COMMAND,
            PIC_EOI
        );
    }

    port_write(
        PIC_MASTER_COMMAND,
        PIC_EOI
    );
}

void interrupt_start(void)
{
    __asm__ volatile (
        "cli"
    );

    for (
        uint64_t i = 0;
        i < 256;
        i++
    ) {
        idt[i].offset_low = 0;
        idt[i].selector = 0;
        idt[i].ist = 0;
        idt[i].type = 0;
        idt[i].offset_middle = 0;
        idt[i].offset_high = 0;
        idt[i].reserved = 0;
    }

    idt_set(
        33,
        interrupt_irq1
    );

    idt_set(
        44,
        interrupt_irq12
    );

    idt_descriptor.limit =
        sizeof(idt) - 1;

    idt_descriptor.base =
        (uint64_t)(uintptr_t)idt;

    __asm__ volatile (
        "lidt %0"
        :
        : "m"(idt_descriptor)
    );

    pic_remap();

    pic_set_masks();
}

void interrupt_enable(void)
{
    __asm__ volatile (
        "sti"
    );
}

void interrupt_disable(void)
{
    __asm__ volatile (
        "cli"
    );
}

void interrupt_handler(
    uint64_t vector
)
{
    if (vector == 44) {
        mouse_interrupt();
    }

    pic_end_of_interrupt(
        vector
    );
}
