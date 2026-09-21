CC = gcc
LD = ld
RUBY = ruby

CFLAGS = -ffreestanding \
         -fno-stack-protector \
         -fno-pie \
         -mno-red-zone \
         -mcmodel=kernel \
         -Wall \
         -Wextra \
         -Iinclude \
         -Iconsole \
         -Iinput \
         -Iprotein \
         -Iprotein/commands \
         -Iorigin \
         -Itask \
         -Ischeduler \
         -Ifilesystem \
         -Ieditor \
         -Istorage

LDFLAGS = -T linker.ld \
          -nostdlib

BOOT_OBJECT = boot/boot.o
TASK_SWITCH_OBJECT = task/switch.o

COMMAND_SOURCES = $(wildcard protein/commands/*.c)
COMMAND_OBJECTS = $(COMMAND_SOURCES:.c=.o)

AMINO_SOURCES = $(wildcard protein/commands/*.amino)
AMINO_GENERATED = protein/amino/generated.c
AMINO_OBJECT = protein/amino/generated.o

C_OBJECTS = \
    core/kernel.o \
    console/console.o \
    input/keyboard.o \
    input/line.o \
    system/info.o \
    protein/protein.o \
    protein/command.o \
    protein/amino/amino.o \
    task/task.o \
    scheduler/scheduler.o \
    filesystem/tfs.o \
    storage/storage.o \
    origin/origin.o \
    editor/editor.o \
    $(COMMAND_OBJECTS) \
    $(AMINO_OBJECT)

OBJECTS = \
    $(BOOT_OBJECT) \
    $(TASK_SWITCH_OBJECT) \
    $(C_OBJECTS)

.PHONY: all clean run

all: titin.iso

boot/boot.o: boot/boot.S
	$(CC) -c $< -o $@

task/switch.o: task/switch.S
	$(CC) -c $< -o $@

core/kernel.o: core/kernel.c
	$(CC) $(CFLAGS) -c $< -o $@

console/console.o: console/console.c
	$(CC) $(CFLAGS) -c $< -o $@

input/keyboard.o: input/keyboard.c
	$(CC) $(CFLAGS) -c $< -o $@

input/line.o: input/line.c
	$(CC) $(CFLAGS) -c $< -o $@

system/info.o: system/info.c
	$(CC) $(CFLAGS) -c $< -o $@

protein/protein.o: protein/protein.c
	$(CC) $(CFLAGS) -c $< -o $@

protein/command.o: protein/command.c
	$(CC) $(CFLAGS) -c $< -o $@

protein/amino/amino.o: protein/amino/amino.c
	$(CC) $(CFLAGS) -c $< -o $@

task/task.o: task/task.c
	$(CC) $(CFLAGS) -c $< -o $@

scheduler/scheduler.o: scheduler/scheduler.c
	$(CC) $(CFLAGS) -c $< -o $@

filesystem/tfs.o: filesystem/tfs.c
	$(CC) $(CFLAGS) -c $< -o $@

storage/storage.o: storage/storage.c
	$(CC) $(CFLAGS) -c $< -o $@

origin/origin.o: origin/origin.c
	$(CC) $(CFLAGS) -c $< -o $@

editor/editor.o: editor/editor.c
	$(CC) $(CFLAGS) -c $< -o $@

protein/commands/%.o: protein/commands/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(AMINO_GENERATED): $(AMINO_SOURCES) tools/embed_amino.rb
	$(RUBY) tools/embed_amino.rb $@ $(AMINO_SOURCES)

$(AMINO_OBJECT): $(AMINO_GENERATED)
	$(CC) $(CFLAGS) -c $< -o $@

titin.bin: $(OBJECTS)
	$(LD) $(LDFLAGS) -o $@ $^

iso/boot/titin.bin: titin.bin
	mkdir -p iso/boot
	cp titin.bin iso/boot/titin.bin

iso/boot/grub/grub.cfg:
	mkdir -p iso/boot/grub
	printf 'multiboot2 /boot/titin.bin\nboot\n' > iso/boot/grub/grub.cfg

titin.disk:
	dd if=/dev/zero of=$@ bs=1M count=8

titin.iso: iso/boot/titin.bin iso/boot/grub/grub.cfg
	grub-mkrescue -o $@ iso

run: titin.iso titin.disk
	qemu-system-x86_64 \
		-cdrom titin.iso \
		-drive file=titin.disk,format=raw,if=ide

clean:
	rm -rf \
		core/*.o \
		console/*.o \
		input/*.o \
		system/*.o \
		protein/*.o \
		protein/amino/*.o \
		protein/amino/generated.c \
		protein/commands/*.o \
		task/*.o \
		scheduler/*.o \
		filesystem/*.o \
		storage/*.o \
		origin/*.o \
		editor/*.o \
		boot/*.o \
		titin.bin \
		titin.iso \
		iso
