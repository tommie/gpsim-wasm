#ifndef GUI_GUI_INTERFACE_H_
#define GUI_GUI_INTERFACE_H_

class GUI_Processor;

void gui_new_processor(unsigned int pic_id);
void gui_new_source(unsigned int pic_id);

void update_register(unsigned int reg_number);
void update_program_memory(GUI_Processor *gp, unsigned int reg_number);

#endif // GUI_GUI_INTERFACE_H_
