# General
- [ ] Design rules set appropriately for fab house?
  - Which Fab is being used? : JLC
  - How many PCB layers?:
    - [ ] 2
    - [X] 4
    - [ ] 6
    - [ ] 8
  - Special PCB Requirements?:
    - [ ] Blind Buried Vias
    - [ ] Capped Vias
    - [ ] Rigid/Flex
- [X] ERC passes?
  - [X] ERC report included?
- [X] DRC passes?
  - [X] DRC report included?
- [X] Appropriate copper weight selected?

# BOM
- [X] Components in stock?
- [X] Specialty components recommended for new designs?
- [X] All specialty components added to design library?
- [X] Resistors / Capacitors rated appropriately?

# Schematic
## General
- [ ] Net classes assigned for critical nets?
- [X] Test points added to critical nets?
- [X] Gound points near test points?
- [X] Proper and judicious use of heirarchical schematics? 
- [X] Diodes properly oriented?
- [X] Schematic included in manufacturing folder?

## Digital
- [X] Decoupling caps on all digital chips?
- [ ] Inputs appropriately protected?
- [X] Bus signals appropriately terminated?
	- [X] I2C Terminated?
	- [ ] SPI tied off?
- [X] Ferrite beads placed on high speed / high noise ICs?
	- PLLs
	- Oscillators
	- Chips with very high speeds

## MCU
- [X] JTAG broken out?
- [X] LED/Debug IO broken out?
- [X] Able to initiate bootloader?
  - i.e. are boot/reset lines broken out to jumper/button?
- [X] GPIOs don't interfere with boot/reset pins

## Power Electronics
- [ ] Proper capacitance on both sides of load switches?
- [X] Power supplies rated appropriately?
- [X] Power supplies have sufficient bulk capacitance?
- [X] All components connected to correct power net?
- [ ] Voltage drop accounted for over cables?

# Layout
- [X] Silkscreen Complete?
	- [X] Version?
	- [X] Name?
	- [X] Connectors labeled?
    - [X] Designators placed/spaced and sized properly for all components?
	- [X] Silkscreen doesn't overlap with drill hits or other features that'd make it illegible?
- [X] Design properly floorplanned?
	- [ ] Analog / Digital / Power separated?
- [ ] Differential signals skew matched?
- [ ] Clocked data lines skew matched?
- [X] Routing only on appropriate layers?
- [ ] Appropriate separation for high voltage?
- [X] Elements with >1 row of pin headers have their own footprint?
- [X] (If ordering via gerber upload) Gerber files included in manufacturing folder?

