// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "fs.h"
#include "file.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

#define K 3

static void consputc(int);

static int panicked = 0;

char clip_board[K];
int clip_board_size;

static struct
{
  struct spinlock lock;
  int locking;
} cons;

static void
printint(int xx, int base, int sign)
{
  static char digits[] = "0123456789abcdef";
  char buf[16];
  int i;
  uint x;

  if (sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do
  {
    buf[i++] = digits[x % base];
  } while ((x /= base) != 0);

  if (sign)
    buf[i++] = '-';

  while (--i >= 0)
    consputc(buf[i]);
}
// PAGEBREAK: 50

// Print to the console. only understands %d, %x, %p, %s.
void cprintf(char *fmt, ...)
{
  int i, c, locking;
  uint *argp;
  char *s;

  locking = cons.locking;
  if (locking)
    acquire(&cons.lock);

  if (fmt == 0)
    panic("null fmt");

  argp = (uint *)(void *)(&fmt + 1);
  for (i = 0; (c = fmt[i] & 0xff) != 0; i++)
  {
    if (c != '%')
    {
      consputc(c);
      continue;
    }
    c = fmt[++i] & 0xff;
    if (c == 0)
      break;
    switch (c)
    {
    case 'd':
      printint(*argp++, 10, 1);
      break;
    case 'x':
    case 'p':
      printint(*argp++, 16, 0);
      break;
    case 's':
      if ((s = (char *)*argp++) == 0)
        s = "(null)";
      for (; *s; s++)
        consputc(*s);
      break;
    case '%':
      consputc('%');
      break;
    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
      break;
    }
  }

  if (locking)
    release(&cons.lock);
}

void panic(char *s)
{
  int i;
  uint pcs[10];

  cli();
  cons.locking = 0;
  // use lapiccpunum so that we can call panic from mycpu()
  cprintf("lapicid %d: panic: ", lapicid());
  cprintf(s);
  cprintf("\n");
  getcallerpcs(&s, pcs);
  for (i = 0; i < 10; i++)
    cprintf(" %p", pcs[i]);
  panicked = 1; // freeze other CPU
  for (;;)
    ;
}

// PAGEBREAK: 50
#define BACKSPACE 0x100
#define CRTPORT 0x3d4
static ushort *crt = (ushort *)P2V(0xb8000); // CGA memory

static void
cgaputc(int c)
{
  int pos;

  // Cursor position: col + 80*row.
  outb(CRTPORT, 14);
  pos = inb(CRTPORT + 1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT + 1);

  if (c == '\n')
    pos += 80 - pos % 80;
  else if (c == BACKSPACE)
  {
    if (pos > 0)
      --pos;
  }
  else
    crt[pos++] = (c & 0xff) | 0x0700; // black on white

  if (pos < 0 || pos > 25 * 80)
    panic("pos under/overflow");

  if ((pos / 80) >= 24)
  { // Scroll up.
    memmove(crt, crt + 80, sizeof(crt[0]) * 23 * 80);
    pos -= 80;
    memset(crt + pos, 0, sizeof(crt[0]) * (24 * 80 - pos));
  }

  outb(CRTPORT, 14);
  outb(CRTPORT + 1, pos >> 8);
  outb(CRTPORT, 15);
  outb(CRTPORT + 1, pos);
  crt[pos] = ' ' | 0x0700;
}

void consputc(int c)
{
  if (panicked)
  {
    cli();
    for (;;)
      ;
  }

  if (c == BACKSPACE)
  {
    uartputc('\b');
    uartputc(' ');
    uartputc('\b');
  }
  else
    uartputc(c);
  cgaputc(c);
}

#define INPUT_BUF 128
struct
{
  char buf[INPUT_BUF];
  uint r; // Read index
  uint w; // Write index
  uint e; // Edit index
} input;

char new_command[INPUT_BUF];
char command_history[10][INPUT_BUF];

void add_command(char command[INPUT_BUF], int length)
{
  for (int i = 0; i < 9; i++)
  {
    for (int j = 0; j < INPUT_BUF; j++)
    {
      command_history[i][j] = '\0';
    }
    for (int j = 0; j < INPUT_BUF; j++)
    {
      if (command_history[i + 1][j] == '\0')
      {
        break;
      }
      command_history[i][j] = command_history[i + 1][j];
    }
  }

  for (int j = 0; j < INPUT_BUF; j++)
  {
    command_history[9][j] = command[j];
  }
  for (int j = length; j < INPUT_BUF; j++)
  {
    command_history[9][j] = '\0';
  }
}

#define C(x) ((x) - '@') // Control-x

void consoleintr(int (*getc)(void))
{
  int c, doprocdump = 0;
  char temp[INPUT_BUF];
	
  acquire(&cons.lock);
  while ((c = getc()) >= 0)
  {
    switch (c)
    {
    case C('P'): // Process listing.
      // procdump() locks cons.lock indirectly; invoke later
      doprocdump = 1;
      break;
    case C('U'): // Kill line.
      while (input.e != input.w &&
             input.buf[(input.e - 1) % INPUT_BUF] != '\n')
      {
        input.e--;
        consputc(BACKSPACE);
      }
      break;
    case ('C'):
      if (input.e - input.w < K)
      {
        clip_board_size = input.e - input.w;
        for (int i = clip_board_size; i > 0; i--)
        {
          clip_board[clip_board_size - i] = input.buf[input.e - i];
        }
        int z = clip_board_size;
        while (clip_board[z] != '\0') // Flushing rest of clipboard
        {
          clip_board[z] = '\0';
          z++;
        }
      }
      else
      {
        clip_board_size = K;
        for (int i = K; i > 0; i--)
        {
          clip_board[K - i] = input.buf[input.e - i];
        }
      }
      break;
    case ('X'):
      if (input.e - input.w < K)
      {
        clip_board_size = input.e - input.w;
        for (int i = clip_board_size; i > 0; i--)
        {
          clip_board[clip_board_size - i] = input.buf[input.e - i];
        }

        int z = clip_board_size;
        while (clip_board[z] != '\0') // Flushing rest of clipboard
        {
          clip_board[z] = '\0';
          z++;
        }
        for (int i = 0; i < clip_board_size; i++)
        {
          input.e--;
          consputc(BACKSPACE);
        }
      }
      else
      {
        clip_board_size = K;
        for (int i = K; i > 0; i--)
        {
          clip_board[K - i] = input.buf[input.e - i];
        }
        for (int i = 0; i < clip_board_size; i++)
        {
          consputc(BACKSPACE);
          input.e--;
        }
      }
      break;
    case ('V'):
      for (int i = 0; i < clip_board_size; i++)
      {
        consputc(clip_board[i]);
        input.buf[input.e] = clip_board[i];
        input.e += 1;
      }
      break;
    case C('E'):
      // eeeeeeeeeeee
      for (int i = 0; i < INPUT_BUF; i++)
      {
        temp[i] = '\0';
      }
      int j = 0;
      for (int i = input.w; i < input.e; i++)
      {
        if (input.buf[i] >= '0' && input.buf[i] <= '9')
        {
          if (input.buf[i] + K <= '9')
          {
            temp[j] = input.buf[i] + K;
            j++;
          }
          else
          {
            temp[j] = 'A' + (input.buf[i] + K - '9' - 1);
            j++;
          }
        }
        else if (input.buf[i] >= 'a' && input.buf[i] <= 'z')
        {
          temp[j] = input.buf[i] + ('A' - 'a');
          j++;
        }
        else if (input.buf[i] >= 'A' && input.buf[i] <= 'Z')
        {
          temp[j] = input.buf[i] + ('a' - 'A');
          j++;
        }
      }
      while (input.e != input.w &&
             input.buf[(input.e - 1) % INPUT_BUF] != '\n')
      {
        input.e--;
        input.buf[input.e] = '\0';
        consputc(BACKSPACE);
      }
      for (int i = 0; temp[i] != '\0'; i++)
      {
        input.buf[input.e] = temp[i];
        consputc(temp[i]);
        input.e += 1;
      }
      break;

    case ('\t'):
    {
      // for (int i = 0; i < 10; i++)
      // {
      //   for (int j = 0; command_history[i][j] != '\0'; j++)
      //   {
      //     consputc(command_history[i][j]);
      //   }
      // }

      int flag = 1;
      int j = input.w;
      int i = 9;
      int leng = 0;
      char buf[INPUT_BUF];
      for (i = input.w; i < input.e; i++)
      {
        buf[i - input.w] = input.buf[i];
        leng++;
      }
      for (i = 9; i >= 0; i--)
      {
        flag = 1;
        for (j = 0; j < leng; j++)
        {
          if (command_history[i][j] == buf[j])
          {
            continue;
          }
          else
          {
            flag = 0;
            break;
          }
        }
        if (flag)
        {
          break;
        }
      }
      if (flag)
      {
        while (input.e != input.w &&
               input.buf[(input.e - 1) % INPUT_BUF] != '\n')
        {
          input.e--;
          consputc(BACKSPACE);
        }
        for (j = 0; j < INPUT_BUF; j++)
        {
          input.buf[j] = '\0';
        }
        input.e = input.w;

        for (j = 0; command_history[i][j] != '\0' && command_history[i][j] != '\n'; j++)
        {
          input.buf[input.e] = command_history[i][j];
          consputc(command_history[i][j]);
          input.e += 1;
        }
      }
      break;
    }
    case C('H'):
    case '\x7f': // Backspace
      if (input.e != input.w)
      {
        input.e--;
        consputc(BACKSPACE);
      }
      break;
    default:
      if (c != 0 && input.e - input.r < INPUT_BUF)
      {
        c = (c == '\r') ? '\n' : c;
        input.buf[input.e++ % INPUT_BUF] = c;
        consputc(c);
        if (c == '\n' || c == C('D') || input.e == input.r + INPUT_BUF)
        {
          int leng = 0;
          for (int i = input.w; input.buf[i] != '\0' && input.buf[i] != '\n'; i++)
          {
            new_command[i - input.w] = input.buf[i];
            leng++;
          }
          add_command(new_command, leng);

          input.w = input.e;
          wakeup(&input.r);
        }
      }
      break;
    }
  }
  release(&cons.lock);
  if (doprocdump)
  {
    procdump(); // now call procdump() wo. cons.lock held
  }
}

int consoleread(struct inode *ip, char *dst, int n)
{
  uint target;
  int c;

  iunlock(ip);
  target = n;
  acquire(&cons.lock);
  while (n > 0)
  {
    while (input.r == input.w)
    {
      if (myproc()->killed)
      {
        release(&cons.lock);
        ilock(ip);
        return -1;
      }
      sleep(&input.r, &cons.lock);
    }
    c = input.buf[input.r++ % INPUT_BUF];
    if (c == C('D'))
    { // EOF
      if (n < target)
      {
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        input.r--;
      }
      break;
    }
    *dst++ = c;
    --n;
    if (c == '\n')
      break;
  }
  release(&cons.lock);
  ilock(ip);

  return target - n;
}

int consolewrite(struct inode *ip, char *buf, int n)
{
  int i;

  iunlock(ip);
  acquire(&cons.lock);
  for (i = 0; i < n; i++)
    consputc(buf[i] & 0xff);
  release(&cons.lock);
  ilock(ip);

  return n;
}

void consoleinit(void)
{
  initlock(&cons.lock, "console");

  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read = consoleread;
  cons.locking = 1;

  ioapicenable(IRQ_KBD, 0);
}
