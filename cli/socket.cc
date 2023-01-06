/*
   Copyright (C) 2004 T. Scott Dattalo

This file is part of gpsim.

gpsim is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

gpsim is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with gpsim; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

#include <glib.h>

#include <string>
#include <iostream>
#include <memory>

#ifndef _WIN32
#include <pthread.h>
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

#include "../src/symbol.h"
#include "../src/protocol.h"
#include "../src/gpsim_interface.h"
#include "../src/breakpoints.h"
#include "../src/gpsim_time.h"

#include "../src/trigger.h"
#include "../src/value.h"

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#else
#include <winsock2.h>
#endif
void exit_gpsim(int);

//#define DEBUG
#if defined(DEBUG)
#include "../config.h"
#define Dprintf(arg) {printf("%s:%d ",__FILE__,__LINE__); printf arg; }
#else
#define Dprintf(arg) {}
#endif



//******** E X P E R I M E N T A L   C O D E !!! *************************
// Here is an experimental attribute for testing the socket interface.
// The purpose is to isolate socket testing from the rest of the simulator.
//
class TestInt32Array : public Value
{
public:
    TestInt32Array(const char *_name, int _sz)
        : Value(_name, " test array for testing sockets"),
          size(_sz)
    {
        array = new int[size];
    }
    TestInt32Array(const TestInt32Array &) = delete;
    TestInt32Array& operator =(const TestInt32Array &) = delete;
    ~TestInt32Array()
    {
        delete[] array;
    }

    void set(const char * /* cP */, int len = 0) override
    {
        (void)len;
        std::cout << name() << " set\n";
    }
    void get(char *, int /* len */ ) override
    {
        std::cout << name() << " get\n";
    }
    std::string toString() override
    {
        return name();
    }

private:
    int size;
    int *array;
};


//*************************  end of experimental code ********************


class SocketLink;
class Socket;

// in input.cc -- parse_string sends a string through the command parser
extern int parse_string(const char * str);

#ifdef WIN32
#define snprintf  _snprintf
#define socklen_t int

bool server_started = false;


void psocketerror(const char *str)
{
    fprintf(stderr, "%s: %s\n", str, g_win32_error_message(WSAGetLastError()));
}


bool winsockets_init()
{
    WSADATA wsaData;
    int err;

    if (0 != (err = WSAStartup(MAKEWORD(2, 2), &wsaData)))
    {
        return false;
    }

    /* Confirm that the WinSock DLL supports 2.2.*/
    /* Note that if the DLL supports versions greater    */
    /* than 2.2 in addition to 2.2, it will still return */
    /* 2.2 in wVersion since that is the version we      */
    /* requested.                                        */

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
    {
        /* Tell the user that we could not find a usable */
        /* WinSock DLL.                                  */
        WSACleanup();
        return false;
    }

    return true;
}


#else
#define psocketerror(s) perror(s)
#define closesocket(s)  close(s)
#define SOCKET          int
#define INVALID_SOCKET  -1
#endif


//

/*

  gpsim sockets.

  The purpose of a socket interface into the simulator is to provide an
  external interface that only loosely depends upon gpsim's internals.

  Towards the bottom of this file is code that will start a server
  'process' that will listen for clients. This 'process' is really
  a glib GIOChannel. The GIOChannel is a glib mechanism for handling
  things like sockets so that they work nicely when compiled for other
  platforms. Now, whenever a client connects to gpsim, another GIOChannel
  is created. This new channel has the client socket associated with
  it and is used to communicate with the client.

  Thus, like most socket server applications, gpsim creates a single
  socket to listen for clients. As clients come along, connections are
  established with them. gpsim uses GLIB's GIOChannels to facilitate
  the simultaneous multiple connections.

  Once a client has connected to gpsim, it can do just about everything
  a user sitting at a keyboard can do. In fact, the client can issue
  command line commands that are directed to the command line parser.
  Although, at the moment, the output of these commands still is sent
  to the terminal from which the gpsim executable was spawned (instead
  of being sent to the client socket...). Probably more powerful though
  is the ability for a client to establish 'links' to gpsim internal
  objects. These links are provide a client with an efficient access
  to gpsim's symbol table.

  There are two kinds of socket connection types that gpsim supports.
  I call them 'Simulator Source' and 'Simulator Sink' sockets. They're
  both similar in that they connect to a client through the normal
  mechanism. However, they're different in how they behave while the
  simulator is running. The Simulator Sink sockets are designed to be
  controlled by a GLIB GIOChannel. They are intended to receive
  unsolicited data from a client. In addition, since they're
  controlled by a GIOChannel, they can only be processed while the GTK
  main loop has control (which in the current implementation is all
  time except when the simulation is running). So the idea is that
  clients can send stuff to the simulator, and if the simulator is not
  running, the information is processed and a response is returned to
  the client.

  Simulator Sources are for sending unsolicited simulator data to a
  client. This is a blocking operation, i.e. it will not return until
  the client has sent a response. Now, the client is responsible for
  setting Simulator Source Sockets up. So during the socket connection
  and initialization the client specifies what it wants the simulator
  to send unsolicited. You might imagine a situation where a plugin
  module may be receiving data from a script.

*/


#define SIM_SINK_PORT   0x1234
#define SIM_SOURCE_PORT 0x1235

#define BUFSIZE         8192
//------------------------------------------------------------------------
class AttributeLink;
bool ParseSocketLink(Packet *buffer, AttributeLink **);


//------------------------------------------------------------------------
//------------------------------------------------------------------------
//

class SocketInterface : public Interface
{
private:
    //  Socket *sock;

public:
    explicit SocketInterface(Socket *);
    virtual ~SocketInterface();

    virtual void SimulationHasStopped(gpointer object);
    virtual void Update(gpointer object);
};


//------------------------------------------------------------------------
// Socket wrapper class
//
// This class is a simple wrapper around the standard BSD socket calls.
//


class SocketBase
{
public:
    explicit SocketBase(SOCKET s);
    SocketBase(const SocketBase &) = delete;
    SocketBase& operator =(const SocketBase &) = delete;
    ~SocketBase();

    SOCKET getSocket();
    void Close();
    bool Send(const char *);
    void Service();
    void ParseObject();

    Packet *packet;

private:
    SOCKET socket;
};


//------------------------------------------------------------------------
// Socket class
// This class is responsible for listening to clients.
//
class Socket
{
public:
    Socket();
    ~Socket();

    void init(int port);
    void AssignChannel(gboolean(*server_func)(GIOChannel *, GIOCondition, void *));

    void Close(SocketBase**);
    void Bind();
    void Listen();
    SocketBase * Accept();

    SocketBase *my_socket;

    struct   sockaddr_in addr;
};


class SocketLink
{
public:
    SocketLink(unsigned int _handle, SocketBase *);

    virtual ~SocketLink()
    {
    }

    /// Send a response back to the link.
    /// if the bTimeStamp is true, then the cycle counter is sent too.
    bool Send(bool bTimeStamp = false);
    bool Receive();

    virtual void set(Packet &) = 0;
    virtual void get(Packet &) = 0;
    unsigned int getHandle()
    {
        return handle;
    }
    void setBlocking(bool bBlocking)
    {
        bWaitForResponse = bBlocking;
    }
    bool bBlocking()
    {
        return bWaitForResponse;
    }

private:
    unsigned int handle;
    SocketBase *parent;
    bool bWaitForResponse;
};


///========================================================================
/// AttributeLink
/// An attribute link is a link associated with a gpsim attribute.
/// Clients will establish attribute links by providing the name
/// of the gpsim object with which they wish to link. The over head
/// for parsing the link name and looking it up in the table only
/// has to be done once.

class AttributeLink : public SocketLink
{
public:
    AttributeLink(unsigned int _handle, SocketBase *, gpsimObject *);

    virtual ~AttributeLink()
    {
    }

    void set(Packet &) override;
    void get(Packet &) override;

    //Value *getValue()
    gpsimObject *getValue()
    {
        return v;
    }

private:
    /// This is a pointer to the gpsim Value object associated with
    /// this link.
//  Value *v;
    gpsimObject *v;
};


///========================================================================
/// NotifyLink
/// A notify link is a link designed to be a sort of cross reference. It
/// is associated with a gpsim attribute in such a way that whenever the
/// gpsim attribute changes, the notify link will notify a client socket.

class NotifyLink : public XrefObject
{
public:
    explicit NotifyLink(AttributeLink *);
    void set(gint64);
    void update();

private:
    AttributeLink *sl;
};


///========================================================================
/// CyclicCallBackLink
/// A cyclic call back link is a link that periodically sends a message
/// to the client.

class CyclicCallBackLink : public TriggerObject
{
public:
    CyclicCallBackLink(guint64, SocketBase *);
    ~CyclicCallBackLink();

    void callback() override;
    void callback_print() override;

private:
    guint64 interval;
    SocketBase *sb;
};


//========================================================================
#define nSOCKET_LINKS 16

AttributeLink *links[nSOCKET_LINKS];

AttributeLink *gCreateSocketLink(unsigned int, Packet &, SocketBase *);


unsigned int FindFreeHandle()
{
    unsigned int i;
    static unsigned int sequence = 0;

    for (i = 0; i < nSOCKET_LINKS; i++)
        if (links[i] == nullptr)
        {
            return i | (++sequence << 16);
        }

    return 0xffff;
}


//========================================================================
SocketBase::SocketBase(SOCKET s)
    : socket(s)
{
    packet = new Packet(BUFSIZE, BUFSIZE);
}


SocketBase::~SocketBase()
{
    delete packet;
}


SOCKET SocketBase::getSocket()
{
    return socket;
}


void SocketBase::Close()
{
    if (socket != INVALID_SOCKET)
    {
        closesocket(socket);
    }

    socket = INVALID_SOCKET;
}


//========================================================================
bool ParseSocketLink(Packet *buffer, AttributeLink **al)

{
    if (!al)
    {
        return false;
    }

    unsigned int handle;

    if (buffer->DecodeUInt32(handle))
    {
        *al = links[handle & 0x0f];

        if ((*al) && (*al)->getHandle() != handle)
        {
            *al = nullptr;
        }

        return true;
    }

    *al = nullptr;
    printf("ParseSocketLink fail no handle\n");
    return false;
}


//========================================================================
void CloseSocketLink(SocketLink *sl)
{
    if (!sl)
    {
        return;
    }

    unsigned int handle = sl->getHandle();
    std::cout << " closing link with handle 0x" << std::hex << handle << '\n';

    if (links[handle & 0x000f] == sl)
    {
        links[handle & 0x000f] = nullptr;
    }
}


//========================================================================
Socket::Socket()
{
    my_socket = nullptr;

    for (int i = 0; i < nSOCKET_LINKS; i++)
    {
        links[i] = nullptr;
    }
}


Socket::~Socket()
{
    Close(&my_socket);
}


void Socket::init(int port)
{
    SOCKET new_socket;

    if ((new_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
    {
        psocketerror("socket");
        exit_gpsim(1);
    }

    my_socket = new SocketBase(new_socket);
    int on = 1;

    if (setsockopt(new_socket, SOL_SOCKET, SO_REUSEADDR, (const char*) &on, sizeof(on)) != 0)
    {
        psocketerror("setsockopt");
        exit_gpsim(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    Bind();
    Listen();
}


void Socket::AssignChannel(gboolean(*server_function)(GIOChannel *, GIOCondition, void *))
{
    if (my_socket->getSocket() > 0)
    {
        GIOChannel *channel = g_io_channel_unix_new(my_socket->getSocket());
        GIOCondition condition = (GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_ERR);
        GError *err = nullptr;
        g_io_channel_set_encoding(channel, nullptr, &err);
#if !defined _WIN32 || defined _DEBUG
        g_io_channel_set_flags(channel, G_IO_FLAG_SET_MASK, &err);
#endif
        g_io_add_watch(channel,
                       condition,
                       server_function,
                       (void*)this);
    }
}


void Socket::Close(SocketBase **sock)
{
    if (sock && *sock)
    {
        (*sock)->Close();
        *sock = nullptr;
    }
}


void Socket::Bind()
{
    if (!my_socket)
    {
        return;
    }

    if (bind(my_socket->getSocket(), (struct sockaddr *) &addr, sizeof(addr)) != 0)
    {
        psocketerror("bind");
    }
}


void Socket::Listen()
{
    if (!my_socket)
    {
        return;
    }

    if (listen(my_socket->getSocket(), 5) != 0)
    {
        psocketerror("listen");
    }
}


SocketBase *Socket::Accept()
{
    socklen_t addrlen = sizeof(addr);
    SOCKET client_socket = accept(my_socket->getSocket(), (struct sockaddr *)  &addr, &addrlen);

    if (client_socket == INVALID_SOCKET)
    {
        psocketerror("accept");
        exit_gpsim(1);
    }

    return new SocketBase(client_socket);
}


bool SocketBase::Send(const char *b)
{
    if (!socket)
    {
        return false;
    }

    //std::cout << "Sending sock="<<socket << " data " << b << std::endl;

    if (send(socket, b, strlen(b), 0) < 0)
    {
        psocketerror("send");
        closesocket(socket);
        return false;
    }

    return true;
}


//------------------------------------------------------------------------
// ParseObject
//
// A client socket has sent data to gpsim that ParseObject has now been
// given the job to interpret. ParseObject will decode the data using
// the gpsim protocol (described in ../src/protocol.h).
//
// The protocol generally consists of a command/data pair. Each command
// has a unique identifier and specifies the type of object. ParseObject
// extracts this information and then uses a large switch statement to
// perform the operation for this type.

void SocketBase::ParseObject()
{
    unsigned int ObjectType;

    if (!packet->DecodeObjectType(ObjectType))
    {
        return;
    }

    switch (ObjectType)
    {
    case GPSIM_CMD_CREATE_SOCKET_LINK:
    {
        unsigned int handle = FindFreeHandle();
        Dprintf(("Parse GPSIM_CMD_CREATE_SOCKET_LINK handle=0x%x\n", handle));
        AttributeLink *al = gCreateSocketLink(handle, *packet, this);

        if (al)
        {
            links[handle & 0x0f] = al;
            packet->EncodeHeader();
            packet->EncodeUInt32(handle);
            packet->txTerminate();
            Send(packet->txBuff());
        }
    }
    break;

    case GPSIM_CMD_CREATE_NOTIFY_LINK:
    {
        Dprintf(("Parse GPSIM_CMD_CREATE_NOTIFY_LINK\n"));
        unsigned int handle = FindFreeHandle();
        AttributeLink *al = gCreateSocketLink(handle, *packet, this);

        if (al)
        {

	    printf("Notify link not currently supported\n");
#ifdef RRR
            unsigned int i = 0;

            if (packet->DecodeUInt32(i) && i)
            {
                al->setBlocking(true);
            }

            //NotifyLink *nl = new NotifyLink(al);
            new NotifyLink(al);	//RRR
            links[handle & 0x0f] = al;
            packet->EncodeHeader();
            packet->EncodeUInt32(handle);
            packet->txTerminate();
            Send(packet->txBuff());
#endif
        }
    }
    break;

    case GPSIM_CMD_CREATE_CALLBACK_LINK:
    {
        Dprintf(("Parse GPSIM_CMD_CREATE_CALLBACK_LINK\n"));
        unsigned int handle = FindFreeHandle();
        guint64 interval = 0;

        if (packet->DecodeUInt64(interval) && interval)
        {
            std::cout << "Creating callback link interval=" << interval << '\n';
            new CyclicCallBackLink(interval,this);
            //links[handle&0x0f] = al;
            packet->EncodeHeader();
            packet->EncodeUInt32(handle);
            packet->txTerminate();
            Send(packet->txBuff());
        }
    }
    break;

    case GPSIM_CMD_REMOVE_SOCKET_LINK:
    {
        AttributeLink *al = nullptr;
        std::cout << "remove socket link command\n";
        ParseSocketLink(packet, &al);

        if (al)
        {
            CloseSocketLink(al);
        }

        Send("$");
    }
    break;

    case GPSIM_CMD_QUERY_SOCKET_LINK:
    {
        AttributeLink *al = nullptr;
        ParseSocketLink(packet, &al);

        Dprintf(("Parse GPSIM_CMD_QUERY_SOCKET_LINK %s type=%s\n", al->getValue()->name().c_str(), al->getValue()->showType().c_str()));
        if (al)
        {
            al->Send();
        }
    }
    break;

    case GPSIM_CMD_WRITE_TO_SOCKET_LINK:
    {
        AttributeLink *al = nullptr;
        ParseSocketLink(packet, &al);
        Dprintf(("Parse GPSIM_CMD_WRITE_TO_SOCKET_LINK %s type=%s\n", al->getValue()->name().c_str(), al->getValue()->showType().c_str()));

        if (al)
        {
            al->set(*packet);
            Send("$");
        }
    }
    break;

    case GPSIM_CMD_QUERY_SYMBOL:
    {
        char tmp[256];

        if (packet->DecodeString(tmp, 256))
        {
            gpsimObject *sym = globalSymbolTable().find(tmp);

            Dprintf(("Parse GPSIM_CMD_QUERY_SYMBOL name=%s type=%s\n", tmp, sym->showType().c_str()));

            if (sym)
            {
                packet->EncodeHeader();
                //RRRsym->get(*packet);
                packet->txTerminate();
                Send(packet->txBuff());

            }
            else
            {
                Send("-");
            }
        }
    }
    break;

    case GPSIM_CMD_WRITE_TO_SYMBOL:
    {
        // The client has requested to write directly to a symbol
        // (without using a SocketLink). So, we'll extract the symbol
        // name from the packet and look it up in the symbol table.
        // If the symbol is found then, it will get assigned the
        // value that's stored in the packet.
        char tmp[256];

        if (packet->DecodeString(tmp, 256))
        {
            Value *sym = globalSymbolTable().findValue(tmp);
            Dprintf(("Parse GPSIM_CMD_WRITE_TO_SYMBOL %s type=%s\n", tmp, sym?sym->showType().c_str():"???"));
            if (sym)
            {
                // We'll define the response header before we decode
                // the rest of the packet. The reason for this is that
                // some symbols may wish to generate a response.
                packet->EncodeHeader();
                sym->set(*packet);
                packet->txTerminate();
                Send(packet->txBuff());

            }
            else
            {
                Send("-");
            }
        }
    }
    break;

    case GPSIM_CMD_RUN:
    {
        guint64 nCycles;
        guint64 startCycle = get_cycles().get();

        // Extract from the packet the number of cycles we should run.
        // If the number of cycles is greater than 0, then we'll set
        // a cycle breakpoint; otherwise we'll run forever.

        if (packet->DecodeUInt64(nCycles))
        {
            guint64 fc = startCycle + nCycles;

            if (nCycles)
            {
                get_bp().set_cycle_break(0, fc);
            }
        }

        // Start running...
        get_interface().start_simulation();
        // The simulation has stopped. For the response (to the run
        // command) we'll send the current value of the cycle counter.
        // (A client can use this to determine if the break was due to
        // cycle break set above or to something else).
        packet->EncodeObjectType(GPSIM_CMD_RUN);
        packet->EncodeUInt64(get_cycles().get() - startCycle);
        packet->txTerminate();
        Send(packet->txBuff());
    }
    break;

    case GPSIM_CMD_RESET:
        get_interface().reset();
        Send("-");
        break;

    default:
        printf("Invalid object type: %u\n", ObjectType);
        Send("-");
    }
}


//------------------------------------------------------------------------
// Service()
//
// A client has sent data to gpsim through the socket interface. It is here
// where we validate that data and decide how to handle it.

void SocketBase::Service()
{
    if (packet->brxHasData())
    {
        // If the data is a 'pure socket command', which is to say it begins
        // with a header as defined in ../src/protocol.h, then the data
        // will be further parsed by ParseObject(). If the data has an
        // invalid header, then we assume that the data is for the command
        // line and we let the CLI parse_string() function handle it.
        // FIXME gpsim poorly handles this CLI interface. We should send
        // the output of the CLI back to the client socket.
        if (packet->DecodeHeader())
        {
            ParseObject();

        }
        else
        {
            if (parse_string(packet->rxBuff()) >= 0)
            {
                Send("+ACK");

            }
            else
            {
                Send("+BUSY");
            }
        }
    }
}


//========================================================================
// Socket Interface

SocketInterface::SocketInterface(Socket * )
//  : sock(new_socket)
{
}


void SocketInterface::SimulationHasStopped(gpointer /* object */ )
{
}


void SocketInterface::Update(gpointer /* object */ )
{
    /*
    if(sock)
      sock->Service();
    printf("socket update\n");
    */
}


SocketInterface::~SocketInterface()
{
    /*
    if(sock)
      sock->Service();
    */
}


//========================================================================

SocketLink::SocketLink(unsigned int _handle, SocketBase *sb)
    : handle(_handle), parent(sb), bWaitForResponse(false)
{
}


bool SocketLink::Send(bool bTimeStamp)
{
    if (parent)
    {
        parent->packet->prepare();
        parent->packet->EncodeHeader();
        get(*parent->packet);

        if (bTimeStamp)
        {
            parent->packet->EncodeUInt64(get_cycles().get());
        }

        parent->packet->txTerminate();

        /*
            std::cout << "SocketLink::Send() "
                      << " socket=" << parent->getSocket()
                      << " sending "
                      << parent->packet->txBuff() << std::endl;
        */

        if (bWaitForResponse)
        {
//      std::cout << "SocketLink::Send waiting for response\n";
            if (parent->Send(parent->packet->txBuff()))
            {
                return Receive();
            }

        }
        else
        {
            return parent->Send(parent->packet->txBuff());
        }
    }

    return false;
}


bool SocketLink::Receive()
{
    if (parent)
    {
        //cout << "SocketLink is waiting for a client response\n";
        parent->packet->prepare();
        int bytes = recv(parent->getSocket(),
                         parent->packet->rxBuff(),
                         parent->packet->rxSize(), 0);

        if (bytes  == -1)
        {
            perror("recv");
            exit_gpsim(1);
        }

        parent->packet->rxTerminate(bytes);
        /*
        cout << "SocketLink got client response: " << parent->packet->rxBuff() <<
          endl;
        */
        return true;
    }

    return false;
}


//========================================================================
AttributeLink::AttributeLink(unsigned int _handle, SocketBase *_sb, gpsimObject *_v)
    : SocketLink(_handle, _sb), v(_v)
{
}


void AttributeLink::set(Packet &p)
{
    if (v)
    {
        unsigned int new_value;
        p.DecodeUInt32(new_value);
        if (!v->showType().compare("Register"))
        {
            ((Register *)v)->value.put(new_value);
        }
        else if (!v->showType().compare("Integer"))
        {
            gint64 i = new_value &0xffffffff;
            ((Integer *)v)->set(i);
            ((Value *)v)->get(i);
        }
        else
        {
            printf("Fix me AttributeLink::set %s unexpected type %s\n", v->name().c_str(), v->showType().c_str());
        }
    }
}


void AttributeLink::get(Packet &p)
{
    if (v)
    {
        if (!v->showType().compare("Register"))
        {
            unsigned int i = ((Register *)v)->get();
            //v->get(p);
            p.EncodeUInt32(i & 0xffffff);
        }
        else if (!v->showType().compare("Integer"))
        {
            gint64 i;
            ((Integer *)v)->get(i);
            p.EncodeUInt32(i & 0xffffff);
        }
        else
        {
            printf("Fix me AttributeLink::get %s unexpected type %s\n", v->name().c_str(), v->showType().c_str());
        }
    }
}


//========================================================================
NotifyLink::NotifyLink(AttributeLink *_sl)
    : XrefObject(), sl(_sl)
{
    std::cout << "Creating a notify link \n";

    if (sl && sl->getValue())
    {
        gpsimObject *v = sl->getValue();
        std::cout << "Creating a notify link and asoc with " << v->name() << " " << v->showType() << '\n';
        if (!v->showType().compare("Register"))
	{
	    ((Register *)v)->add_xref(this);
	}
        //FIXME: removed 27JAN06 sl->getValue()->set_xref(this);
        //v->set_xref(this);
    }
}


void NotifyLink::update()
{
    printf("RRR NotifyLink::update()\n");
}
void NotifyLink::set(gint64 )
{
    //std::cout << "notify link is sending data back to client\n";
    if (sl)
    {
        sl->Send(true);
    }
}


//========================================================================

CyclicCallBackLink::CyclicCallBackLink(guint64 i, SocketBase *_sb)
    : interval(i), sb(_sb)
{
    std::cout << " cyclic callback object\n ";
    get_cycles().set_break(get_cycles().get() + interval, this);
    get_cycles().dump_breakpoints();
}

CyclicCallBackLink::~CyclicCallBackLink()
{
}


void CyclicCallBackLink::callback()
{
    std::cout << "CyclicCallBackLink callback now=" << get_cycles().get() <<"\n";

    if (sb)
    {
        static bool bfirst = true;
        static char st[5];

        if (bfirst)
        {
            bfirst = false;
            st[0] = 'h';
            st[1] = 'e';
            st[2] = 'y';
            st[3] = '0';
            st[4] = 0;
        }
        else if (++st[3] > '9')
        {
            st[3] = '0';
        }

        if (sb->Send(st))
        {
            get_cycles().set_break(get_cycles().get() + interval, this);

        }
        else
        {
            static int seq = 0;

            std::cout << "socket callback failed seq:" << seq++ << '\n';
        }
    }
}


void CyclicCallBackLink::callback_print()
{
    std::cout << "CyclicCallBackLink callback\n ";
}


//========================================================================

AttributeLink *gCreateSocketLink(unsigned int handle, Packet &p, SocketBase *sb)
{
    char tmp[256];

    if (p.DecodeString(tmp, 256))
    {
        gpsimObject *sym = globalSymbolTable().find(tmp);

        if (sym)
        {
            return new AttributeLink(handle, sb, sym);
        }
        else
        {
            globalSymbolTable().addSymbol(sym = new Integer(tmp, 0));
            return new AttributeLink(handle, sb, sym);
        }
    }

    printf("AttributeLink *gCreateSocketLink Symbol name not in packet\n");

    return nullptr;
}


//////////////////////////////////////////////////////////////////////////

#ifdef USE_THREADS_BUT_NOT_RECOMMENDED_BECAUSE_OF_CROSS_PLATFORM_ISSUES


void *server_thread(void *ignored)
{
    std::cout << "running....\n";
    Socket s;

    while (true)
    {
        s.receive();
    }

    return nullptr;
}


pthread_t thSocketServer;
pthread_attr_t thAttribute;
int something = 1;


void start_server()
{
    std::cout << "starting server....\n";
    pthread_attr_init(&thAttribute);
    pthread_attr_setdetachstate(&thAttribute, PTHREAD_CREATE_JOINABLE);
    pthread_create(&thSocketServer, &thAttribute, server_thread, (void *)&something);
    std::cout << " started server\n";
}


#else   // if USE_THREADS


static void debugPrintChannelStatus(GIOStatus stat)
{
    switch (stat)
    {
    case G_IO_STATUS_ERROR:
        std::cout << "G_IO_STATUS_ERROR\n";
        break;

    case G_IO_STATUS_NORMAL:
        std::cout << "G_IO_STATUS_NORMAL\n";
        break;

    case G_IO_STATUS_EOF:
        std::cout << "G_IO_STATUS_EOF\n";
        break;

    case G_IO_STATUS_AGAIN:
        std::cout << "G_IO_STATUS_AGAIN\n";
        break;
    }
}


#if 0 // defined but not used
static void debugPrintCondition(GIOCondition cond)
{
    if (cond & G_IO_IN)
    {
        std::cout << "  G_IO_IN\n";
    }

    if (cond & G_IO_OUT)
    {
        std::cout << "  G_IO_OUT\n";
    }

    if (cond & G_IO_PRI)
    {
        std::cout << "  G_IO_PRI\n";
    }

    if (cond & G_IO_ERR)
    {
        std::cout << "  G_IO_ERR\n";
    }

    if (cond & G_IO_HUP)
    {
        std::cout << "  G_IO_HUP\n";
    }

    if (cond & G_IO_NVAL)
    {
        std::cout << "  G_IO_NVAL\n";
    }
}
#endif

/*========================================================================
  server_callback ()

  This call back function is invoked from the GTK main loop whenever a client
  socket has sent something to send to gpsim. This callback will only be
  invoked for those clients that have already connected to gpsim. Those
  connected clients have associated with them a 'SocketBase' object. A pointer
  to that object is passed in the server_callback parameters.

  This function essentially will respond to the messages that clients send
  in much the same way the BSD socket command recv() does. Which is to say,
  the data that the clients send is read from the socket. The actual parsing
  of the data is done by the routine 'SocketBase::Service()'.

 */

static gboolean server_callback(GIOChannel *channel,
                                GIOCondition condition,
                                void *pSocketBase)
{
    SocketBase *s = static_cast<SocketBase *>(pSocketBase);

    if (condition & G_IO_HUP)
    {
        std::cout << "client has gone away\n";
        GError *err = nullptr;
        GIOStatus stat = g_io_channel_shutdown(channel, TRUE, &err);
        std::cout << "channel status " << std::hex << stat << "  " ;
        debugPrintChannelStatus(stat);
        delete s;
        return FALSE;
    }

    if (condition & G_IO_IN)
    {
        gsize bytes_read = 0;
        s->packet->prepare();
        memset(s->packet->rxBuff(), 0, 256);
        GError *err = nullptr;
        gsize b;
#if !defined _WIN32 || defined _DEBUG
        g_io_channel_set_flags(channel, G_IO_FLAG_NONBLOCK, &err);
#endif
        g_io_channel_read_chars(channel,
                                s->packet->rxBuff(),
                                s->packet->rxSize(),
                                &b,
                                &err);
        bytes_read = b;
        s->packet->rxAdvance(bytes_read);

        if (err)
        {
            std::cout << "GError:" << err->message << '\n';
        }

        if (bytes_read)
        {
            if (get_interface().bSimulating())
            {
                std::cout << "setting a socket break point because sim is running \n";
                get_bp().set_socket_break();

            }
            else
            {
                s->Service();
            }

        }
        else
        {
            return FALSE;
        }

        return TRUE;
    }

    return FALSE;
}


/*========================================================================

  server_accept( )

  This call back function is invoked from the GTK loop whenever a client
  is attempting to establish a socket connection to gpsim. The purpose
  of this function is to accept a socket connection and to create a GLIB
  GIOChannel associated with it. This GIOChannel will listen to the client
  socket and will invoke the server_callback() callback whenever the client
  sends data. In addition, a new 'SocketBase' object will be created when
  the connection is accepted. This SocketBase object wraps the BSD socket
  and provides additional support for simplifying the socket communication.

 */

static gboolean sink_server_accept(GIOChannel *channel, GIOCondition /* condition */, void *d)
{
    Socket *s = static_cast<Socket *>(d);
    std::cout << " SourceSink accepting new client connect\n";
    SocketBase *client = s->Accept();

    if (!client)
    {
        return FALSE;
    }

    GIOChannel *new_channel = g_io_channel_unix_new(client->getSocket());
    GIOCondition new_condition = (GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_ERR);
    GError *err = nullptr;
    g_io_channel_set_encoding(channel, nullptr, &err);
#if !defined _WIN32 || defined _DEBUG
    //stat = g_io_channel_set_flags (channel, G_IO_FLAG_SET_MASK, &err);
    g_io_channel_set_flags(channel, G_IO_FLAG_NONBLOCK, &err);
#endif
    g_io_add_watch(new_channel,
                   new_condition,
                   server_callback,
                   (void*)client);
    return TRUE;
}


static gboolean source_server_accept(GIOChannel * /* channel */, GIOCondition /* condition */, void *d)
{
    Socket *s = static_cast<Socket *>(d);
    std::cout << " SourceServer accepting new client connect\n";
    SocketBase *client = s->Accept();
    std::cout << " SourceServer accepted connection\n";

    if (!client)
    {
        return FALSE;
    }

    int bytes = recv(client->getSocket(),
                     client->packet->rxBuff(),
                     client->packet->rxSize(), 0);
    std::cout << " SourceServer received data" << client->packet->rxBuff()
              << '\n';

    if (bytes  == -1)
    {
        perror("recv");
        exit_gpsim(1);
    }

    client->packet->rxAdvance(bytes);
    client->Service();
    std::cout << " SourceServer serviced client\n";
    // FIXME: SocketBase *client goes out of scope almost certainly leaking resources.
    return TRUE;
}


void start_server()
{
    //  TestInt32Array *test = new TestInt32Array("test",16);
    //  symbol_table.add(test);
    std::cout << "starting server....\n";
#ifdef _WIN32

    if (!winsockets_init())
    {
        fprintf(stderr, "Could not find a usable WinSock DLL, sockets are disabled.\n");
        server_started = false;
        return;
    }

    server_started = true;
#endif
    // Create the Sink and Source servers
    static Socket sinkServer;
    sinkServer.init(SIM_SINK_PORT);
    sinkServer.AssignChannel(sink_server_accept);
    get_interface().add_interface(new SocketInterface(&sinkServer));
    static Socket sourceServer;
    sourceServer.init(SIM_SOURCE_PORT);
    sourceServer.AssignChannel(source_server_accept);
    std::cout << " started server\n";
}


void stop_server()
{
#ifdef _WIN32

    if (server_started)
    {
        WSACleanup();
    }

#endif
}


#endif   // if USE_THREADS

