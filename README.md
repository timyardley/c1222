# c1222

A location for the c1222.net codebase that is no longer accessible. **This is not my work**, nor is there any guarantee that the software works in any way.

Note, the sites and contact information below likely will not exist when you see this. You can try to reach out to them, but I doubt you will get a response.

# Original README

C12.22 stack and tools readme, release 0.1c March 20, 2009


Original developer: ITRON, Liberty Lake, WA, USA


## 1.0 General

The stack implements the January 30, 2008 version of the C12.22 standard.  This is *NOT* the final version of the standard which is being published in March 2009.  The stack also has support for receiving messages in March 2006 C12.22 standard format.  If such a message is received, the stack will respond using the same version of the protocol.
  
See "Known Problems" section below.

The stack includes C code for application, transport, and datalink layers.  In addition, two different "physical" layers are included which provide interface to a PC serial port and to a "test link" that allows two stacks in the same PC application to communicate.

In addition to the C12.22 stack several test tools are included that
1) demonstrate how to use the stack
2) provide a meter simulation on a PC that can be interrogated via C12.22 over TCP/IP
3) provide a meter simulation on a PC that communicates serially to a comm module simulation on a PC that allows the meter to be interrogated via C12.22 over TCP/IP
4) allow checking CRC and encryption implementation

The files are licensed under the terms of the CDDL version 1.  See source file headers.  
The Common Development and Distribution License (CDDL) is available at: 
    http://www.sun.com/cddl/cddl.html

For questions about the use of the stack and other files, please contact C1222@itron.com, and/or visit C1222.net.

The stack source is located in the "stack" subdirectory.  The test tools are located and built from the 
"stack\tools" subdirectory.  

## 2.0 List of test tools provided with brief descriptions

The test tools were originally built using Borland C++ version 5.02.

### 2.1 C1222TEST
Includes a meter simulation, a C12.22 device stack and a C12.22 comm module stack connected by a "test link" physical layer.  The comm module stack interfaces to a TCPIP socket allowing interrogation with tools such as tables test bench.

### 2.2 C1222APTITLE
Command line tool allows converting between C12.22 Universal ID in dot delimited and ascii hex forms.

### 2.3 ENCRYPTION
Command line tool allows encrypting or decrypting arbitrary messages.

### 2.4 CRC
Command line tool allows calculating a CRC on arbitrary ascii hex strings.

### 2.5 SIMPLESERIALDEVICE
This has a meter simulation and a C12.22 device stack which uses a PC serial port physical layer.

### 2.6 SIMPLETCPIPCOMMMODULE
This has a communication module stack interfacing to a PC serial port physical layer, and sends application layer messages through a TCPIP socket.

### 2.7 CHECKSUM
Command line tool allows calculating a checksum on arbitrary ascii hex strings.

## 3.0 List of source modules provided with brief descriptions

### 3.1 C12.22 stack

    acsemsg.c                   Handling of ACSE messages including decomposing into components and building messages 
                                and segments, and control of encryption, decryption, and authentication.
    
    acsemsg.h                   Header for acsemsg.
    
    c1219table.h                Header for C12.19 table interface.
    
    c1222.h                     Provides commonly needed typedefs etc for C12.22 library
    
    c1222al.c                   General C12.22 application layer routines.
    
    c1222al.h                   Header for c1222al
    
    c1222al_commmodule.c        C12.22 application layer routines specific to the communication module.
    
    c1222al_device.c            C12.22 application layer routines specific to the C12.22 device (meter register).
    
    c1222al_messagereceiver.c   C12.22 application layer message receiver handles interface with segmenter and receiving 
                                messages from the communication module/device interface.
    
    c1222al_messagesender.c     C12.22 application layer message sender handles interface with segmenter and sending of 
                                messages through the communication module/device interface.
    
    c1222al_receivehistory.c    C12.22 application layer handler for identifying duplicate messages.
    
    c1222al_relay.c             C12.22 application layer routines specific to the C12.22 relay (used in test code only).
    
    c1222al_segmenter.c         C12.22 application layer routines that handle segmenting complete messages into multiple 
                                segments and assembly of multiple segments into a complete message.
    
    c1222al_segmenter.h         Header for c1222al_segmenter
    
    c1222al_tlinterface.c       C12.22 application layer routines used primarily by the transport layer.
    
    c1222aptitle.c              C12.22 application layer routines that deal with application titles (ApTitles)
    
    c1222aptitle.h              Header for C1222aptitle.
    
    c1222dl.c                   C12.22 datalink layer routines
    
    c1222dl.h                   Header for c1222dl
    
    c1222dl_crc.c               Implementation of the CRC used in the datalink and also in the application layer.
    
    c1222dl_crc.h               Header for c1222dl_crc
    
    c1222dl_packet.c            Routines for decomposing c12.22 datalink packets
    
    c1222encrypt.c              Routines for encrypting and decrypting c12.22 messages.
    
    c1222encrypt.h              Header for c1222encrypt
    
    c1222environment.h          Header.

    c1222event.h                Defines c12.22 processing events
    
    c1222localportrouter.c      Implements local port router as a mailbox.
    
    c1222misc.c                 Miscellaneous utility routines used by C12.22 library
    
    c1222misc.h                 Header for c1222misc
    
    c1222pl.h                   Header for physical layer routines
        
    c1222response.h             C12.22 response codes (psem) defines.
    
    c1222server.c               Provides a message server for use in a C12.22 device or communication module (or relay). 
    
    c1222server.h               Header for c1222server
    
    c1222stack.c                Provides access to a stack consisting of an application layer, transport layer, 
                                datalink layer, and physical layer.
    
    c1222stack.h                Header for c1222stack
    
    c1222statistic.h            Header has defines for C12.22 standard statistics and for the current Itron 
                                manufacturers statistics.
    
    c1222tl.c                   C12.22 transport layer routines
    
    c1222tl.h                   Header for c1222tl.
    
    cbitarray.c                 Routines provide the ability to treat an array of bytes as an array of bits, setting, 
                                clearing, and testing individual bits.
    
    cbitarray.h                 Header for cbitarray
    
    comevent_id.h               Header has defines needed for logging of communication related events.
    
    epsem.c                     Provides routines to manage epsem (information content in c12.22 message).
    
    epsem.h                     Header for epsem
    
### 3.2 Test tools
    All of the test tools are Windows command line applications, and are built using 
    Borland C++ 5.02.  Most use the test library.  Most use the stack.  Some of the test tools
    provide usage help if they are run with no parameters.
    
    In C1222TEST, SIMPLESERIALDEVICE, and SIMPLETCPIPCOMMMODULE, the app shows some status info once a
    second or so in its window.  R= count of link resets; AS= application state (2=good); 
    TS= transport state (2=good), RP=received packets, TP=transmitted packets.  
    Each also has a number that is shown that is related to time.
    
    The listed IDE defines are the ones currently defined in the Borland IDE provided.
    
#### 3.2.1 C1222TEST
	Test application has a simple meter simulation with C12.22 stack to communication module (using testlink physical layer) connected to via TCPIP PC port 27015.  Simulated meter aptitle is .68.33.3.  The simulation has a table 0, 3, 5, 51, 52, and 2048.  Tables 5 and 2048 are writable.  The meter is sessionless only.
    
	C1222test.ide               Borland C++ 5.02 IDE
    c1222test.c                 Source for the application
    
    IDE Defines:                WANT_TCPIP, AVOID_PORTAB_H, C1222_INCLUDE_COMMMODULE, C1222_INCLUDE_DEVICE, WANT_LOG, 
                                C1222_INCLUDE_RELAY, C1222TOOL, AVOID_SLB_H, C1222_TABLESERVER_BIGENDIAN                         
                            
#### 3.2.2 C1222APTITLE
	Application converts universal ids between hex and text.
    
	c1222aptitletest.cpp        Source for this application.
    C1222aptitletest.ide        Borland C++ 5.02 IDE
    How to use c1222aptitle.doc Documentation.
    c1222aptitle.exe            Windows application.
    
    IDE Defines:                AVOID_PORTAB_H, AVOID_SLB_H, C1222TOOL
    
#### 3.2.3 ENCRYPTION
	Test application encrypts and decrypts arbitrary hex strings using C12.22 encryption.

    c1222testcbc.ide            Borland C++ 5.02 IDE
    c1222testcbc.exe            Windows application. (source is in c1222encrypt.c)
    
    IDE Defines:                WANT_MAIN, AVOID_PORTAB_H, AVOID_SLB_H, C1222TOOL

#### 3.2.4 CRC
	Application calculates CRC on arbitrary ascii hex strings
    
	Testcrc.ide                 Borland C++ 5.02 IDE (source is embedded in c1222dl_crc.c).
    Testcrc.exe                 Windows application.
    
    IDE Defines:                WANT_MAIN, AVOID_PORTAB_H, C1222TOOL, AVOID_SLB_H
    
#### 3.2.5 TESTLIBRARY
	Library of routines used by the test applications
    
	network.cpp                 Interface to a windows serial port.
    network.hpp                 Header for network.cpp
    uart.cpp                    Connects uart oriented serial to a windows serial port (network).
    uart.h                      Header for uart.cpp
    tcpiptask.cpp               Provides tcpip message interface for test routines
    tcpiptaskinterface.c        Interface to tcpip task.
    testtableserver.c           Implements a table server for a meter simulation with tables 0, 3, 5, 52, and 2048.
    simulatehost.c              Sends a registration response if needed back down the stack.
    c1222pl_pc.c                Physical layer for a Windows PC serial port.
    c1222pl_testlink.c          C12.22 physical layer for a test link allowing multiple stacks in the same Windows application.
    
#### 3.2.6 SIMPLESERIALDEVICE 
	Meter simulation with C12.22 interface to PC serial port.  Meter properties are the same as for C1222TEST.

    simple_serial_device.ide    Borland C++ 5.02 IDE
    simple_serial_device.c      Application source.
    simple_serial_device.exe    Windows application.
    
    IDE Defines:                C1222_DEVICE_ONLY, AVOID_PORTAB_H, C1222_INCLUDE_DEVICE, WANT_LOG, C1222TOOL, AVOID_SLB_H,
                                Bool=Boolean, Signed32=long
    
#### 3.2.7 SIMPLETCPIPCOMMMODULE
	Comm module with C12.22 interface to PC serial port and TCPIP application interface to tcpip port 27015.  A PC running simpleserialdevice should be able to talk to a pc running simpletcpipcommmodule through a serial port.  If it does not seem to be working make sure both are pointed at the right port (first parameter is port number) and maybe try a null modem or try removing a null modem.

    simple_tcpip_cm.ide         Borland C++ 5.02 IDE
    simple_tcpip_cm.c           Application source.
    simple_tcpip_cm.exe         Windows application.
    
    IDE Defines:                C1222_COMM_MODULE, WANT_TCPIP, AVOID_PORTAB_H, C1222_INCLUDE_COMMMODULE, WANT_LOG,
                                C1222TOOL, SEPARATE_REPLY=0, FORCE_BAUD=19200, AVOID_SLB_H, 
                                Bool=Boolean, Signed32=long
                                
#### 3.2.8 CHECKSUM
	Test application calculates the checksum of an ascii hex string.
	
    getchecksum.exe             Windows command line application
    getchecksum.cpp             Application source
    getchecksum.ide             Borland C++ 5.02 IDE
    
    IDE Defines:                (none)                                
    
## 4.0 Known problems and other release notes

### 4.1 Known problems  
  1.    Implemented standard in the provided source is the January 30, 2008 version.  As of March 20, 2009 the C12.22 standard will be published this month.  The published version has a number of differences from the January 30, 2008 version.  We needed the standard to be finished before updating the submission to support it.
        
  2.    Stack does not handle a renegotiate response (0x09).
    
  3.    Provided code for local port routing has not had significant test.
  
  4.    Short message support is ifdefed out and untested.
  
### 4.2 Other release notes
  1.    Timing for the stack is provided by calls to C1222Misc_IsrIncrementFreeRunningTime which pass the elapsed time in milliseconds.  It should be called frequently - like every 1, 2, 5, 10, 100 ms or so. It does not need to be called at a consistent frequency.
  2.    The stack is single threaded.  Messages are sent by calling C1222Stack_SendMessage. C1222Stack_Run should be called frequently.  After running the stack check for message reception using C1222Stack_IsMessageAvailable. If a message is available call C1222Stack_GetReceivedMessage.  When done using a message, notify the source stack using C1222Stack_NoteReceivedMessageHandlingIsComplete or C1222Stack_NoteReceivedMessageHandlingFailed.
  3.    Look at C1222TEST for an example of how to use the stack. 
  4.    C12.22 device handles registration (there is some code in the comm module source to handle registration).  The device notifies the comm module that it should not perform registration in two ways.  First in the get config response, the "auto-registration" bit will always be clear.  Secondly, whenever the transport layer interface status indicates that auto-registration is on, it sends a transport layer link control request to the comm module to disable auto-registration.
  5.    Before the device becomes C12.22 registered, its transport configuration has a zero length node aptitle (0x0d 0x00).
  6.    When the device becomes C12.22 registered, it notifies the comm module(CM) to get its configuration using a link control request.  The configuration provided will include the registered aptitle of the meter in the node aptitle field.  This is the mechanism used to let the CM know the device's aptitle so it can know its own aptitle.
  7.    When the device receives a C12.22 deregister request/response (depending on the originator or the request being the a local procedure 24 or a deregister request received from the LAN) it returns its aptitle to zero length (0x0d 0x00), sets its registered state to false, and restarts the normal registration process.  The CM is also notified to get its configuration.
  8.    The native address in the transport configuration and in the transport layer send message is a 6 byte array of unsigned char (binary) instead of being a "STRING" as stated in the 1/30/2008 standard.  Latest version of the standard has changed the definition of native address as being binary.
  9.    The stack interprets several manufacturer's statistics with meaning as sent by the Itron RFLAN.  These include statistics between 2048 and 2067.  Please reserve manufacturer's statistics 2048 through 2999 for Itron use in updates to the submission.
  10.   Transport layer maximum payload is 150 bytes.  Application layer can send up to 643 byte messages, including up to 515 bytes of epsem and 128 bytes of ACSE message overhead.
  11.   The receive segmenter can handle at most 10 disjoint application message segments.  Adjacent segments are combined and count as 1.

## 5.0 Build process

The test applications can be built using the included IDE's using Borland C++ 5.02.  We would welcome
a contribution that made Visual C++ projects for the test tools.

### 5.1 IDE preprocessor symbols

The following symbols may need to be defined in order to build with non-Borland tools:

    AVOID_PORTAB_H                      Portab.h has some needed defines but if this is defined they are provided in 
                                        environment.h
										
    AVOID_SLB_H                         slb.h has some needed typedefs but if C1222TOOL is defined they will be defined in 
                                        c1222.h
										
    C1222TOOL                           Needed unless you provide an slb.h
	
    WANT_TCPIP                          Enables tcpip code in tools that need it
	
    C1222_INCLUDE_COMMMODULE            Enables comm module code in tools that need it
	
    C1222_INCLUDE_DEVICE                Enables c1222 device code in tools that need it
	
    C1222_INCLUDE_RELAY                 Enables relay code in tools that need it
	
    WANT_MAIN                           Needed in CRC and ENCRYPTION since the main is in the library routine
	
    C1222_DEVICE_ONLY                   Define if this is definitely a device and not a comm module
	
    C1222_COMM_MODULE                   Define if this is definitely a comm module and not a device
	
    Bool=Boolean                        Bool is defined in slb.h but not in c1222.h (oops)
	
    Signed32=long                       Signed32 is defined in slb.h but not in c1222.h (oops)
	
    C1222_WANT_SHORT_MESSAGES           Define if you want the untested short message support
	
    METER_CODE                          This should probably NOT be defined else will need to have a C1222HistLog routine
	
    ENABLE_LOCAL_PORT_ROUTER            Define if you want untested local port router support
	
    ENABLE_C1222_OPTICAL_PORT           Define if you want untested optical port support
	
    __BCPLUSPLUS__                      Needed to get linkage right if compiled in C++
	
    BORLAND_C                           May be needed to get the right header in c1222localportrouter.c
	
    C1222_TABLESERVER_BIGENDIAN         This effects byte order of time response in table 52.

## 6.0 Where's the help file

Sorry, this is it.  Or contact c1222@itron.com and/or visit C1222.net.
