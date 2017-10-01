#include "start.h"
#include "utility.h"


#define switch_ds 	__asm__("mov eax,0X48\n push ds\n push eax\n pop ds\n");
#define switch_ds_back __asm__("pop ds");

// Write a single character out to the screen.
void monitor_put(char c);
// Clear the screen to all black.
void monitor_clear();
// Output a null-terminated ASCII string to the monitor.
void monitor_write(char *c);
void monitor_write_at(char *c, int locX, int locY);

u16int cursorLocation=0;
int cursor_x=0, cursor_y=0;

// Updates the hardware cursor.
static void move_cursor()
{
   // The screen is 80 characters wide...
   cursorLocation = cursor_y * 160 + cursor_x;
//   outb(0x3D4, 14);                  // Tell the VGA board we are setting the high cursor byte.
//   outb(0x3D5, cursorLocation >> 8); // Send the high cursor byte.
//   outb(0x3D4, 15);                  // Tell the VGA board we are setting the low cursor byte.
//   outb(0x3D5, cursorLocation);      // Send the low cursor byte.
} 

// Scrolls the text on the screen up by one line.
static void scroll()
{
volatile char *video_memory = (volatile char*)0;
   // Get a space character with the default colour attributes.
   u8int attributeByte = (0 /*black*/ << 4) | (15 /*white*/ & 0x0F);
   u16int blank = 0x20 /* space */ | (attributeByte << 8);

   // Row 25 is the end, this means we need to scroll up
   if(cursor_y >= 25)
   {
       // Move the current text chunk that makes up the screen
       // back in the buffer by a line
       int i;
switch_ds
       for (i = 0*80; i < 24*80; i++)
       {
           video_memory[i] = video_memory[i+80];
       }
       // The last line should now be blank. Do this by writing
       // 80 spaces to it.
       for (i = 24*80; i < 25*80; i++)
       {
           video_memory[i] = blank;
       }
switch_ds_back
       // The cursor should now be on the last line.
       cursor_y = 24;
   }
} 

// Writes a single character out to the screen.
void monitor_put(char c)
{
volatile char *video_memory = (volatile char*)0;
   // The background colour is black (0), the foreground is white (15).
//   u8int backColour = 0;
//   u8int foreColour = 15;
   // The attribute byte is made up of two nibbles - the lower being the
   // foreground colour, and the upper the background colour.
   u8int  attributeByte = 7; //(backColour << 4) | (foreColour & 0x0F);
   u16int location;
   // Handle a backspace, by moving the cursor back one space
   if (c == 0x08 && cursor_x)
   {
       cursor_x--;
   }

   // Handle a tab by increasing the cursor's X, but only to a point
   // where it is divisible by 8.
   else if (c == 0x09)
   {
       cursor_x = (cursor_x+8) & ~(8-1);
   }

   // Handle carriage return
   else if (c == '\r')
   {
       cursor_x = 0;
   }

   // Handle newline by moving cursor back to left and increasing the row
   else if (c == '\n')
   {
       cursor_x = 0;
       cursor_y++;
   }
   // Handle any other printable character.
   else if(c >= ' ')
   {
       location = (cursor_y*160 + (cursor_x*2));
switch_ds
		video_memory[location++] = c;
		video_memory[location] = attributeByte;
switch_ds_back
       cursor_x++;
   }

   // Check if we need to insert a new line because we have reached the end
   // of the screen.
   if (cursor_x >= 160)
   {
       cursor_x = 0;
       cursor_y ++;
   }

} 

// Clears the screen, by copying lots of spaces to the framebuffer.
void monitor_clear()
{
   // Make an attribute byte for the default colours
volatile char *video_memory = (volatile char*)0;
   //u8int attributeByte = (0 /*black*/ << 4) | (15 /*white*/ & 0x0F);
   char blank = 0x20;
   char attrib = 0x07;

   int i;
switch_ds
   for (i = 0; i < 160*25; i+=2)
   {
       video_memory[i] = blank;
	video_memory[i+1] = attrib;
   }
switch_ds_back
   // Move the hardware cursor back to the start.
   cursor_x = 0;
   cursor_y = 0;
   move_cursor();
} 

// Outputs a null-terminated ASCII string to the monitor.
void monitor_write(char *c)
{
   int i = 0;
   while (c[i])
   {
       monitor_put(c[i++]);
   }
   // Scroll the screen if needed.
   scroll();
   // Move the hardware cursor.
   move_cursor();
} 

void monitor_write_at(char *c, int locX, int locY)
{

	cursor_x = locX;
	cursor_y = locY;
	monitor_write(c);
}
