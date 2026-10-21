#!/usr/bin/env python3
# rewrite sweet_early_panic wholesale: locate 'void\nsweet_early_panic' up to
# the next 'void\nsweet_early_relocate' and replace the function body.
f = "osfmk/arm64/sweet_early.c"
lines = open(f).read().split("\n")
i = next(i for i, l in enumerate(lines) if l.strip() == "void" and "sweet_early_panic" in lines[i + 1])
j = next(j for j in range(i, len(lines)) if lines[j].strip() == "void" and "sweet_early_relocate" in lines[j + 1])
new_fn = [
    "void",
    "sweet_early_panic(const char *fmt, va_list *args)",
    "{",
    "\tif (early_disabled || early_panicking) {",
    "\t\treturn;",
    "\t}",
    "\tearly_panicking = 1;",
    "\tearly_stage_write(0x77665000u | (early_last_stage & 0xff));",
    "\t__asm__ volatile(\"dsb sy\" ::: \"memory\");",
    "\tsweet_early_string(\"\\nSWEET EARLY PANIC: \");",
    "\tva_list copy;",
    "\tva_copy(copy, *args);",
    "\t_doprnt(fmt, &copy, sweet_early_putc, 16);",
    "\tva_end(copy);",
    "\tsweet_early_putc('\\n');",
    "\t__asm__ volatile(\"dsb sy\" ::: \"memory\");",
    "\t/* PSCI SYSTEM_RESET (0x84000009): the marks are already in",
    "\t * IMEM/ramoops -- reset through ABL instead of falling through",
    "\t * to system_off; a powered-off phone reports nothing. */",
    "\t__asm__ volatile(\"mov x0, #0x84000009; smc #0\" ::: \"x0\", \"memory\");",
    "\tfor (;;) {",
    "\t\t__asm__ volatile(\"wfe\");",
    "\t}",
    "}",
    "",
]
lines[i:j] = new_fn
open(f, "w").write("\n".join(lines))
print("sweet_early_panic rewritten")
