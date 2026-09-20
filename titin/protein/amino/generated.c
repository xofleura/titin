#include "amino.h"

static const char amino_source_0[] = "command about\n    write \" |-|    *           Titin \"\n    info version\n    newline\n    write \" |-|   _    *  __\"\n    newline\n    write \" |-|   |  *    |/'\"\n    newline\n    write \" |-|   |~*~~~o~|\"\n    newline\n    write \" |-|   |  O o *|\"\n    newline\n    write \"/___\\  |o___O__|\"\n    newline\nend\n";
static const amino_command amino_command_0 __attribute__((section("amino_commands"), used)) = { "about", amino_source_0 };

static const char amino_source_1[] = "command amifetch\n    newline\n    write \" |-|    *          +--------------------+\"\n    newline\n    write \" |-|   _    *  __  | TITIN \"\n    info version\n    newline\n    write \" |-|   |  *    |/' | ARCHITECTURE: \"\n    info architecture\n    newline\n    write \" |-|   |~*~~~o~|   | KERNEL: \"\n    info version\n    newline\n    write \" |-|   |  O o *|   | BUFFER: ON\"\n    newline\n    write \"/___\\  |o___O__|   | CONSOLE: VGA\"\n    newline\n    write \"                   +--------------------+\"\n    newline\nend\n";
static const amino_command amino_command_1 __attribute__((section("amino_commands"), used)) = { "amifetch", amino_source_1 };

static const char amino_source_2[] = "command arch\n    info architecture\n    newline\nend\n";
static const amino_command amino_command_2 __attribute__((section("amino_commands"), used)) = { "arch", amino_source_2 };

static const char amino_source_3[] = "command cls\n    clear\nend\n";
static const amino_command amino_command_3 __attribute__((section("amino_commands"), used)) = { "cls", amino_source_3 };

static const char amino_source_4[] = "command enter\n    enter\nend\n";
static const amino_command amino_command_4 __attribute__((section("amino_commands"), used)) = { "enter", amino_source_4 };

static const char amino_source_5[] = "command exit\n    exit\nend\n";
static const amino_command amino_command_5 __attribute__((section("amino_commands"), used)) = { "exit", amino_source_5 };

static const char amino_source_6[] = "command fs\n     fs\nend\n";
static const amino_command amino_command_6 __attribute__((section("amino_commands"), used)) = { "fs", amino_source_6 };

static const char amino_source_7[] = "command hello\n    write \"Hello from Amino!\"\n    newline\nend\n";
static const amino_command amino_command_7 __attribute__((section("amino_commands"), used)) = { "hello", amino_source_7 };

static const char amino_source_8[] = "command help\n    commands\nend\n";
static const amino_command amino_command_8 __attribute__((section("amino_commands"), used)) = { "help", amino_source_8 };

static const char amino_source_9[] = "command tasks\n    tasks\nend\n";
static const amino_command amino_command_9 __attribute__((section("amino_commands"), used)) = { "tasks", amino_source_9 };

static const char amino_source_10[] = "command version\n    write \"Titin \"\n    info version\n    newline\nend\n";
static const amino_command amino_command_10 __attribute__((section("amino_commands"), used)) = { "version", amino_source_10 };

static const char amino_source_11[] = "command zm\n    zm\nend\n";
static const amino_command amino_command_11 __attribute__((section("amino_commands"), used)) = { "zm", amino_source_11 };

