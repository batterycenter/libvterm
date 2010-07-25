#include "vterm_internal.h"

#define UNICODE_INVALID 0xFFFD

#ifdef DEBUG
# define DEBUG_PRINT_UTF8
#endif

static int decode_utf8(VTermEncoding *enc, uint32_t cp[], int *cpi, const char bytes[], size_t *pos, size_t len)
{
  // number of bytes remaining in this codepoint
  int bytes_remaining = 0;
  // number of bytes total in this codepoint once it's finished
  // (for detecting overlongs)
  int bytes_total     = 0;

  int this_cp;

#ifdef DEBUG_PRINT_UTF8
  printf("BEGIN UTF-8\n");
#endif

  for( ; *pos < len; (*pos)++) {
    unsigned char c = bytes[*pos];

#ifdef DEBUG_PRINT_UTF8
    printf(" pos=%zd c=%02x rem=%d\n", *pos, c, bytes_remaining);
#endif

    if(c < 0x20)
      return 0;

    else if(c >= 0x20 && c < 0x80) {
      if(bytes_remaining)
        cp[(*cpi)++] = UNICODE_INVALID;

      cp[(*cpi)++] = c;
#ifdef DEBUG_PRINT_UTF8
      printf(" UTF-8 char: U+%04x\n", c);
#endif
      bytes_remaining = 0;
    }

    else if(c >= 0x80 && c < 0xc0) {
      if(!bytes_remaining) {
        cp[(*cpi)++] = UNICODE_INVALID;
        continue;
      }

      this_cp <<= 6;
      this_cp |= c & 0x3f;
      bytes_remaining--;

      if(!bytes_remaining) {
#ifdef DEBUG_PRINT_UTF8
        printf(" UTF-8 raw char U+%04x len=%d ", this_cp, bytes_total);
#endif
        // Check for overlong sequences
        switch(bytes_total) {
        case 2:
          if(this_cp <  0x0080) this_cp = UNICODE_INVALID; break;
        case 3:
          if(this_cp <  0x0800) this_cp = UNICODE_INVALID; break;
        case 4:
          if(this_cp < 0x10000) this_cp = UNICODE_INVALID; break;
        case 5:
          if(this_cp < 0x200000) this_cp = UNICODE_INVALID; break;
        case 6:
          if(this_cp < 0x4000000) this_cp = UNICODE_INVALID; break;
        }
        // Now look for plain invalid ones
        if((this_cp >= 0xD800 && this_cp <= 0xDFFF) ||
           this_cp == 0xFFFE ||
           this_cp == 0xFFFF)
          this_cp = UNICODE_INVALID;
#ifdef DEBUG_PRINT_UTF8
        printf(" char: U+%04x\n", this_cp);
#endif
        cp[(*cpi)++] = this_cp;
      }
    }

    else if(c >= 0xc0 && c < 0xe0) {
      if(bytes_remaining)
        cp[(*cpi)++] = UNICODE_INVALID;

      if(len - *pos < 2)
        return 1;

      this_cp = c & 0x1f;
      bytes_total = 2;
      bytes_remaining = 1;
    }

    else if(c >= 0xe0 && c < 0xf0) {
      if(bytes_remaining)
        cp[(*cpi)++] = UNICODE_INVALID;

      if(len - *pos < 3)
        return 1;

      this_cp = c & 0x0f;
      bytes_total = 3;
      bytes_remaining = 2;
    }

    else if(c >= 0xf0 && c < 0xf8) {
      if(bytes_remaining)
        cp[(*cpi)++] = UNICODE_INVALID;

      if(len - *pos < 4)
        return 1;

      this_cp = c & 0x07;
      bytes_total = 4;
      bytes_remaining = 3;
    }

    else if(c >= 0xf8 && c < 0xfc) {
      if(bytes_remaining)
        cp[(*cpi)++] = UNICODE_INVALID;

      if(len - *pos < 5)
        return 1;

      this_cp = c & 0x03;
      bytes_total = 5;
      bytes_remaining = 4;
    }

    else if(c >= 0xfc && c < 0xfe) {
      if(bytes_remaining)
        cp[(*cpi)++] = UNICODE_INVALID;

      if(len - *pos < 6)
        return 1;

      this_cp = c & 0x01;
      bytes_total = 6;
      bytes_remaining = 5;
    }

    else {
      cp[(*cpi)++] = UNICODE_INVALID;
    }
  }

  return 1;
}

VTermEncoding encoding_utf8 = {
  .decode = &decode_utf8,
};

static int decode_usascii(VTermEncoding *enc, uint32_t cp[], int *cpi, const char bytes[], size_t *pos, size_t len)
{
  for(; *pos < len; (*pos)++) {
    unsigned char c = bytes[*pos];

    if(c < 0x20 || (c >= 0x80 && c < 0xa0))
      return 0;

    cp[(*cpi)++] = c;
  }

  return 1;
}

VTermEncoding encoding_usascii = {
  .decode = &decode_usascii,
};