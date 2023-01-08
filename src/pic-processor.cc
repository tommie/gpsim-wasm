/*
   Copyright (C) 1998 T. Scott Dattalo

This file is part of the libgpsim library of gpsim

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, see
<http://www.gnu.org/licenses/lgpl-2.1.html>.
*/

#include <stdio.h>
#ifdef _WIN32
#include "uxtime.h"
#include "unistd.h"
#else
#include <unistd.h>
#endif
#ifndef _MSC_VER
#include <sys/time.h>
#endif
#include <time.h>

#include <algorithm>
#include <iostream>
#include <list>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

#include "../config.h"
#include "14bit-registers.h"
#include "breakpoints.h"
#include "clock_phase.h"
#include "eeprom.h"
#include "exports.h"
#include "pic-processor.h"
#include "pic-registers.h"
#include "gpsim_interface.h"
#include "gpsim_time.h"
#include "interface.h"
#include "ioports.h"
#include "modules.h"
#include "p16x5x.h"
#include "p16f62x.h"
#include "p16f8x.h"
#include "p16f88x.h"
#include "p16x8x.h"
#include "p16f87x.h"
#include "p16x6x.h"
#include "p16x7x.h"
#include "p16f91x.h"
#include "p12x.h"
#include "p12f6xx.h"
#include "p1xf1xxx.h"
#ifdef P17C7XX  // code no longer works
#include "p17c75x.h"
#endif
#include "p18x.h"
#include "p18fk.h"
#include "icd.h"
#include "packages.h"
#include "pic-instructions.h"
#include "pic-ioports.h"
#include "stimuli.h"
#include "trace.h"
#include "ui.h"
#include "wdt.h"

#define DEBUG 2
// debug window WDT
#if DEBUG & 1
#define WINprintf(arg) {printf("%s:%d %s ",__FILE__,__LINE__, __FUNCTION__); printf arg; }
#else
#define WINprintf(arg) {}
#endif


uint64_t simulation_start_cycle;

//================================================================================
//
// pic_processor
//
// This file contains all (most?) of the code that simulates those features
// common to all pic microcontrollers.
//
//

ProcessorConstructor pP10F200(P10F200::construct,
                              "__10F200",   "pic10f200",    "p10f200",  "10f200");
ProcessorConstructor pP10F202(P10F202::construct,
                              "__10F202",   "pic10f202",    "p10f202",  "10f202");
ProcessorConstructor pP10F204(P10F204::construct,
                              "__10F204",   "pic10f204",    "p10f204",  "10f204");
ProcessorConstructor pP10F220(P10F220::construct,
                              "__10F220",   "pic10f220",    "p10f220",  "10f220");
ProcessorConstructor pP10F222(P10F222::construct,
                              "__10F222",   "pic10f222",    "p10f222",  "10f222");
ProcessorConstructor pP10F320(P10F320::construct,
                              "__10F320",   "pic10f320",    "p10f320",  "10f320");
ProcessorConstructor pP10LF320(P10LF320::construct,
                               "__10LF320",   "pic10lf320",    "p10lf320",  "10lf320");
ProcessorConstructor pP10F322(P10F322::construct,
                              "__10F322",   "pic10f322",    "p10f322",  "10f322");
ProcessorConstructor pP10LF322(P10LF322::construct,
                               "__10LF322",   "pic10lf322",    "p10lf322",  "10lf322");
ProcessorConstructor pP12C508(P12C508::construct,
                              "__12C508",   "pic12c508",    "p12c508",  "12c508");
ProcessorConstructor pP12C509(P12C509::construct,
                              "__12C509",   "pic12c509",    "p12c509",  "12c509");
ProcessorConstructor pP12CE518(P12CE518::construct,
                               "__12ce518",  "pic12ce518",   "p12ce518", "12ce518");
ProcessorConstructor pP12CE519(P12CE519::construct,
                               "__12ce519",  "pic12ce519",   "p12ce519", "12ce519");
ProcessorConstructor pP12F508(P12F508::construct,
                              "__12F508",   "pic12f508",    "p12f508",  "12f508");
ProcessorConstructor pP12F509(P12F509::construct,
                              "__12F509",   "pic12f509",    "p12f509",  "12f509");
ProcessorConstructor pP12F510(P12F510::construct,
                              "__12F510",   "pic12f510",    "p12f510",  "12f510");
ProcessorConstructor pP12F629(P12F629::construct,
                              "__12F629",   "pic12f629",    "p12f629",  "12f629");
ProcessorConstructor pP12F675(P12F675::construct,
                              "__12F675",   "pic12f675",    "p12f675",  "12f675");
ProcessorConstructor pP12F683(P12F683::construct,
                              "__12F683",   "pic12f683",    "p12f683",  "12f683");
ProcessorConstructor pP12F1822(P12F1822::construct,
                               "__12F1822", "pic12f1822", "p12f1822", "12f1822");
ProcessorConstructor pP12LF1822(P12LF1822::construct,
                                "__12LF1822", "pic12lf1822", "p12lf1822", "12lf1822");
ProcessorConstructor pP12F1840(P12F1840::construct,
                               "__12F1840", "pic12f1840", "p12f1840", "12f1840");
ProcessorConstructor pP12LF1840(P12LF1840::construct,
                                "__12LF1840", "pic12lf1840", "p12lf1840", "12lf1840");
ProcessorConstructor pP16C54(P16C54::construct,
                             "__16C54",     "pic16c54",     "p16c54",   "16c54");
ProcessorConstructor pP16C55(P16C55::construct,
                             "__16C55",     "pic16c55",     "p16c55",   "16c55");
ProcessorConstructor pP16C56(P16C56::construct,
                             "__16C56",     "pic16c56",     "p16c56",   "16c56");
ProcessorConstructor pP16C61(P16C61::construct,
                             "__16C61",     "pic16c61",     "p16c61",   "16c61");
ProcessorConstructor pP16C62(P16C62::construct,
                             "__16C62",     "pic16c62",     "p16c62",   "16c62");
ProcessorConstructor pP16C62A(P16C62::construct,
                              "__16C62A",   "pic16c62a",    "p16c62a",  "16c62a");
ProcessorConstructor pP16CR62(P16C62::construct,
                              "__16CR62",   "pic16cr62",    "p16cr62",  "16cr62");
ProcessorConstructor pP16C63(P16C63::construct,
                             "__16C63",     "pic16c63",     "p16c63",   "16c63");
ProcessorConstructor pP16C64(P16C64::construct,
                             "__16C64",     "pic16c64",     "p16c64",   "16c64");
ProcessorConstructor pP16C65(P16C65::construct,
                             "__16C65",     "pic16c65",     "p16c65",   "16c65");
ProcessorConstructor pP16C65A(P16C65::construct,
                              "__16C65A",    "pic16c65a",    "p16c65a",  "16c65a");
ProcessorConstructor pP16C71(P16C71::construct,
                             "__16C71",     "pic16c71",     "p16c71",   "16c71");
ProcessorConstructor pP16C712(P16C712::construct,
                              "__16C712",   "pic16c712",    "p16c712",  "16c712");
ProcessorConstructor pP16C716(P16C716::construct,
                              "__16C716",   "pic16c716",    "p16c716",  "16c716");
ProcessorConstructor pP16C72(P16C72::construct,
                             "__16C72",     "pic16c72",     "p16c72",   "16c72");
ProcessorConstructor pP16C73(P16C73::construct,
                             "__16C73",     "pic16c73",     "p16c73",   "16c73");
ProcessorConstructor pP16C74(P16C74::construct,
                             "__16C74",     "pic16c74",     "p16c74",   "16c74");
ProcessorConstructor pP16C84(P16C84::construct,
                             "__16C84",     "pic16c84",     "p16c84",   "16c84");
ProcessorConstructor pP16CR83(P16CR83::construct,
                              "__16CR83",   "pic16cr83",    "p16cr83",  "16cr83");
ProcessorConstructor pP16CR84(P16CR84::construct,
                              "__16CR84",   "pic16cr84",    "p16cr84",  "16cr84");
ProcessorConstructor pP16F505(P16F505::construct,
                              "__16F505",   "pic16f505",    "p16f505",  "16f505");
ProcessorConstructor pP16F73(P16F73::construct,
                             "__16F73",     "pic16f73",     "p16f73",   "16f73");
ProcessorConstructor pP16F74(P16F74::construct,
                             "__16F74",     "pic16f74",     "p16f74",   "16f74");
ProcessorConstructor pP16F716(P16F716::construct,
                              "__16F716",   "pic16f716",    "p16f716",  "16f716");
ProcessorConstructor pP16F83(P16F83::construct,
                             "__16F83",     "pic16f83",     "p16f83",   "16f83");
ProcessorConstructor pP16F84(P16F84::construct,
                             "__16F84",     "pic16f84",     "p16f84",   "16f84");
ProcessorConstructor pP16F87(P16F87::construct,
                             "__16F87",    "pic16f87",     "p16f87",   "16f87");
ProcessorConstructor pP16F88(P16F88::construct,
                             "__16F88",    "pic16f88",     "p16f88",   "16f88");
ProcessorConstructor pP16F882(P16F882::construct,
                              "__16F882",    "pic16f882",     "p16f882",   "16f882");
ProcessorConstructor pP16F883(P16F883::construct,
                              "__16F883",    "pic16f883",     "p16f883",   "16f883");
ProcessorConstructor pP16F884(P16F884::construct,
                              "__16F884",    "pic16f884",     "p16f884",   "16f884");
ProcessorConstructor pP16F886(P16F886::construct,
                              "__16F886",    "pic16f886",     "p16f886",   "16f886");
ProcessorConstructor pP16F887(P16F887::construct,
                              "__16F887",    "pic16f887",     "p16f887",   "16f887");
ProcessorConstructor pP16F610(P16F610::construct,
                              "__16F610",   "pic16f610",    "p16f610",  "16f610");
ProcessorConstructor pP16F616(P16F616::construct,
                              "__16F616",   "pic16f616",    "p16f616",  "16f616");
ProcessorConstructor pP16F627(P16F627::construct,
                              "__16F627",   "pic16f627",    "p16f627",  "16f627");
ProcessorConstructor pP16F627A(P16F627::construct,
                               "__16F627A",  "pic16f627a",   "p16f627a", "16f627a");
ProcessorConstructor pP16F628(P16F628::construct,
                              "__16F628",   "pic16f628",    "p16f628",  "16f628");
ProcessorConstructor pP16F628A(P16F628::construct,
                               "__16F628A",  "pic16f628a",   "p16f628a", "16f628a");
ProcessorConstructor pP16F630(P16F630::construct,
                              "__16F630",   "pic16f630",    "p16f630",  "16f630");
ProcessorConstructor pP16F631(P16F631::construct,
                              "__16F631",   "pic16f631",    "p16f631",  "16f631");
ProcessorConstructor pP16F648(P16F648::construct,
                              "__16F648",   "pic16f648",    "p16f648",  "16f648");
ProcessorConstructor pP16F648A(P16F648::construct,
                               "__16F648A",  "pic16f648a",   "p16f648a", "16f648a");
ProcessorConstructor pP16F676(P16F676::construct,
                              "__16F676",   "pic16f676",    "p16f676",  "16f676");
ProcessorConstructor pP16F677(P16F677::construct,
                              "__16F677",   "pic16f677",    "p16f677",  "16f677");
ProcessorConstructor pP16F684(P16F684::construct,
                              "__16F684",   "pic16f684",    "p16f684",  "16f684");
ProcessorConstructor pP16F685(P16F685::construct,
                              "__16F685",   "pic16f685",    "p16f685",  "16f685");
ProcessorConstructor pP16F687(P16F687::construct,
                              "__16F687",   "pic16f687",    "p16f687",  "16f687");
ProcessorConstructor pP16F689(P16F689::construct,
                              "__16F689",   "pic16f689",    "p16f689",  "16f689");
ProcessorConstructor pP16F690(P16F690::construct,
                              "__16F690",   "pic16f690",    "p16f690",  "16f690");
ProcessorConstructor pP16F818(P16F818::construct,
                              "__16F818",   "pic16f818",    "p16f818",  "16f818");
ProcessorConstructor pP16F819(P16F819::construct,
                              "__16F819",   "pic16f819",    "p16f819",  "16f819");
ProcessorConstructor pP16F871(P16F871::construct,
                              "__16F871",   "pic16f871",    "p16f871",  "16f871");
ProcessorConstructor pP16F873(P16F873::construct,
                              "__16F873",   "pic16f873",    "p16f873",  "16f873");
ProcessorConstructor pP16F874(P16F874::construct,
                              "__16F874",   "pic16f874",    "p16f874",  "16f874");
ProcessorConstructor pP16F876(P16F876::construct,
                              "__16F876",   "pic16f876",    "p16f876",  "16f876");
ProcessorConstructor pP16F877(P16F877::construct,
                              "__16F877",   "pic16f877",    "p16f877",  "16f877");
ProcessorConstructor pP16F873A(P16F873A::construct,
                               "__16F873a",  "pic16f873a",   "p16f873a", "16f873a");
ProcessorConstructor pP16F874A(P16F874A::construct,
                               "__16F874a",  "pic16f874a",   "p16f874a", "16f874a");
ProcessorConstructor pP16F876A(P16F876A::construct,
                               "__16F876a",  "pic16f876a",   "p16f876a", "16f876a");
ProcessorConstructor pP16F877A(P16F877A::construct,
                               "__16F877a",  "pic16f877a",   "p16f877a", "16f877a");
ProcessorConstructor pP16F913(P16F913::construct,
                              "__16F913",  "pic16f913",   "p16f913", "16f913");
ProcessorConstructor pP16F914(P16F914::construct,
                              "__16F914",  "pic16f914",   "p16f914", "16f914");
ProcessorConstructor pP16F916(P16F916::construct,
                              "__16F916",  "pic16f916",   "p16f916", "16f916");
ProcessorConstructor pP16F917(P16F917::construct,
                              "__16F917",  "pic16f917",   "p16f917", "16f917");
ProcessorConstructor pP16F1503(P16F1503::construct,
                         "__16F1503", "pic16f1503", "p16f1503", "16f1503");
ProcessorConstructor pP16LF1503(P16LF1503::construct,
                         "__16LF1503", "pic16lf1503", "p16lf1503", "16lf1503");
ProcessorConstructor pP16F1705(P16F1705::construct,
                         "__16F1705", "pic16f1705", "p16f1705", "16f1705");
ProcessorConstructor pP16LF1705(P16LF1705::construct,
                         "__16LF1705", "pic16lf1705", "p16lf1705", "16lf1705");
ProcessorConstructor pP16F1709(P16F1709::construct,
                         "__16F1709", "pic16f1709", "p16f1709", "16f1709");
ProcessorConstructor pP16LF1709(P16LF1709::construct,
                         "__16LF1709", "pic16lf1709", "p16lf1709", "16lf1709");
ProcessorConstructor pP16F1788(P16F1788::construct,
                         "__16F1788", "pic16f1788", "p16f1788", "16f1788");
ProcessorConstructor pP16LF1788(P16LF1788::construct,
                         "__16LF1788", "pic16lf1788", "p16lf1788", "16lf1788");
ProcessorConstructor pP16F1823(P16F1823::construct,
                          "__16F1823", "pic16f1823", "p16f1823", "16f1823");
ProcessorConstructor pP16LF1823(P16LF1823::construct,
                          "__16LF1823", "pic16lf1823", "p16lf1823", "16lf1823");
ProcessorConstructor pP16F1825(P16F1825::construct,
                          "__16F1825", "pic16f1825", "p16f1825", "16f1825");
ProcessorConstructor pP16LF1825(P16F1825::construct,
                          "__16LF1825", "pic16lf1825", "p16lf1825", "16lf1825");
#ifdef P17C7XX  // code no longer works
ProcessorConstructor pP17C7xx(P17C7xx::construct,
                              "__17C7xx", "pic17c7xx",  "p17c7xx", "17c7xx");
ProcessorConstructor pP17C75x(P17C75x::construct,
                              "__17C75x", "pic17c75x",  "p17c75x", "17c75x");
ProcessorConstructor pP17C752(P17C752::construct,
                              "__17C752", "pic17c752",  "p17c752", "17c752");
ProcessorConstructor pP17C756(P17C756::construct,
                              "__17C756", "pic17c756",  "p17c756", "17c756");
ProcessorConstructor pP17C756A(P17C756A::construct,
                               "__17C756A", "pic17c756a",   "p17c756a", "17c756a");
ProcessorConstructor pP17C762(P17C762::construct,
                              "__17C762", "pic17c762",  "p17c762", "17c762");
ProcessorConstructor pP17C766(P17C766::construct,
                              "__17C766", "pic17c766",  "p17c766", "17c766");
#endif // P17C7XX
ProcessorConstructor pP18C242(P18C242::construct,
                              "__18C242",   "pic18c242",    "p18c242",  "18c242");
ProcessorConstructor pP18C252(P18C252::construct,
                              "__18C252",   "pic18c252",    "p18c252",  "18c252");
ProcessorConstructor pP18C442(P18C442::construct,
                              "__18C442",   "pic18c442",    "p18c442",  "18c442");
ProcessorConstructor pP18C452(P18C452::construct,
                              "__18C452",   "pic18c452",    "p18c452",  "18c452");
ProcessorConstructor pP18F242(P18F242::construct,
                              "__18F242",   "pic18f242",    "p18f242",  "18f242");
ProcessorConstructor pP18F248(P18F248::construct,
                              "__18F248",   "pic18f248",    "p18f248",  "18f248");
ProcessorConstructor pP18F258(P18F258::construct,
                              "__18F258",   "pic18f258",    "p18f258",  "18f258");
ProcessorConstructor pP18F252(P18F252::construct,
                              "__18F252",   "pic18f252",    "p18f252",  "18f252");
ProcessorConstructor pP18F442(P18F442::construct,
                              "__18F442",   "pic18f442",    "p18f442",  "18f442");
ProcessorConstructor pP18F448(P18F448::construct,
                              "__18F448",   "pic18f448",    "p18f448",  "18f448");
ProcessorConstructor pP18F458(P18F458::construct,
                              "__18F458",   "pic18f458",    "p18f458",  "18f458");
ProcessorConstructor pP18F452(P18F452::construct,
                              "__18F452",   "pic18f452",    "p18f452",  "18f452");
ProcessorConstructor pP18F1220(P18F1220::construct,
                               "__18F1220",  "pic18f1220",   "p18f1220", "18f1220");
ProcessorConstructor pP18F1320(P18F1320::construct,
                               "__18F1320",  "pic18f1320",   "p18f1320", "18f1320");
ProcessorConstructor pP18F14K22(P18F14K22::construct,
                                "__18F14K22", "pic18f14k22",  "p18f14k22", "18f14k22");
ProcessorConstructor pP18F2221(P18F2221::construct,
                               "__18F2221",  "pic18f2221",   "p18f2221", "18f2221");
ProcessorConstructor pP18F2321(P18F2321::construct,
                               "__18F2321",  "pic18f2321",   "p18f2321", "18f2321");
ProcessorConstructor pP18F2420(P18F2420::construct,
                               "__18F2420",  "pic18f2420",   "p18f2420", "18f2420");
ProcessorConstructor pP18F2455(P18F2455::construct,
                               "__18F2455",  "pic18f2455",   "p18f2455", "18f2455");
ProcessorConstructor pP18F2520(P18F2520::construct,
                               "__18F2520",  "pic18f2520",   "p18f2520", "18f2520");
ProcessorConstructor pP18F2525(P18F2525::construct,
                               "__18F2525",  "pic18f2525",   "p18f2525", "18f2525");
ProcessorConstructor pP18F2550(P18F2550::construct,
                               "__18F2550",  "pic18f2550",   "p18f2550", "18f2550");
ProcessorConstructor pP18F2620(P18F2620::construct,
                               "__18F2620",  "pic18f2620",   "p18f2620", "18f2620");
ProcessorConstructor pP18F26K22(P18F26K22::construct,
                                "__18F26K22", "pic18f26k22",  "p18f26k22", "18f26k22");
ProcessorConstructor pP18F4221(P18F4221::construct,
                               "__18F4221",  "pic18f4221",   "p18f4221", "18f4221");
ProcessorConstructor pP18F4321(P18F4321::construct,
                               "__18F4321",  "pic18f4321",   "p18f4321", "18f4321");
ProcessorConstructor pP18F4420(P18F4420::construct,
                               "__18F4420",  "pic18f4420",   "p18f4420", "18f4420");
ProcessorConstructor pP18F4520(P18F4520::construct,
                               "__18F4520",  "pic18f4520",   "p18f4520", "18f4520");
ProcessorConstructor pP18F4550(P18F4550::construct,
                               "__18F4550",  "pic18f4550",   "p18f4550", "18f4550");
ProcessorConstructor pP18F4455(P18F4455::construct,
                               "__18F4455",  "pic18f4455",   "p18f4455", "18f4455");
ProcessorConstructor pP18F4620(P18F4620::construct,
                               "__18F4620",  "pic18f4620",   "p18f4620", "18f4620");
ProcessorConstructor pP18F6520(P18F6520::construct,
                               "__18F6520",  "pic18f6520",   "p18f6520", "18f6520");


//========================================================================
// Trace Type for Resets

class InterruptTraceObject : public ProcessorTraceObject
{
public:
    explicit InterruptTraceObject(Processor *_cpu);
    void print(FILE *fp) override;
};


class InterruptTraceType : public ProcessorTraceType
{
public:
    explicit InterruptTraceType(Processor *_cpu);
    TraceObject *decode(unsigned int tbi) override;
    void record();
    int dump_raw(Trace *pTrace, unsigned int tbi, char *buf, int bufsize) override;

    unsigned int m_uiTT;
};


//------------------------------------------------------------
InterruptTraceObject::InterruptTraceObject(Processor *_cpu)
    : ProcessorTraceObject(_cpu)
{
}


void InterruptTraceObject::print(FILE *fp)
{
    fprintf(fp, "  %s *** Interrupt ***\n",
            (cpu ? cpu->name().c_str() : ""));
}


//------------------------------------------------------------
InterruptTraceType::InterruptTraceType(Processor *_cpu)
    : ProcessorTraceType(_cpu, 1, "Interrupt")
{
    m_uiTT = trace.allocateTraceType(this);
}


TraceObject *InterruptTraceType::decode(unsigned int /* tbi */ )
{
    //unsigned int tv = trace.get(tbi);
    return new InterruptTraceObject(cpu);
}


void InterruptTraceType::record()
{
    trace.raw(m_uiTT);
}


int InterruptTraceType::dump_raw(Trace *pTrace, unsigned int tbi, char *buf, int bufsize)
{
    if (!pTrace)
    {
        return 0;
    }

    int n = TraceType::dump_raw(pTrace, tbi, buf, bufsize);
    buf += n;
    bufsize -= n;
    int m = snprintf(buf, bufsize,
                     " %s *** Interrupt ***",
                     (cpu ? cpu->name().c_str() : ""));
    return m > 0 ? (m + n) : n;
}


//-------------------------------------------------------------------
void pic_processor::set_eeprom(EEPROM *e)
{
    eeprom = e;

    if (e)
    {
        ema.set_Registers(e->rom, e->rom_size);
    }
}


//-------------------------------------------------------------------
void pic_processor::BP_set_interrupt()
{
    m_pInterruptTT->record();
    mCaptureInterrupt->firstHalf();
}


//-------------------------------------------------------------------
//
// sleep - Begin sleeping and stay asleep until something causes a wake
//

void pic_processor::sleep()
{
}


//-------------------------------------------------------------------
//
// enter_sleep - The processor is about to go to sleep, so update
//  the status register.

void pic_processor::enter_sleep()
{
    status->put_TO(1);
    status->put_PD(0);
    sleep_time = get_cycles().get();
    wdt->update();
    pc->increment();
    save_pNextPhase = mCurrentPhase->getNextPhase();
    save_CurrentPhase = mCurrentPhase;
    mCurrentPhase->setNextPhase(mIdle);
    mCurrentPhase = mIdle;
    mCurrentPhase->setNextPhase(mIdle);
    m_ActivityState = ePASleeping;
}


//-------------------------------------------------------------------
//
// exit_sleep

void pic_processor::exit_sleep()
{
    // If enter and exit sleep at same clock cycle, restore execute state
    if (get_cycles().get() == sleep_time)
    {
        mCurrentPhase = save_CurrentPhase;
        mCurrentPhase->setNextPhase(save_pNextPhase);

    }
    else
    {
        mCurrentPhase->setNextPhase(mExecute1Cycle);
    }

    m_ActivityState = ePAActive;
}


//-------------------------------------------------------------------
//
// inattentive - In some situations, the processor will execute a few
// instructions as NOPs. For example when it is reading flash memory.

void pic_processor::inattentive(unsigned int count) {
    mSkip->arm(count);
    mCurrentPhase->setNextPhase(mSkip);
    mCurrentPhase = mSkip;
}


//-------------------------------------------------------------------
//
// is_sleeping

bool pic_processor::is_sleeping()
{
    return m_ActivityState == ePASleeping;
}


//-------------------------------------------------------------------
//
// pm_write - program memory write
//

void pic_processor::pm_write()
{
    m_ActivityState = ePAPMWrite;

    do
    {
        get_cycles().increment();  // burn cycles until we're through writing
    }
    while (bp.have_pm_write());

    simulation_mode = eSM_RUNNING;
}


static bool realtime_mode = false;
static bool realtime_mode_with_gui = false;


void EnableRealTimeMode(bool bEnable)
{
    realtime_mode = bEnable;
}


void EnableRealTimeModeWithGui(bool bEnable)
{
    realtime_mode_with_gui = bEnable;
}


extern void update_gui();

class RealTimeBreakPoint : public TriggerObject
{
public:
    Processor *cpu;
    struct timeval tv_start;
    uint64_t cycle_start;
    uint64_t future_cycle;
    int warntimer;
    uint64_t period;            // callback period in us

    //#define REALTIME_DEBUG
    uint64_t diffmax;
    uint64_t diffsum;
    int diffsumct;
    struct timeval stat_start;

    RealTimeBreakPoint()
        : cpu(nullptr), future_cycle(0), warntimer(1), period(1),
          diffmax(0), diffsum(0), diffsumct(0)
    {
    }

    void start(Processor *active_cpu)
    {
        if (!active_cpu)
        {
            return;
        }

        diffsum = 0;
        diffsumct = 0;
        diffmax = 0;
        // Grab the system time and record the simulated pic's time.
        // We'll then set a break point a short time in the future
        // and compare how the two track.
        cpu = active_cpu;
        gettimeofday(&tv_start, 0);
        stat_start = tv_start;
        cycle_start = get_cycles().get();
        uint64_t fc = cycle_start + 100;

        //cout << "real time start : " << cycle_start << '\n';

        if (future_cycle)
        {
            get_cycles().reassign_break(future_cycle, fc, this);

        }
        else
        {
            get_cycles().set_break(fc, this);
        }

        future_cycle = fc;
    }

    void stop()
    {
        //cout << "real time stop : " << future_cycle << '\n';
#ifdef REALTIME_DEBUG
        dump_stats();
#endif

        // Clear any pending break point.
        if (future_cycle)
        {
            std::cout << " real time clearing\n";
            get_cycles().clear_break(this);
            future_cycle = 0;

            if (realtime_mode_with_gui)
            {
                update_gui();
            }
        }
    }

#ifdef REALTIME_DEBUG
    void dump_stats()
    {
        struct timeval tv;
        gettimeofday(&tv, 0);
        double simulation_time = (tv.tv_sec - stat_start.tv_sec) + (tv.tv_usec - stat_start.tv_usec) / 1000000.0; // in seconds

        if (diffsumct > 0 && simulation_time > 0)
        {
            std::cout << std::dec << "Average realtime error: " << diffsum / diffsumct << " microseconds. Max: " << diffmax << '\n';

            if (realtime_mode_with_gui)
            {
                std::cout << "Number of realtime callbacks (gui refreshes) per second:";

            }
            else
            {
                std::cout << "Number of realtime callbacks per second:";
            }

            std::cout << diffsumct / (double)simulation_time << '\n';
        }

        stat_start = tv;
        diffsum = 0;
        diffsumct = 0;
        diffmax = 0;
    }
#endif

    void callback() override
    {
        uint64_t system_time;	// wall clock time since datum in micro seconds
        uint64_t simulation_time;	// simulation time since datum in micro seconds
        uint64_t diff_us;
        struct timeval tv;
        // We just hit the break point. A few moments ago we
        // grabbed a snap shot of the system time and the simulated
        // pic's time. Now we're going to compare the two deltas and
        // see how well they've tracked.
        //
        // If the host is running faster than the PIC, we'll put the
        // host to sleep briefly.
        //
        // If the host is running slower than the PIC, lengthen the
        // time between GUI updates.
        gettimeofday(&tv, 0);
        system_time = (tv.tv_sec - tv_start.tv_sec) * 1000000ULL + (tv.tv_usec - tv_start.tv_usec); // in micro-seconds
        simulation_time = ((get_cycles().get() - cycle_start) * 4.0e6 * cpu->get_OSCperiod());

        if (simulation_time > system_time)
        {
            // we are simulating too fast
            diff_us = simulation_time - system_time;

            if (period > diff_us)
            {
                period -= diff_us;

            }
            else
            {
                period = 1;
            }

            usleep((unsigned int)diff_us);

        }
        else
        {
            diff_us = system_time - simulation_time;
            period += diff_us;

            if (period > 1000000)
            {
                period = 1000000;  // limit to a one second callback period
            }

            if (diff_us > 1000000)
            {
                // we are simulating too slow
                if (warntimer < 10)
                {
                    warntimer++;

                }
                else
                {
                    warntimer = 0;
                    puts("Processor is too slow for realtime mode!");
                }

            }
            else
            {
                warntimer = 0;
            }
        }

        uint64_t delta_cycles = (uint64_t)(period * cpu->get_frequency() / 4000000);

        if (delta_cycles < 1)
        {
            delta_cycles = 1;
        }

        // Look at realtime_mode_with_gui and update the gui if true
        if (realtime_mode_with_gui)
        {
            update_gui();
        }

#ifdef REALTIME_DEBUG

        if (tv.tv_sec < stat_start.tv_sec + 10)
        {
            diffsum += diff_us;
            diffsumct++;

        }
        else
        {
            dump_stats();
        }

        if (diff_us > diffmax)
        {
            diffmax = diff_us;
        }

        static uint64_t oldtime = 0;
        //cout<<dec<<"dt="<<(system_time-oldtime)/1000 << "\tdiff_us="<<diff_us<<"\tdelta_cycles="<<delta_cycles<<"\tperiod="<<period<<endl;
        oldtime = system_time;
#endif
        uint64_t fc = get_cycles().get() + delta_cycles;

        if (future_cycle)
        {
            get_cycles().reassign_break(future_cycle, fc, this);

        }
        else
        {
            get_cycles().set_break(fc, this);
        }

        future_cycle = fc;
    }

};


RealTimeBreakPoint realtime_cbp;

//-------------------------------------------------------------------
void pic_processor::save_state()
{
    Processor::save_state();

    if (Wreg)
    {
        Wreg->put_trace_state(Wreg->value);
    }

    if (eeprom)
    {
        eeprom->save_state();
    }
}


//-------------------------------------------------------------------
//
// run  -- Begin simulating and don't stop until there is a break.
//


void pic_processor::run(bool /* refresh */ )
{
    if (simulation_mode != eSM_STOPPED)
    {
        if (verbose)
        {
            std::cout << "Ignoring run request because simulation is not stopped\n";
        }

        return;
    }

    simulation_mode = eSM_RUNNING;

    // If the first instruction we're simulating is a break point,
    // then ignore it.

    if (realtime_mode)
    {
        realtime_cbp.start(this);
    }

    simulation_start_cycle = get_cycles().get();
    bp.clear_global();
    mCurrentPhase = mCurrentPhase ? mCurrentPhase : mExecute1Cycle;

    do
    {
        mCurrentPhase = mCurrentPhase->advance();
    }
    while (!bp.global_break);

    if (realtime_mode)
    {
        realtime_cbp.stop();
    }

    bp.clear_global();
    trace.cycle_counter(get_cycles().get());
    simulation_mode = eSM_STOPPED;
}


//-------------------------------------------------------------------
//
// step - Simulate one (or more) instructions. If a breakpoint is set
// at the current PC-> 'step' will go right through it. (That's supposed
// to be a feature.)
//

void pic_processor::step(unsigned int steps, bool refresh)
{
    if (!steps)
    {
        return;
    }

    if (get_use_icd())
    {
        if (steps != 1)
        {
            std::cout << "Can only step one step in ICD mode\n";
        }

        icd_step();
        pc->get_value();
        disassemble((signed int)pc->value, (signed int)pc->value); // FIXME, don't want this in HLL ICD mode.

        if (refresh)
        {
            gi.simulation_has_stopped();
        }

        return;
    }

    if (simulation_mode != eSM_STOPPED)
    {
        if (verbose)
        {
            std::cout << "Ignoring step request because simulation is not stopped\n";
        }

        return;
    }

    simulation_mode = eSM_SINGLE_STEPPING;
    mCurrentPhase = mCurrentPhase ? mCurrentPhase : mExecute1Cycle;

    do
    {
        mCurrentPhase = mCurrentPhase->advance();
    }
    while (!bp.have_halt() && --steps > 0);

    // complete the step if this is a multi-cycle instruction.

    if (mCurrentPhase == mExecute2ndHalf)
        while (mCurrentPhase != mExecute1Cycle)
        {
            mCurrentPhase = mCurrentPhase->advance();
        }

    get_trace().cycle_counter(get_cycles().get());

    if (refresh)
    {
        trace_dump(0, 1);
    }

    bp.clear_halt();
    simulation_mode = eSM_STOPPED;

    if (refresh)
    {
        get_interface().simulation_has_stopped();
    }
}


//-------------------------------------------------------------------
void pic_processor::step_cycle()
{
    mCurrentPhase = mCurrentPhase->advance();
}

void pic_processor::step_one(bool)
{
    if (pc->value < program_memory_size())
    {
        program_memory[pc->value]->execute();
    }
    else
    {
        std::cout << "Program counter not valid " << std::hex << pc->value << '\n';
        get_bp().halt();
    }
}

//
//-------------------------------------------------------------------
//
// step_over - In most cases, step_over will simulate just one instruction.
// However, if the next instruction is a branching one (e.g. goto, call,
// return, etc.) then a break point will be set after it and gpsim will
// begin 'running'. This is useful for stepping over time-consuming calls.
//

void pic_processor::step_over(bool refresh)
{
    bool skip = false;

    if (simulation_mode != eSM_STOPPED)
    {
        if (verbose)
        {
            std::cout << "Ignoring step-over request because simulation is not stopped\n";
        }

        return;
    }

    unsigned int saved_pc = pma->get_PC();
    instruction *nextInstruction = pma->getFromAddress(saved_pc);

    if (!nextInstruction)
    {
        // this is really fatal...
        return;
    }

    // If break set, get actual instruction
    if (typeid(*nextInstruction) == typeid(Breakpoint_Instruction))
    {
        Breakpoint_Instruction *x = (Breakpoint_Instruction *)nextInstruction;
        nextInstruction = x->getReplaced();
    }
    // skip over calls of various types
    if (nextInstruction->name() == "call" ||
            nextInstruction->name() == "rcall" ||
            nextInstruction->name() == "callw"
       )
    {
        skip = true;
    }

    unsigned int nextExpected_pc =
        saved_pc + map_pm_index2address(nextInstruction->instruction_size());
    step(1, false); // Try one step -- without refresh
    // if the pc did not advance just one instruction, then some kind of branch occurred.
    unsigned int current_pc = pma->get_PC();

    if (skip && !(current_pc >= saved_pc && current_pc <= nextExpected_pc))
    {
        // If the branch is not a skip instruction then we'll set a break point and run.
        // (note, the test that's performed will treat a goto $+2 as a skip.
        instruction *nextNextInstruction = pma->getFromAddress(nextExpected_pc);
        unsigned int nextNextExpected_pc = nextExpected_pc +
                                           (nextNextInstruction ? map_pm_index2address(nextNextInstruction->instruction_size()) : 0);

        if (!(current_pc >= saved_pc && current_pc <= nextNextExpected_pc))
        {
            unsigned int bp_num = pma->set_break_at_address(nextExpected_pc);

            if (bp_num != INVALID_VALUE)
            {
                run();
                bp.clear(bp_num);
            }
        }
    }

    // note that we don't need to tell the gui to update its windows since
    // that is already done by step() or run().

    if (refresh)
    {
        get_interface().simulation_has_stopped();
    }
}


//-------------------------------------------------------------------
//
// finish
//
// this method really only applies to processors with stacks.

void pic_processor::finish()
{
    if (!stack)
    {
        return;
    }

    run_to_address(stack->contents[(stack->pointer - 1) & stack->stack_mask]);
    get_interface().simulation_has_stopped();
}


//-------------------------------------------------------------------
//
// reset - reset the pic based on the desired reset type.
//

void pic_processor::reset(RESET_TYPE r)
{
    bool bHaltSimulation = getBreakOnReset();

    if (get_use_icd())
    {
        puts("RESET");
        icd_reset();
        disassemble((signed int)pc->get_value(), (signed int)pc->get_value());
        gi.simulation_has_stopped();
        return;
    }

    m_pResetTT->record(r);
    rma.reset(r);
    stack->reset(r);
    wdt->reset(r);
    pc->reset();
    bp.clear_global();

    switch (r)
    {
    case POR_RESET:
        if (verbose)
        {
            std::cout << "POR\n";

            if (config_modes)
            {
                config_modes->print();
            }
        }

        bHaltSimulation = false;
        mCurrentPhase = mCurrentPhase ? mCurrentPhase : mExecute1Cycle;
        m_ActivityState = ePAActive;
        break;

    case SOFT_RESET:
        std::cout << "Reset due to Software reset instruction\n";
        mCurrentPhase = mExecute1Cycle;
        mCurrentPhase->setNextPhase(mExecute1Cycle);
        m_ActivityState = ePAActive;
        break;

    case MCLR_RESET:
        std::cout << "MCLR reset\n";
        mCurrentPhase = mIdle;
        mCurrentPhase->setNextPhase(mIdle);
        m_ActivityState = ePAIdle;
        break;

    case IO_RESET:
        mCurrentPhase = mExecute1Cycle;
        mCurrentPhase->setNextPhase(mExecute1Cycle);
        m_ActivityState = ePAActive;
        break;

    case WDT_RESET:
        std::cout << "Reset on Watch Dog Timer expire\n";
        mCurrentPhase = mCurrentPhase ? mCurrentPhase : mExecute1Cycle;
        mCurrentPhase->setNextPhase(mExecute1Cycle);
        m_ActivityState = ePAActive;
        break;

    case WDTWV_RESET:
        std::cout << "Reset on Watch Dog Timer window violation\n";
        mCurrentPhase = mCurrentPhase ? mCurrentPhase : mExecute1Cycle;
        mCurrentPhase->setNextPhase(mExecute1Cycle);
        m_ActivityState = ePAActive;
        break;

    case EXIT_RESET:	// MCLR reset has cleared
        std::cout << "MCLR low, resume execution\n";
        mCurrentPhase = mCurrentPhase ? mCurrentPhase : mExecute1Cycle;
        mCurrentPhase->setNextPhase(mExecute1Cycle);
        m_ActivityState = ePAActive;
        return;
        break;

    case STKOVF_RESET:
        std::cout << "Reset on Stack overflow\n";
        mCurrentPhase = mCurrentPhase ? mCurrentPhase : mIdle;
        mCurrentPhase->setNextPhase(mIdle);
        m_ActivityState = ePAActive;
        //  mCurrentPhase->setNextPhase(mExecute1Cycle);
        //  m_ActivityState = ePAActive;
        break;

    case STKUNF_RESET:
        std::cout << "Reset on Stack undeflow\n";
        mCurrentPhase = mCurrentPhase ? mCurrentPhase : mIdle;
        mCurrentPhase->setNextPhase(mIdle);
        m_ActivityState = ePAActive;
        break;

    default:
        printf("pic_processor::reset unknow reset type %d\n", r);
        m_ActivityState = ePAActive;
        break;
    }

    if (bHaltSimulation || getBreakOnReset())
    {
        bp.halt();
        gi.simulation_has_stopped();
    }
}


//-------------------------------------------------------------------
//
// pic_processor -- constructor
//

pic_processor::pic_processor(const char *_name, const char *_desc)
    : Processor(_name, _desc),
      tmr0(this, "tmr0", "Timer 0")
{
    wdt = new WDT(this, 18.0e-3);
    mExecute1Cycle    = new phaseExecute1Cycle(this);
    mExecute2ndHalf   = new phaseExecute2ndHalf(this);
    mCaptureInterrupt = new phaseCaptureInterrupt(this);
    mIdle             = new phaseIdle(this);
    mSkip             = new phaseSkip(this);
    mCurrentPhase   = mExecute1Cycle;
    std::fill_n(m_osc_Monitor, 4, nullptr);
    m_Capabilities = eSTACK | eWATCHDOGTIMER;

    if (verbose)
    {
        std::cout << "pic_processor constructor\n";
    }

    config_modes = create_ConfigMode();
    Integer::setDefaultBitmask(0xff);
    // Test code for logging to disk:
    GetTraceLog().switch_cpus(this);
    m_pResetTT = new ResetTraceType(this);
    m_pInterruptTT = new InterruptTraceType(this);

    for (int i = 0; i < 4; i++)
    {
        osc_pin_Number[i] = 254;
    }
}


//-------------------------------------------------------------------
pic_processor::~pic_processor()
{
    if (pma)
    {
        while (!rma.SpecialRegisters.empty())
        {
            rma.SpecialRegisters.pop_back();
        }

        while (!pma->SpecialRegisters.empty())
        {
            pma->SpecialRegisters.pop_back();
        }
    }

    delete wdt;
    delete m_pResetTT;
    delete m_pInterruptTT;
    delete_sfr_register(Wreg);
    delete_sfr_register(pcl);
    delete_sfr_register(pclath);
    delete_sfr_register(status);
    delete_sfr_register(indf);
    delete m_PCHelper;
    delete stack;
    delete mExecute1Cycle;
    delete mExecute2ndHalf;
    delete mCaptureInterrupt;
    delete mIdle;
    delete config_modes;
    delete m_configMemory;

    if (m_MCLR)
    {
        m_MCLR->setMonitor(nullptr);
    }

    if (m_MCLR_Save)
    {
        m_MCLR_Save->setMonitor(nullptr);
    }

    delete m_MCLRMonitor;
    delete clksource;
    delete clkcontrol;
}


//-------------------------------------------------------------------
//
//
//    create
//
//  The purpose of this member function is to 'create' a pic processor.
// Since this is a base class member function, only those things that
// are common to all pics are created.

void pic_processor::create()
{
    init_program_memory(program_memory_size());
    init_register_memory(register_memory_size());
    // Now, initialize the core stuff:
    pc->set_cpu(this);
    Wreg = new WREG(this, "W", "Working Register");
    pcl = new PCL(this, "pcl", "Program Counter Low");
    pclath = new PCLATH(this, "pclath", "Program Counter Latch High");
    status = new Status_register(this, "status", "Processor status");
    indf = new INDF(this, "indf", "Indirect register");
    register_bank = &registers[0];  // Define the active register bank

    if (pma)
    {
        m_PCHelper = new PCHelper(this, pma);
        rma.SpecialRegisters.push_back(m_PCHelper);
        rma.SpecialRegisters.push_back(status);
        rma.SpecialRegisters.push_back(Wreg);
        pma->SpecialRegisters.push_back(m_PCHelper);
        pma->SpecialRegisters.push_back(status);
        pma->SpecialRegisters.push_back(Wreg);
    }

    create_config_memory();
}


//-------------------------------------------------------------------
//
// add_sfr_register
//
// The purpose of this routine is to add one special function register
// to the file registers. If the sfr has a physical address (like the
// status or tmr0 registers) then a pointer to that register will be
// placed in the file register map.

// FIXME It doesn't make any sense to initialize the por_value here!
// FIXME The preferred
// FIXME parent's constructor.

void pic_processor::add_sfr_register(Register *reg, unsigned int addr,
                                     RegisterValue por_value,
                                     const char *new_name,
                                     bool warn_dup)
{
    reg->set_cpu(this);

    if (addr < register_memory_size())
    {
        if (registers[addr])
        {
            if (registers[addr]->isa() == Register::INVALID_REGISTER)
            {
                delete registers[addr];
                registers[addr] = reg;

            }
            else if (warn_dup)
            {
                printf("%s %s 0x%x Already register %s\n", __FUNCTION__, name().c_str(), addr, registers[addr]->name().c_str());
            }

        }
        else
        {
            registers[addr] = reg;
        }

        reg->address = addr;
        reg->alias_mask = 0;

        if (new_name)
        {
            reg->new_name(new_name);
        }

        RegisterValue rv = getWriteTT(addr);
        reg->set_write_trace(rv);
        rv = getReadTT(addr);
        reg->set_read_trace(rv);
    }

    reg->value       = por_value;
    reg->por_value   = por_value;  /// FIXME why are we doing this?
    reg->initialize();
}


// Use this function when register is initialized on WDT reset to
// same value as a POR.
void pic_processor::add_sfr_registerR(sfr_register *reg, unsigned int addr,
                                      RegisterValue por_value,
                                      const char *new_name,
                                      bool warn_dup)
{
    add_sfr_register(reg, addr, por_value, new_name, warn_dup);
    reg->wdtr_value = por_value;
}


//-------------------------------------------------------------------
//
// delete_sfr_register
// This both deletes the register from the registers array,
// but also deletes the register class.
//
void pic_processor::delete_sfr_register(Register *pReg)
{
    if (pReg)
    {
        unsigned int a = pReg->getAddress();

        if (0)
            std::cout << __FUNCTION__ << " addr = 0x" << std::hex << a
                      << " reg " << pReg->name() << '\n';

        if (a < rma.get_size() && registers[a] == pReg)
        {
            delete_file_registers(a, a);

        }
        else
        {
            delete pReg;
	    pReg = nullptr;
        }
    }
}


//-------------------------------------------------------------------
//
// remove_sfr_register
// This is the inverse of add_sfr_register and does not delete the register.
//
void pic_processor::remove_sfr_register(Register *ppReg)
{
    if (ppReg)
    {
        unsigned int a = ppReg->getAddress();

        if (a == AN_INVALID_ADDRESS)
        {
            return;
        }

        if (registers[a] == ppReg)
        {
            delete_file_registers(a, a, true);
        }
    }
}


//-------------------------------------------------------------------
//
// init_program_memory
//
// The purpose of this member function is to allocate memory for the
// pic's code space. The 'memory_size' parameter tells how much memory
// is to be allocated AND it should be an integer of the form of 2^n.
// If the memory size is not of the form of 2^n, then this routine will
// round up to the next integer that is of the form 2^n.
//   Once the memory has been allocated, this routine will initialize
// it with the 'bad_instruction'. The bad_instruction is an instantiation
// of the instruction class that chokes gpsim if it is executed. Note that
// each processor owns its own 'bad_instruction' object.

void pic_processor::init_program_memory(unsigned int memory_size)
{
    if (verbose)
    {
        std::cout << "Initializing program memory: 0x" << memory_size << " words\n";
    }

    // The memory_size_mask is used by the branching instructions
    pc->memory_size = memory_size;
    Processor::init_program_memory(memory_size);
}


void pic_processor::create_symbols()
{
    if (verbose)
    {
        std::cout << __FUNCTION__ << " register memory size = " << register_memory_size() << '\n';
    }

    for (unsigned int i = 0; i < register_memory_size(); i++)
    {
        switch (registers[i]->isa())
        {
        case Register::SFR_REGISTER:
            //if(!symbol_table.find((char *)registers[i]->name().c_str()))
            //  symbol_table.add_register(registers[i]);
            //
            addSymbol(registers[i]);
            break;

        default:
            break;
        }
    }

    pc->set_description("Program Counter");  // Fixme put this in the pc constructor.
    addSymbol(pc);
    addSymbol(wdt);
}


//-------------------------------------------------------------------


bool pic_processor::set_config_word(unsigned int address, unsigned int cfg_word)
{
    int i = get_config_index(address);

    if (i >= 0)
    {
        m_configMemory->getConfigWord(i)->set((int)cfg_word);

        if (i == 0 && config_modes)
        {
            config_word = cfg_word;
            config_modes->config_mode = (config_modes->config_mode & ~7) |
                                        (cfg_word & 7);
        }

        return true;
    }

    return false;
}


unsigned int pic_processor::get_config_word(unsigned int address)
{
    int i;

    if ((i = get_config_index(address)) >= 0)
    {
        return m_configMemory->getConfigWord(i)->getVal();
    }

    return 0xffffffff;
}


int pic_processor::get_config_index(unsigned int address)
{
    if (m_configMemory)
    {
        for (int i = 0; i < m_configMemory->getnConfigWords(); i++)
        {
            if (m_configMemory->getConfigWord(i))
            {
                if (m_configMemory->getConfigWord(i)->ConfigWordAdd() == address)
                {
                    return i;
                }
            }
        }
    }

    return -1;
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
//  ConfigMode
//
void ConfigMode::print()
{
    if (config_mode & CM_FOSC1x)
    {
        // Internal Oscillator type processor
        switch (config_mode & (CM_FOSC0 | CM_FOSC1))   // Lower two bits are the clock type
        {
        case 0:
            std::cout << "LP";
            break;

        case CM_FOSC0:
            std::cout << "XT";
            break;

        case CM_FOSC1:
            std::cout << "Internal RC";
            break;

        case (CM_FOSC0|CM_FOSC1):
            std::cout << "External RC";
            break;
        }

    }
    else
    {
        switch (config_mode & (CM_FOSC0 | CM_FOSC1))   // Lower two bits are the clock type
        {
        case 0:
            std::cout << "LP";
            break;

        case CM_FOSC0:
            std::cout << "XT";
            break;

        case CM_FOSC1:
            std::cout << "HS";
            break;

        case (CM_FOSC0|CM_FOSC1):
            std::cout << "RC";
            break;
        }
    }

    std::cout << " oscillator\n";

    if (valid_bits & CM_WDTE)
    {
        std::cout << " WDT is " << (get_wdt() ? "enabled\n" : "disabled\n");
    }

    if (valid_bits & CM_MCLRE)
    {
        std::cout << "MCLR is " << (get_mclre() ? "enabled\n" : "disabled\n");
    }

    if (valid_bits & CM_CP0)
    {
        if (valid_bits & CM_CP1)
        {
            std::cout << "CP0 is " << (get_cp0() ? "high\n" : "low\n");
            std::cout << "CP1 is " << (get_cp1() ? "high\n" : "low\n");

        }
        else
        {
            std::cout << "code protection is " << (get_cp0() ? "enabled\n" : "disabled\n");
        }
    }
}


//-------------------------------------------------------------------
//-------------------------------------------------------------------
void ProgramMemoryAccess::callback()
{
    if (_state)
    {
        _state = 0;
        //cout << __FUNCTION__ << " address= " << address << ", opcode= " << opcode << '\n';
        //cpu->program_memory[address]->opcode = opcode;
        put_opcode(_address, _opcode);
        // FIXME trace.opcode_write(_address,_opcode);
        bp.clear_pm_write();
    }
}

// Catch stopped simulation and update value
class WDT_Interface : public Interface
{
public:
    explicit WDT_Interface(WDT *_pt_wdt)
        : Interface((void **)_pt_wdt), pt_wdt(_pt_wdt)
    {
    }
    void SimulationHasStopped(void * /* object */ ) override
    {
	pt_wdt->WDT_counter();
    }
    void Update(void * /* object */ ) override
    {
        pt_wdt->WDT_counter();
    }

private:
    WDT *pt_wdt;
};

//--------------------------------------------------
WDT::WDT(pic_processor *p_cpu, double _timeout)
    : gpsimObject("WDT", "Watch Dog Timer"),
      cpu(p_cpu), prescale(1), postscale(128), timeout(_timeout)
{
}


//--------------------------------------------------
void WDT::update()
{
    if (wdte)
    {
        // FIXME - the WDT should not be tied to the instruction counter...
        uint64_t delta_cycles;

	if (wdttmr)
        {
	    WINprintf(("has_window low_window=%d\n", low_window));
	    wdttmr->value.put((low_window == 0)?WDTTMR::STATE:0);
	    // reset prescale to original value
	    prescale = 1 << (wdtcon0->value.get() >> 1);
	    if (low_window == 0)
	    {
		postscale = 1;
		prescale = prescale * 32;
	    }
	    else
	    {
		postscale = 8;
		prescale = prescale * 4;
	    }
	}
        else if (!use_t0_prescale)
        {
            postscale = 32;
        }




        delta_cycles = (uint64_t)(prescale * timeout / get_cycles().seconds_per_cycle());
	postscale_cnt = 0;

        if (verbose)
        {
            std::cout << "WDT::update timeout in " << (postscale * prescale * timeout);
            std::cout << " seconds (" << std::dec << delta_cycles << " cycles), ";
            std::cout << "CPU frequency " << (cpu->get_frequency()) << '\n';
        }

        last = get_cycles().get();
     WINprintf(("last=%ld delta=%ld prescale=%ld postscale=%d timeout=%f cps=%.1f\n", last, delta_cycles, prescale, postscale, timeout, 1./get_cycles().seconds_per_cycle()));

        uint64_t fc = last + delta_cycles ;

        if (future_cycle)
        {
            if (verbose)
            {
                std::cout << "WDT::update:  moving break from " << future_cycle << " to " << fc << '\n';
            }

            get_cycles().reassign_break(future_cycle, fc, this);

        }
        else
        {
            get_cycles().set_break(fc, this);
        }

        future_cycle = fc;
    }
}

//
void WDT::WDT_counter()
{
    union
    {
	uint32_t  word;
	uint8_t   byte[4];
    }count;
    count.word = 0;
    if (wdtcon0)
    {
	uint32_t delta = 0;
	if (future_cycle)
            delta = 0.5 + (get_cycles().get() - last) * get_cycles().seconds_per_cycle()/timeout;
        uint32_t scale = 1 << (wdtcon0->value.get() >> 1);
        uint32_t mask = ~(0xffff << (wdtcon0->value.get() >> 1));
        count.word = delta & mask;
        wdtpsl->value.put(count.byte[0]);
        wdtpsh->value.put(count.byte[1]);
	unsigned int val = (delta / scale ) << WDTTMR::WDTTMR_shift;
        val |= wdttmr->value.get() & WDTTMR::STATE;
	val |= count.byte[2] & 0x3;
	wdttmr->value.put(val);

    WINprintf(("precount=%d 5bit=0x%x now=%ld last=%ld delta=%d scale=%d mask=0x%x prescale=0x%x\n", count.word, delta/scale, get_cycles().get(), last, delta, scale, mask, delta & mask));

    }
}

//--------------------------------------------------
// WDT::put - shouldn't be called?
//

void WDT::put(unsigned int /* new_value */ )
{
    std::cout << "WDT::put should not be called\n";
}


void WDT::set_timeout(double _timeout)
{
    timeout = _timeout;
    update();
}

// Configure window version of WDT from config word
//  wdte - wdt enable bits
//  ccs - wdt configure clock
//  cws - wdt configure window selection
//  cps - wdt configure period selection
void WDT::config(unsigned int wdte, unsigned int ccs, unsigned int cws, unsigned int cps)
{

   WINprintf(("ccs=0x%x cws=0x%x wdte=%d cps=0x%x\n",  ccs, cws, wdte, cps));

    // Create Window registers on first call of this function
    if (!wdtcon0)
    {
	wdtcon0 = new WDTCON0(this, cpu, "wdtcon0", "Watchdog Timer Control Register 0", 0x3f);
        wdtcon1 = new WDTCON1(this, cpu, "wdtcon1", "Watchdog Timer Control Register 1");
        wdtpsl = new WDTPSL(this, cpu, "wdtpsl", "WDT Prescale Select Low Byte Register (READ ONLY)");
        wdtpsh = new WDTPSH(this, cpu, "wdtpsh", "WDT Prescale Select High Byte Register (READ ONLY)");
        wdttmr = new WDTTMR(this, cpu, "wdttmr", "WDT Timer Register (READ ONLY)");
	// keep registers up to date for GUI
	if (!wdt_interface)
        {
	    wdt_interface = new WDT_Interface(this);
	    get_interface().prepend_interface(wdt_interface);
        }
    }

     wdtcon1->wdtcs_readonly = (ccs != 7)?true:false;
     wdtcon1->window_readonly = (cws != 7)?true:false;

     // WDT armed above this postcounter
     // csw from config is 0
     low_window = (cws == 6) ? 0:(7-cws); // WDT armed below this postcounter
     // cws == 6 from config low_window == 32, set cws in wdtcon1 to 7
     int wdtcon1_por = (cws == 6) ? 7 : cws;
     if (ccs == 7) ccs= 0;
     wdtcon1_por |= ccs<<4;
     wdtcon1->por_value.put(wdtcon1_por);
     has_window = true;
     if (ccs > 1)
	fprintf(stderr, "WDT::config ccs=%d which is a reserved value\n", ccs);

     wdtcon0->wdps_readonly = true;
    //preselect
    if (cps <= 0x12)  // use value as is, no software
    {
        wdtcon0->set_por(cps<<1);
	prescale = 1 << cps;
    }
    else if (cps == 0x1f) // wdtwps software control
    {
	prescale = 1 << 0xb;
        wdtcon0->set_por(0xb<<1);
	wdtcon0->wdps_readonly = false;
    }
    else		   // fixed at 1:32 about 1ms
    {
	prescale = 1;
        wdtcon0->set_por(0);
    }

    initialize((int)wdte);
    use_t0_prescale = false;

}


//  TMR0 prescale is WDT postscale
void WDT::set_postscale(unsigned int newPostscale)
{
    unsigned int value = 1 << newPostscale;


    if (verbose)
    {
        std::cout << "WDT::set_postscale postscale = " << std::dec << value << '\n';
    }

    if (value != postscale)
    {
        postscale = value;
        update();
    }
}


void WDT::swdten(bool enable)
{
    WINprintf(("enable=%d cfgw_enable=%d wdte=%d\n", enable, cfgw_enable, wdte));
    if (!cfgw_enable)
    {
        return;
    }

    if (wdte != enable)
    {
        wdte = enable;
        warned = false;

        if (verbose)
            std::cout << " WDT swdten "
                      << ((wdte) ?  "enabling\n" : ", but disabling WDT\n");

        if (wdte)
        {
            update();

        }
        else
        {
            if (future_cycle)
            {
                if (verbose)
                {
                    std::cout << "Disabling WDT\n";
                }

                get_cycles().clear_break(this);
                future_cycle = 0;
            }
        }
    }
}


// For WDT period select, postscale counter should be 32
// if !use_t0_prescale
void WDT::set_prescale(unsigned int newPrescale)
{
    uint64_t value = 1 << newPrescale;

    if (use_t0_prescale)
	value = value << 5;

    if (verbose)
    {
        std::cout << "WDT::set_prescale prescale = " << std::dec << value << '\n';
    }

    if (value != prescale)
    {
	WINprintf(("newPrescale=%d value=%ld t0=%d\n", newPrescale, value, use_t0_prescale));
        prescale = value;
        update();
    }
}


void WDT::initialize(int enable)
{
    use_t0_prescale = true;
    cpu->set_wdt_exit_sleep(false);
    switch(enable)
    {
    case 0:	//disabled in hardware; SWDTEN bit disabled
	wdte = false;
	cfgw_enable = false;
        if (future_cycle)
        {
            std::cout << "Disabling WDT\n";
            get_cycles().clear_break(this);
            future_cycle = 0;
        }
	break;
    case 1:	//enabled when device is active, disabled when device is in Sleep; SWDTEN bit disabled
	wdte = true;
	cfgw_enable = false;
	update();
	break;
    case 2:	//controlled by the SWDTEN bit
        cpu->set_wdt_exit_sleep(true);
	wdte = false;
	cfgw_enable = true;
        if (future_cycle)
        {
            std::cout << "Disabling WDT\n";
            get_cycles().clear_break(this);
            future_cycle = 0;
        }
	break;
    case 3:	//enabled in hardware; SWDTEN bit disabled
        cpu->set_wdt_exit_sleep(true);
	wdte = true;
	cfgw_enable = false;
	update();
	break;
    }


}
void WDT::initialize(bool enable, bool _use_t0_prescale)
{
    wdte = enable;
    cfgw_enable = enable;
    use_t0_prescale = _use_t0_prescale;
    warned = false;

    if (verbose)
    {
        std::cout << " WDT init called " << ((enable) ? "enabling\n" : ", but disabling WDT\n");
    }

    if (wdte)
    {
	cfgw_enable = false;
        update();

    }
    else
    {
	cfgw_enable = true;
        if (future_cycle)
        {
            std::cout << "Disabling WDT\n";
            get_cycles().clear_break(this);
            future_cycle = 0;
        }
    }
}


void WDT::reset(RESET_TYPE r)
{
    switch (r)
    {
    case POR_RESET:
    case EXIT_RESET:
        update();
        break;

    case MCLR_RESET:
        if (future_cycle)
        {
            get_cycles().clear_break(this);
        }

        future_cycle = 0;
        break;

    default:
      WINprintf(("r=%d\n", r));
        ;
    }
}


void WDT::set_breakpoint(unsigned int bpn)
{
    breakpoint = bpn;
}


void WDT::callback()
{
    assert(wdte);
    uint64_t delta;

    delta = 0.5 + (get_cycles().get() - last) * get_cycles().seconds_per_cycle()/timeout;
    WINprintf(("total=%ld prescale=%ld  postscale_cnt=%d, postscale=%d timeout=%f cps %.1f\n", delta, prescale, postscale_cnt+1, postscale, timeout, 1./get_cycles().seconds_per_cycle()));
    if (++postscale_cnt < postscale)
    {
	if (wdttmr && (postscale_cnt == low_window))
	{
	    WINprintf(("now=%ld set state active postscale_cnt=%d low_window=%d\n", get_cycles().get(), postscale_cnt, low_window));
		//set state active
		wdttmr->value.put(wdttmr->value.get() | WDTTMR::STATE);
        }


        delta = (uint64_t)(prescale * timeout / get_cycles().seconds_per_cycle());
 	future_cycle = get_cycles().get() + delta;
        get_cycles().set_break(future_cycle, this);
	return;
    }


    if (wdte)
    {
        if (verbose)
        {
            std::cout << "WDT timeout: " << std::hex << get_cycles().get() << '\n';
        }

        if (breakpoint)
        {
            bp.halt();

        }
        else if (cpu->is_sleeping() && cpu->exit_wdt_sleep())
        {
            std::cout << "WDT expired during sleep\n";
            update();
            cpu->exit_sleep();
            cpu->status->put_TO(0);

        }
        else
        {
            // The TO bit gets cleared when the WDT times out.
            std::cout << "WDT expired reset\n";
            update();
            cpu->status->put_TO(0);
            cpu->reset(WDT_RESET);
        }
    }
}


void WDT::clear()
{
    if (wdte)
    {
	if (wdttmr)
	{
    WINprintf(("wdte=%d wdttmr::state=%d\n", wdte, (bool)(wdttmr->value.get() & WDTTMR::STATE)));
	    // if state not active, do WDT break
	    if (!(wdttmr->value.get() & WDTTMR::STATE))
	    {
        	if (future_cycle)
            	    get_cycles().clear_break(this);
		future_cycle = 0;
                cpu->status->put_TO(0);
                cpu->reset(WDTWV_RESET);
		return;
	    }
        }
        update();

    }
    else if (!warned)
    {
        warned = 1;
        std::cout << "The WDT is not enabled - clrwdt has no effect!\n";
    }
    cpu->status->put_TO(1);
}


void WDT::callback_print()
{
    std::cout <<  name() << " has callback, ID = " << CallBackID << '\n';
    // cout << "WDT\n";
}


//------------------------------------------------------------------------
// ConfigMemory - Base class
ConfigWord::ConfigWord(const char *_name, unsigned int default_val, const char *desc,
                       pic_processor *pCpu, unsigned int addr, bool EEw)
    : Integer(_name, default_val, desc), m_pCpu(pCpu), m_addr(addr),
      EEWritable(EEw)
{
    /*
    if (m_pCpu)
      m_pCpu->addSymbol(this);
    */
}


// this get controls the display format in the symbols window
void ConfigWord::get_as(char *buffer, int buf_size)
{
    if (buffer)
    {
        int64_t i;
        get_as(i);
        long long int j = i;
        snprintf(buffer, buf_size, "0x%" PRINTF_INT64_MODIFIER "x", j);
    }
}


void ConfigWord::get_as(int64_t &i)
{
    Integer::get_as(i);
}


//------------------------------------------------------------------------
ConfigMemory::ConfigMemory(pic_processor *pCpu, unsigned int nWords)
    : m_pCpu(pCpu), m_nConfigWords(nWords)
{
    if (nWords > 0 && nWords < 100)
    {
        m_ConfigWords = new ConfigWord *[nWords];

        for (unsigned int i = 0; i < nWords; i++)
        {
            m_ConfigWords[i] = nullptr;
        }
    }
}


ConfigMemory::~ConfigMemory()
{
    for (unsigned int i = 0; i < m_nConfigWords; i++)
        if (m_ConfigWords[i])
        {
            m_pCpu->deleteSymbol(m_ConfigWords[i]);
        }

    delete [] m_ConfigWords;
}


int ConfigMemory::addConfigWord(unsigned int addr, ConfigWord *pConfigWord)
{
    if (addr < m_nConfigWords)
    {
        if (m_ConfigWords[addr])
        {
            m_pCpu->deleteSymbol(m_ConfigWords[addr]);
        }

        m_ConfigWords[addr] = pConfigWord;
        m_pCpu->addSymbol(pConfigWord);
        return 1;
    }

    delete pConfigWord;
    return 0;
}


ConfigWord *ConfigMemory::getConfigWord(unsigned int addr)
{
    return addr < m_nConfigWords ? m_ConfigWords[addr] : nullptr;
}


//-------------------------------------------------------------------
class MCLRPinMonitor : public PinMonitor
{
public:
    explicit MCLRPinMonitor(pic_processor *pCpu);
    ~MCLRPinMonitor() {}

    void setDrivenState(char) override;
    void setDrivingState(char) override {}
    void set_nodeVoltage(double) override {}
    void putState(char) override {}
    void setDirection() override {}

private:
    pic_processor *m_pCpu;
    char m_cLastResetState;
};


MCLRPinMonitor::MCLRPinMonitor(pic_processor *pCpu)
    : m_pCpu(pCpu),
      m_cLastResetState('I')  // I is not a valid state. It's used here for 'I'nitialization
{
}


void MCLRPinMonitor::setDrivenState(char newState)
{
    if (newState == '0' || newState == 'w')
    {
        m_cLastResetState = '0';
        m_pCpu->reset(MCLR_RESET);
    }

    if (newState == '1' || newState == 'W')
    {
        if (m_cLastResetState == '0')
        {
            m_pCpu->reset(EXIT_RESET);
        }

        m_cLastResetState = '1';
    }
}


//-------------------------------------------------------------------
void pic_processor::createMCLRPin(int pkgPinNumber)
{
    if (m_MCLR)
    {
        std::cout << "BUG?: assigning multiple MCLR pins: " __FILE__ << std::dec << " " << __LINE__ << '\n';
    }

    if (package)
    {
        m_MCLR = new IO_open_collector("MCLR");
        package->assign_pin(pkgPinNumber, m_MCLR);
        addSymbol(m_MCLR);
        m_MCLRMonitor = new MCLRPinMonitor(this);
        m_MCLR->setMonitor(m_MCLRMonitor);
    }
}


//-------------------------------------------------------------------
// This function is called instead of createMCLRPin where the pin
// is already defined, but the configuration word has set the function
// to MCLR


void pic_processor::assignMCLRPin(int pkgPinNumber)
{
    if (package)
    {
        if (m_MCLR == nullptr)
        {
            m_MCLR_pin = pkgPinNumber;
            m_MCLR = new IO_open_collector("MCLR");
            addSymbol(m_MCLR);
            m_MCLR_Save = package->get_pin(pkgPinNumber);
            package->assign_pin(pkgPinNumber, m_MCLR, false);
            m_MCLRMonitor = new MCLRPinMonitor(this);
            m_MCLR->setMonitor(m_MCLRMonitor);
            m_MCLR->newGUIname("MCLR");

        }
        else if (m_MCLR != package->get_pin(pkgPinNumber))
        {
            std::cout << "BUG?: assigning multiple MCLR pins: "
                      << std::dec << pkgPinNumber << " " __FILE__ " "
                      << __LINE__ << '\n';
        }
    }
}


//-------------------------------------------------------------------
// This function sets the pin currently set as MCLR back to its original function
void pic_processor::unassignMCLRPin()
{
    if (package && m_MCLR_Save)
    {
        size_t l = m_MCLR_Save->name().find_first_of('.');
        package->assign_pin(m_MCLR_pin, m_MCLR_Save, false);

        if (l == std::string::npos)
        {
            m_MCLR_Save->newGUIname(m_MCLR_Save->name().c_str());

        }
        else
        {
            m_MCLR_Save->newGUIname(m_MCLR_Save->name().substr(l + 1).c_str());
        }

        if (m_MCLR)
        {
            m_MCLR->setMonitor(nullptr);
            deleteSymbol(m_MCLR);
            m_MCLR = nullptr;

            delete m_MCLRMonitor;
            m_MCLRMonitor = nullptr;
        }
    }
}


//--------------------------------------------------
//
class IO_SignalControl : public SignalControl
{
public:
    explicit IO_SignalControl(char _dir)
        : direction(_dir)
    {
    }
    ~IO_SignalControl() {}
    char getState() override
    {
        return direction;
    }
    void release() override {}
    void setState(char _dir)
    {
        direction = _dir;
    }

private:
    char direction;
};


// This function sets a label on a pin and if PinMod is defined
// removes its control from it's port register
//
void pic_processor::set_clk_pin(unsigned int pkg_Pin_Number,
                                PinModule *PinMod,
                                const char * name,
                                bool in,
                                PicPortRegister *m_port,
                                PicTrisRegister *m_tris,
                                PicLatchRegister *m_lat)
{
    IOPIN *m_pin = package->get_pin(pkg_Pin_Number);

    if (name)
    {
        m_pin->newGUIname(name);

    }
    else
    {
        m_pin->newGUIname(package->get_pin_name(pkg_Pin_Number).c_str());
    }

    if (PinMod)
    {
        if (m_port)
        {
            unsigned int mask = m_port->getEnableMask();
            mask &= ~(1 << PinMod->getPinNumber());
            m_port->setEnableMask(mask);

            if (m_tris)
            {
                m_tris->setEnableMask(mask);
            }

            if (m_lat)
            {
                m_lat->setEnableMask(mask);
            }
        }

        if (!clksource)
        {
            clksource = new  PeripheralSignalSource(PinMod);
            clkcontrol = new IO_SignalControl(in ? '1' : '0');
        }

        PinMod->setSource(clksource);
        PinMod->setControl(clkcontrol);
        PinMod->updatePinModule();
    }
}


// This function reverses the effects of the previous function
void pic_processor::clr_clk_pin(unsigned int pkg_Pin_Number,
                                PinModule *PinMod,
                                PicPortRegister *m_port,
                                PicTrisRegister *m_tris,
                                PicLatchRegister *m_lat)
{
    IOPIN *m_pin = package->get_pin(pkg_Pin_Number);
    m_pin->newGUIname(package->get_pin_name(pkg_Pin_Number).c_str());

    if (PinMod)
    {
        if (m_port)
        {
            unsigned int mask = m_port->getEnableMask();
            mask |= (1 << PinMod->getPinNumber());
            m_port->setEnableMask(mask);

            if (m_tris)
            {
                m_tris->setEnableMask(mask);
            }

            if (m_lat)
            {
                m_lat->setEnableMask(mask);
            }
        }

        PinMod->setSource(nullptr);
        PinMod->setControl(nullptr);
        PinMod->updatePinModule();
    }
}


void pic_processor::osc_mode(unsigned int value)
{
    IOPIN *m_pin;
    unsigned int pin_Number =  get_osc_pin_Number(0);

    if (pin_Number < 253)
    {
        m_pin = package->get_pin(pin_Number);
    }

    if ((pin_Number =  get_osc_pin_Number(1)) < 253 &&
            (m_pin = package->get_pin(pin_Number)))
    {
        pll_factor = 0;

        if (value < 5)
        {
            set_clk_pin(pin_Number, m_osc_Monitor[1], "OSC2", true);

        }
        else if (value == 6)
        {
            pll_factor = 2;
            set_clk_pin(pin_Number, m_osc_Monitor[1], "CLKO", false);

        }
        else
        {
            clr_clk_pin(pin_Number, m_osc_Monitor[1]);
        }
    }
}


void pic_processor::Wput(unsigned int value)
{
    Wreg->put(value);
}


unsigned int pic_processor::Wget()
{
    return Wreg->get();
}
