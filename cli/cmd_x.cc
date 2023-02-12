/*
   Copyright (C) 1999 T. Scott Dattalo

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

#include <iostream>
#include <string>

#include "cmd_x.h"
#include "cmd_dump.h"
#include "misc.h"

#include "../src/expr.h"
#include "../src/processor.h"
#include "../src/registers.h"
#include "../src/ui.h"
#include "../src/value.h"

cmd_x c_x;

cmd_reg c_reg;

static cmd_options cmd_x_options[] =
{
    {nullptr, 0, 0}
};
static cmd_options cmd_reg_options[] =
{
    {nullptr, 0, 0}
};


cmd_x::cmd_x()
    : command("x", nullptr)
{
    brief_doc = "[deprecated] examine and/or modify memory";
    long_doc = "\nx examine command -- deprecated\n"
               "\tInstead of the using a special command to examine and modify\n"
               "\tvariables, it's possible to directly access them using gpsim's\n"
               "\texpression parsing. For example, to examine a variable:\n"
               "gpsim> my_variable\n"
               "my_variable [0x27] = 0x00 = 0b00000000\n"
               "\tTo modify a variable\n"
               "gpsim> my_variable = 10\n"
               "\tIt's also possible to assign the value of register to another\n"
               "gpsim> my_variable = porta\n"
               "\tOr to assign the results of an expression:\n"
               "gpsim> my_variable = (porta ^ portc) & 0x0f\n";
    /*
    long_doc = string ("\nx [file_register] [new_value]\n\
    \toptions:\n\
    \t\tfile_register - ram location to be examined or modified.\n\
    \t\tnew_value - the new value written to the file_register.\n\
    \t\tif no options are specified, then the entire contents\n\
    \t\tof the file registers will be displayed (dump).\n\
    ");
    */
    op = cmd_x_options;
}

cmd_reg::cmd_reg()
    : command("reg", nullptr)
{
    brief_doc = "examine and/or modify register memory";
    long_doc = "\nreg examine/modify register memory\n"
		"    reg(SRC)[= EXPRESSION]\n\n"
		"      SRC is an expression which evaluates to the target\n"
		"        register address. The expression can be or contain\n"
		"	 integers, register names (address of register is\n"
		"        used), REGnnn's(nnn, a HEX memory address, is used),\n"
		"	 and arithmetic operators such as + - & ^.\n\n"
		"      EXPRESSION is an expression which evaluates to the\n"
		"	 integer which is placed in SRC memory location.\n"
		"	 EXPRESSION uses the same elements as SRC except \n"
		"	 register names (including REGnnn) return their\n"
		"        contents.\n\n"
		"    Some examples:\n"
		"        **gpsim> reg(0xf92)\n"
		"        trisa[0xf92] = 0x7f\n\n"
		"        **gpsim> reg(REGf92)\n"
		"        trisa[0xf92] = 0x7f\n\n"
		"        reg(trisa) = trisa & $f0\n"
		"        trisa[0xf92] = 0x70 was 0x7f\n\n"
		"        **gpsim> reg(0x20) = trisb\n"
		"        REG020[0x20] = 0xff was 0x0\n"
		"        **gpsim> reg(0x20+1) = REG020\n"
		"        REG021[0x21] = 0xff was 0x0\n\n"

		;
    op = cmd_reg_options;
}

void cmd_x::x()
{
    dump.dump(cmd_dump::DUMP_RAM);

    if (GetActiveCPU())
    {
        GetActiveCPU()->dump_registers();
    }
}

/*
 * This function produces the output.
 * if pExpr == nullptr it outputs the value of the specified register(reg)
 * otherwise it evaluates pExpr, updates the specified register and
 * outputs the result
 */
void cmd_x::x(int reg, Expression *pExpr)
{
    Register *pReg = nullptr;

    if (!GetActiveCPU())
    {
        return;
    }

    if (reg < 0 || (reg >= (int)GetActiveCPU()->register_memory_size()) ||
	(pReg = GetActiveCPU()->registers[reg]) == nullptr)
    {
        GetUserInterface().DisplayMessage("bad file register 0x%x\n", reg);
        return;
    }

    RegisterValue rvCurrent(pReg->getRV());

    if (pExpr == nullptr)
    {
        GetUserInterface().DisplayMessage("%s[0x%x] = 0x%x\n",
                                          pReg->name().c_str(), reg, rvCurrent.data);
    }
    else
    {
	unsigned int val;
        if (int_from_expression(pExpr, val))
        {
	    unsigned int new_value = val & GetActiveCPU()->register_mask();
            RegisterValue RV(new_value, 0);
            pReg->putRV(RV);
            // Notify listeners
            pReg->update();
            // Display new value
            GetUserInterface().DisplayMessage("%s[0x%x] = 0x%x ",
                                              pReg->name().c_str(), reg, new_value);
            // Display old value
            GetUserInterface().DisplayMessage("was 0x%x\n",
                                              ((int64_t)rvCurrent.get() &
                                               GetActiveCPU()->register_mask()));
        }
        else
        {
            GetUserInterface().DisplayMessage("Error evaluating the expression\n");
        }

        delete pExpr;
    }
}


void cmd_x::x(char *reg_name)
{
    std::cout << "this command is deprecated. "
              << "Type '" << reg_name << "' at the command line to display the contents of a register.\n";
    // get_symbol_table().dump_one(reg_name);
}


void cmd_x::x(char * /* reg_name */, int /* val */)
{
    std::cout << "this command is deprecated. use: \n  symbol_name = value\n\ninstead\n";
    //  update_symbol_value(reg_name,val);
}


/*
 * this function is called from the parser.
 * it evaluates the first expression and passes the result
 * to another function to evaluate the second expression and
 * output the result
 */
void cmd_x::x(Expression *expr, Expression *pExpr)
{
    unsigned int val;

    expr->get_address = true;
    try {
        if(int_from_expression(expr, val))
        {
	    x(val, pExpr);
        }
    }

    catch (const Error &err)
    {
        std::cout << "ERROR:" << err.what() << std::endl;
    }

    delete expr;
}
/*
 * Evaluate an expression to an integer value.
 * if the expression is a register label, return the register address
 * if get_address == true, otherwise return register value.
 * return true on success
 */
bool cmd_x::int_from_expression(Expression *pExpr, unsigned int &val)
{
    Value *pValue = nullptr;
    Integer *pInt = nullptr;



    if ( (typeid(LiteralString) == typeid(*pExpr) &&
        !((LiteralString*)pExpr)->toString().empty() ))
    {
            std::string sName(((LiteralString*)pExpr)->toString());
            if (sscanf(sName.c_str(), "REG%x", &val) > 00)
            {
		if (pExpr->get_address)
		    return true;
		else
		{
    		    Register *pReg = GetActiveCPU()->registers[val];
		    if (pReg)
		    {
		        val = (unsigned int)pReg->get_value();
			return true;
		    }

                    GetUserInterface().DisplayMessage(
                        "Error: %s is not REGnnn\n", sName.c_str());
	            return false;
		}

            }
	    else
	    {
                GetUserInterface().DisplayMessage(
                    "Error: %s is not REGnnn\n", sName.c_str());
	        return false;
	    }
    }
/*
    else if (typeid(LiteralInteger) == typeid(*pExpr) &&
		(pValue = toValue(pExpr)) &&
		(pInt = dynamic_cast<Integer*>(pValue)))
    {
	    val = pInt->getVal();

	    delete pValue;
	    return true;
    }
*/
    else if ((pValue = pExpr->evaluate()) &&
             (pInt = dynamic_cast<Integer*>(pValue))
            )
    {
        val = (unsigned int)pInt->getVal();
        delete pValue;
        return true;
    }
    else
    {
        GetUserInterface().DisplayMessage(
            "Error: the expression did not evaluate to on integer\n");
    }
    return false;
}
