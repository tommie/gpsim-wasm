#ifndef SRC_ICD_H_
#define SRC_ICD_H_

//#include "gpsim_classes.h"
#include "exports.h"

#define ID_LOC_ADDR      0x2000
#define DEVICE_ID_ADDR   0x2006
#define CONF_WORD_ADDR   0x2007
#define ID_LOC_LEN       8

#define MAX_PROG_MEM_SIZE 0x2000            /* 16F877 has 8K word flash */
#define MAX_MEM_SIZE (MAX_PROG_MEM_SIZE + 0x200)    /* + ID location + EEPROM */
#define UNINITIALIZED -1               /* Used to flag that a memory location isn't used */


LIBGPSIM_EXPORT bool get_use_icd();
int icd_connect(const char *dev);
int icd_reconnect();
int icd_disconnect();
int icd_detected();
const char *icd_version();
const char *icd_target();
float icd_vdd();
float icd_vpp();
int icd_reset();

int icd_erase();
int icd_prog(int *mem);

int icd_has_debug_module();
int icd_step();
int icd_run();
int icd_halt();
int icd_stopped();

int icd_get_state();
int icd_get_file();

int icd_set_break(int address);
int icd_clear_break();

int icd_read_file(int address);
int icd_write_file(int address, int data);
void icd_set_bulk(int flag);

int icd_read_eeprom(int address);
int icd_write_eeprom(int address, int data);

/*
class icd_Register : public file_register
{
        public:
          file_register *replaced;   // A pointer to the register that this break replaces
          int is_stale;

      icd_Register();

  virtual REGISTER_TYPES isa(void) {return replaced->isa();};
    virtual char *name(void) {return replaced->name();};

      virtual void put_value(unsigned int new_value);
       virtual void put(unsigned int new_value);
          virtual unsigned int get_value(void);
    virtual unsigned int get(void);
};

class icd_statusReg : public Status_register
{
        public:
          Status_register *replaced;   // A pointer to the register that this break replaces
          int is_stale;

      icd_statusReg();

  virtual REGISTER_TYPES isa(void) {return replaced->isa();};
    virtual char *name(void) {return replaced->name();};

      virtual void put_value(unsigned int new_value);
       virtual void put(unsigned int new_value);
          virtual unsigned int get_value(void);
    virtual unsigned int get(void);
};

class icd_WREG : public WREG
{
        public:
          WREG *replaced;   // A pointer to the register that this break replaces
          int is_stale;

      icd_WREG();

  virtual REGISTER_TYPES isa(void) {return replaced->isa();};
    virtual char *name(void) {return replaced->name();};

      virtual void put_value(unsigned int new_value);
       virtual void put(unsigned int new_value);
          virtual unsigned int get_value(void);
    virtual unsigned int get(void);
};

class icd_PC : public Program_Counter
{
        Program_Counter *replaced;
        int is_stale;

        icd_PC();

        virtual void put_value(unsigned int new_value);
        virtual unsigned int get_value(void);
};
*/

#endif


