
;
;SSD0323 driver for 16bit-core PICs
; Scott Dattalo
;

   EXTERN  SSD0323_Init
   EXTERN  LCD_RefreshDisplay
   EXTERN  LCD_RefreshEntireDisplay

   EXTERN  SSD_SetColumnRange
   EXTERN  SSD_SetRowRange

   EXTERN  CommandLoop
   EXTERN  SSD0323_Write
   EXTERN  SSD0323_WriteCmd
   EXTERN  CommandWaitForSPI
