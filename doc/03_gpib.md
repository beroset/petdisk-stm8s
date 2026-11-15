# IEEE-488 bus {#gpib}

Here's an overview of how it works, but interested readers are strongly encouraged to check the [references](@ref references) for details of how the IEEE-488 bus works and how the Commodore PET uses it.

There is an 8-bit bidirectional data portion of the interface which is how data and addresses are transferred.  There are five interface control lines and three transfer control lines.  This table shows the names of the IEEE-488 bus lines and what it means, as well as the PET connector J1 pin and processor pin:

Name    | Function        |  Group            | PET J1 | CPU |
--------|-----------------|-------------------|--------|-----|
DIO1    | data in/out     | data bus          |   1    |  25 |
DIO2    | data in/out     | data bus          |   2    |  26 |
DIO3    | data in/out     | data bus          |   3    |  27 |
DIO4    | data in/out     | data bus          |   4    |  28 |
DIO5    | data in/out     | data bus          |   A    |  29 |
DIO6    | data in/out     | data bus          |   B    |  16 |
DIO7    | data in/out     | data bus          |   C    |  15 |
DIO8    | data in/out     | data bus          |   D    |  14 |
REN     | Remote Enable   | interface mgmt    |   E    |  -- |
IFC     | Interface Clear | interface mgmt    |   9    |  -- |
SRQ     | Service Request | interface mgmt    |   10   |  -- |
ATN     | Attention       | interface mgmt    |   11   |  11 |
EOI     | End or Identify | interface mgmt    |   5    |  12 |
DAV     | Data Valid      | transfer control  |   6    |   2 |
NRFD    | Not Ready For Data | transfer control  | 7   |   3 |
NDAC    | Not Data Accepted  | transfer control  | 8   |   8 |

The PET is hardwired to act as the sole controller on the IEEE-488 bus, which means that anything connecting to the PET can only act as a peripheral device.  With the IEEE-488 bus, such devices can operate as a Talker, a Listener or both.  

In brief, the PET asserts ATN and sends an address on the data bus.  Then ATN is de-asserted, and data is transferred with a handshake using DAV, NRFD and NDAC.  The first diagram shows just a primary and secondary address being transmitted.  Note that signals are active low, (0V) so when the description mentions that a line is "asserted" it means that it is low.

@startuml
binary "ATteNtion (ATN)" as ATN
binary "DAta Valid (DAV)" as DAV
binary "Not Ready For Data (NRFD)" as NRFD
binary "Not Data ACcepted (NDAC)" as NDAC
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
binary "ATteNtion (ATN)" as ATN
binary "DAta Valid (DAV)" as DAV
binary "Not Ready For Data (NRFD)" as NRFD
binary "Not Data ACcepted (NDAC)" as NDAC
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

#### Details on IEEE-488 data transfer
Although there is only one Controller (the PET), all devices on the IEEE-488 bus may operate as either a Talker or a Listener.  The DAV line belongs to the Talker, while the NRFD and NDAC lines belong to the Listener.  For each of the described transitions, the number in parentheses refers to the first timing diagram above.  The numbers on the X-axis are only intended to indicate sequence and are not meant to show equal time periods.  One byte transfer (pri_addr in the diagram above) starts with the Listener de-asserts the NDAC and NRFD lines indicating that it's ready for data (0).  Then the Talker puts data on the DIO lines (2) and, after letting the data settle for 11µs, asserts the DAV line to indicate the data is valid (3).  The Listener responds by asserting NRFD (4).  Then the Listener de-asserts NDAC, signaling that it has read the data (5).  The Talker detects this and de-asserts DAV (6).  The Listener detects this condition and asserts NDAC (7) and then de-asserts NRFD (8), indicating that it is once again ready to receive data.

#### Old vs. new PET ROMs
The old ROMs (that show `***` in the title line at power up) and the new ROMs (that show `###` in the title at power up) also operate differently with the IEEE-488 bus.  The new ROMs correctly implement IEEE-488 commands, while the old ROMs do not.  The primary differences is that the old ROMs keep ATN asserted through the entire data transfer, while the new ROMs correctly de-assert when data transfer starts and through sending EOI coincident with the last data byte.  The new ROMs then assert ATN for the UNListen and any Listen Address Group (LAG)/Talk Address Group (TAG) and Secondary Command Group (SCG) bytes that follow the data transfer.

### PET commands using IEEE-488 bus
The following table describes the commands that use the IEEE-488 bus, and whether they are implemented in this project.  In the table, *f* is logical file number, *d* is the device primary address, *s* is the secondary address, and *n* is a string holding the file name.  The PET can only use a maximum of ten logical files at any one time.

|   command        | support (old/new) | First byte | Second byte |
|------------------|-------------|------------|-------------|
| OPEN f, d, s, n  | no/no       | 0x20 + d   | 0xF0 + s    |
| INPUT# f, n      | no/no       | | |           
| GET# f, n        | no/no       | | |           
| PRINT# f, n      | no/no       | | |           
| CMD f, n         | no/no       | | |           
| CLOSE f          | no/no       | | |           
| SAVE n, d, s     | yes/no      | 0x20 + d   | 0xF1        |
| LOAD n, d, s     | no/no       | 0x40 + d   | 0xF1/0xF0   |

For the LOAD command, the SCG is F0 for new ROMs and F1 for old ones.  For the SAVE command F1 is used by both.

## Further resources {#references}
### IEEE-488
One valuable resource describing the details of how the IEEE-488 bus works and is implemented on the Commodore PET is a book published in 1980.  I bought my softcover copy at Computerland in 1981, but it's now also available on [the Internet Archive](https://archive.org/details/PET_and_the_IEEE488_Bus).  The book is _PET and the IEEE 488 Bus (GPIB)_ , published in 1980 by OSBORNE/McGraw Hill, and written by Fisher, E. R. (Eugene Ralph), 1940-; Jensen, C. W. (C. William).
