# Game Boy CPU Opcode Reference

---

## FLAGS REFERENCE

| Flag | Bit | Meaning    |
| ---- | --- | ---------- |
| Z    | 7   | Zero       |
| N    | 6   | Subtract   |
| H    | 5   | Half Carry |
| C    | 4   | Carry      |

---

## LEGEND

* `r` = A,B,C,D,E,H,L
* `(HL)` = memory at HL
* `d8` = 8-bit immediate
* `d16` = 16-bit immediate
* `e8` = signed 8-bit
* `a16` = 16-bit address

---

## 0x00–0x3F (Loads + Inc/Dec + Control)

| Opcode | Mnemonic    | Cycles | Flags   | Notes |
| ------ | ----------- | ------ | ------- | ----- |
| 00     | NOP         | 4      | -       |       |
| 01     | LD BC,d16   | 12     | -       |       |
| 02     | LD (BC),A   | 8      | -       |       |
| 03     | INC BC      | 8      | -       |       |
| 04     | INC B       | 4      | Z 0 H - |       |
| 05     | DEC B       | 4      | Z 1 H - |       |
| 06     | LD B,d8     | 8      | -       |       |
| 07     | RLCA        | 4      | 0 0 0 C | Z=0   |
| 08     | LD (a16),SP | 20     | -       |       |
| 09     | ADD HL,BC   | 8      | - 0 H C |       |
| 0A     | LD A,(BC)   | 8      | -       |       |
| 0B     | DEC BC      | 8      | -       |       |
| 0C     | INC C       | 4      | Z 0 H - |       |
| 0D     | DEC C       | 4      | Z 1 H - |       |
| 0E     | LD C,d8     | 8      | -       |       |
| 0F     | RRCA        | 4      | 0 0 0 C | Z=0   |

---

## 0x40–0x7F (8-bit Loads)

| Pattern | Meaning |
| ------- | ------- |
| 40–7F   | LD r,r  |

👉 All:

* Cycles: 4
* Flags: -

Exception:

* `(HL)` involved → 8 cycles

---

## 0x80–0xBF (ALU)

### ADD / ADC

| Opcode | Mnemonic | Flags   |
| ------ | -------- | ------- |
| 80–87  | ADD A,r  | Z 0 H C |
| 88–8F  | ADC A,r  | Z 0 H C |

---

### SUB / SBC

| Opcode | Mnemonic | Flags   |
| ------ | -------- | ------- |
| 90–97  | SUB r    | Z 1 H C |
| 98–9F  | SBC A,r  | Z 1 H C |

---

### AND / XOR / OR / CP

| Opcode | Mnemonic | Flags   |
| ------ | -------- | ------- |
| A0–A7  | AND r    | Z 0 1 0 |
| A8–AF  | XOR r    | Z 0 0 0 |
| B0–B7  | OR r     | Z 0 0 0 |
| B8–BF  | CP r     | Z 1 H C |

---

## 0xC0–0xFF (Control + Stack + Jumps)

### Returns

| Opcode | Mnemonic | Cycles |
| ------ | -------- | ------ |
| C0     | RET NZ   | 8/20   |
| C8     | RET Z    | 8/20   |
| C9     | RET      | 16     |

---

### Calls

| Opcode | Mnemonic | Cycles |
| ------ | -------- | ------ |
| CD     | CALL a16 | 24     |

---

### Jumps

| Opcode | Mnemonic  | Cycles |
| ------ | --------- | ------ |
| C3     | JP a16    | 16     |
| C2     | JP NZ,a16 | 12/16  |
| 18     | JR e8     | 12     |

---

### Stack

| Opcode | Mnemonic | Cycles |
| ------ | -------- | ------ |
| C5     | PUSH BC  | 16     |
| C1     | POP BC   | 12     |

---

### Misc

| Opcode | Mnemonic | Flags   |
| ------ | -------- | ------- |
| 27     | DAA      | Z - 0 C |
| 2F     | CPL      | - 1 1 - |
| 3F     | CCF      | - 0 0 C |
| 37     | SCF      | - 0 0 1 |

---

## CB PREFIX (0xCB)

---

### Rotates

| Opcode | Mnemonic | Cycles | Flags   |
| ------ | -------- | ------ | ------- |
| 00–07  | RLC r    | 8      | Z 0 0 C |
| 08–0F  | RRC r    | 8      | Z 0 0 C |
| 10–17  | RL r     | 8      | Z 0 0 C |
| 18–1F  | RR r     | 8      | Z 0 0 C |

---

### Shifts

| Opcode | Mnemonic | Flags   |
| ------ | -------- | ------- |
| 20–27  | SLA r    | Z 0 0 C |
| 28–2F  | SRA r    | Z 0 0 C |
| 38–3F  | SRL r    | Z 0 0 C |

---

### BIT

| Opcode | Mnemonic | Flags   |
| ------ | -------- | ------- |
| 40–7F  | BIT b,r  | Z 0 1 - |

---

### RES

| Opcode | Mnemonic |
| ------ | -------- |
| 80–BF  | RES b,r  |

---

### SET

| Opcode | Mnemonic |
| ------ | -------- |
| C0–FF  | SET b,r  |

---

## SPECIAL CASES

### ADD SP,e8

* Flags:

  * Z = 0
  * N = 0
  * H = from bit 3
  * C = from bit 7

---

### HALT

* Cycles: 4
* Special behavior with IME

---

### STOP

* Used for power saving

---

## IMPORTANT RULES

* `(HL)` operations → +4 cycles
* Conditional jumps:

  * not taken → base cycles
  * taken → +4 cycles

---

## NOTES

* Never guess flags — use rules
* Separate:

  * execution
  * flags
  * cycles
* CB opcodes often double cycles for `(HL)`

---
