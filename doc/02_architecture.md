# Software Architecture {#architecture}

This page describes the software architecture of the project.

## Overview
As mentioned in the [introduction](@ref mainpage) page, there are two main hardware interfaces: the IEEE-488 bus and the microSD card.

### IEEE-488 bus

Here's an overview of how it works, but interested readers are strongly encouraged to check the [references](@ref references) for details of how the IEEE-488 bus works and how the Commodore PET uses it.

There is an 8-bit bidirectional data portion of the interface which is how data and addresses are transferred.  There are five interface control lines and three transfer control lines.  This table shows the names of the IEEE-488 bus lines and what it means:

Name    | Function        |  Group            |
--------|-----------------|-------------------|
DIO1    | data in/out     | data bus          |
DIO2    | data in/out     | data bus          |
DIO3    | data in/out     | data bus          |
DIO4    | data in/out     | data bus          |
DIO5    | data in/out     | data bus          |
DIO6    | data in/out     | data bus          |
DIO7    | data in/out     | data bus          |
DIO8    | data in/out     | data bus          |
REN     | Remote Enable   | interface mgmt    |
IFC     | Interface Clear | interface mgmt    |
SRQ     | Service Request | interface mgmt    |
ATN     | Attention       | interface mgmt    |
EOI     | End or Identify | interface mgmt    |
DAV     | Data Valid      | transfer control  |
NRFD    | Not Ready For Data | transfer control  |
NDAC    | Not Data Accepted  | transfer control  |

The PET is hardwired to act as the sole controller on the IEEE-488 bus, which means that anything connecting to the PET can only act as a peripheral device.  With the IEEE-488 bus, such devices can operate as a Talker, a Listener or both.  

In brief, the PET asserts ATN and sends an address on the data bus.  Then ATN is de-asserted, and data is transferred with a handshake using DAV, NRFD and NDAC.  The first diagram shows just a primary and secondary address being transmitted.  Note that signals are active low.

@startuml
binary "ATteNtion" as ATN
binary "DAta Valid" as DAV
binary "Not Ready For Data" as NRFD
binary "Not Data ACcepted" as NDAC
concise "Data" as D
@0
ATN is High
DAV is High
NRFD is High
NDAC is Low
D is {-}
@1 
ATN is Low
@2
D is pri_addr
@3
DAV is Low
@4
NRFD is Low
@5
NDAC is High
@6
DAV is High
@7
NDAC is Low
D is {-}
@8 
NRFD is High
@9
D is sec_addr
@10
DAV is Low
@11
NRFD is Low
NDAC is High
@12
DAV is High
@13
NRFD is High
NDAC is Low
D is {-}
@14
ATN is High
@enduml

The second timing diagram shows the transfer of data immediately after the addressing shown above.  This sequence happens repeatedly until the last date byte, when the EOI line is also asserted (pulled low).

@startuml
binary "ATteNtion" as ATN
binary "DAta Valid" as DAV
binary "Not Ready For Data" as NRFD
binary "Not Data ACcepted" as NDAC
concise "Data" as D
@0
ATN is High
DAV is High
NRFD is High
NDAC is Low
D is {-}
@1 
D is data
@2
DAV is Low
@3
NRFD is Low
NDAC is High
@4
DAV is High
@5
NDAC is Low
D is {-}
NRFD is High
@enduml

### microSD card
The microSD interface can operate in a serial mode via a standard 3-wire SPI bus interface, which is the most common way it is used in embedded systems.  The only additional line needed is a CS (Chip Select) which would allow a host microprocessor to talk with multiple devices, but only select one at a time. 

## Further resources {#references}
### IEEE-488
One valuable resource describing the details of how the IEEE-488 bus works and is implemented on the Commodore PET is a book published in 1980.  I bought my softcover copy at Computerland in 1981, but it's now also available on [the Internet Archive](https://archive.org/details/PET_and_the_IEEE488_Bus).  The book is _PET and the IEEE 488 Bus (GPIB)_ , published in 1980 by OSBORNE/McGraw Hill, and written by Fisher, E. R. (Eugene Ralph), 1940-; Jensen, C. W. (C. William).
