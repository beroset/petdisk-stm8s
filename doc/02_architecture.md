# Software Architecture {#architecture}

This page describes the software architecture of the project.

## Overview
As mentioned in the [introduction](@ref mainpage) page, there are two main hardware interfaces: the [IEEE-488 bus](@ref gpib) and the [microSD card](@ref microsd).  This project also uses the UART to communicate with the user for troubleshooting and testing.

## Interrupts
As with many microcontroller projects, the code is primarily interrupt-driven.  After initialization, the `main` loop ends with the WFI (Wait For Interrupt) instruction.  The interrupt sources are:

 - timer2: used to toggle the blinking LED
 - timer3: used in timer routines
 - ATN: a GPIO interrupt for the IEEE-488 bus (B5)
 - DAV: a GPIO interrupt for the IEEE-488 bus (A2)

