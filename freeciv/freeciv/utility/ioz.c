/********************************************************************** 
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
***********************************************************************/

/********************************************************************** 
  An IO layer to support transparent compression/uncompression.
  (Currently only "required" functionality is supported.)

  There are various reasons for making this a full-blown module
  instead of just defining a few macros:
  
  - Ability to switch between compressed and uncompressed at run-time
    (zlib with compression level 0 saves uncompressed, but still with
    gzip header, so non-zlib server cannot read the savefile).
    
  - Flexibility to add other methods if desired (eg, bzip2, arbitrary
    external filter program, etc).

  FIXME: when zlib support _not_ included, should sanity check whether
  the first few bytes are gzip marker and complain if so.
**********************************************************************/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

#ifdef HAVE_BZLIB_H
#include <bzlib.h>
#endif

#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "shared.h"
#include "support.h"

#include "ioz.h"

#ifdef HAVE_LIBBZ2
struct bzip2_struct {
  BZFILE *file;
  FILE *plain;
  int error;
  int firstbyte;
  bool eof;
};
#endif

struct fz_FILE_s {
  enum fz_method method;
  char mode;
  union {
    FILE *plain;		/* FZ_PLAIN */
#ifdef HAVE_LIBZ
    gzFile zlib;		/* FZ_ZLIB */
#endif
#ifdef HAVE_LIBBZ2
    struct bzip2_struct bz2;
#endif
  } u;
};

/***************************************************************
  Open file for reading/writing, like fopen.
  Parameters compress_method and compress_level only apply
  for writing: for reading try to use the most appropriate
  available method.
  Returns NULL if there was a problem; check errno for details.
  (If errno is 0, and using FZ_ZLIB, probably had zlib error
  Z_MEM_ERROR.  Wishlist: better interface for errors?)
***************************************************************/
fz_FILE *fz_from_file(const char *filename, const char *in_mode,
		      enum fz_method method, int compress_level)
{
  fz_FILE *fp;
  char mode[64];

  if (!is_reg_file_for_access(filename, in_mode[0] == 'w')) {
    return NULL;
  }

  fp = (fz_FILE *)fc_malloc(sizeof(*fp));
  sz_strlcpy(mode, in_mode);

  if (mode[0] == 'w') {
    /* Writing: */
    fp->mode = 'w';

#ifndef HAVE_LIBZ
    if (method == FZ_ZLIB) {
      freelog(LOG_ERROR, "Not compiled with zlib support, reverting to plain.");
      method = FZ_PLAIN;
    }
#endif
#ifndef HAVE_LIBBZ2
    if (method == FZ_BZIP2) {
      freelog(LOG_ERROR, "Not compiled with bzib2 support, reverting to plain.");
      method = FZ_PLAIN;
    }
#endif

  } else {
    /* Reading: ignore specified method and try best: */
    fp->mode = 'r';
#ifdef HAVE_LIBBZ2
    /* Try to open as bzip2 file */
    method = FZ_BZIP2;
    sz_strlcat(mode,"b");
    fp->u.bz2.plain = fopen(filename, mode);
    if (fp->u.bz2.plain) {
      fp->u.bz2.file = BZ2_bzReadOpen(&fp->u.bz2.error, fp->u.bz2.plain, 1, 0,
                                      NULL, 0);
    }
    if (!fp->u.bz2.file) {
      if (fp->u.bz2.plain) {
        fclose(fp->u.bz2.plain);
      }
      free(fp);
      return NULL;
    } else {
      /* Try to read first byte out of stream so we can figure out if this
         really is bzip2 file or not. Store byte for later use */
      char tmp;
      int read_len;

      /* We put error to tmp variable when we don't want to overwrite
       * error already in fp->u.bz2.error. So calls to fz_ferror() or
       * fz_strerror() will later return what originally went wrong,
       * and not what happened in error recovery. */
      int tmp_err;

      read_len = BZ2_bzRead(&fp->u.bz2.error, fp->u.bz2.file, &tmp, 1);
      if (fp->u.bz2.error != BZ_DATA_ERROR_MAGIC) {
        /* bzip2 file */
        if (fp->u.bz2.error == BZ_STREAM_END) {
          /* We already reached end of file with our read of one byte */
          if (read_len == 0) {
            /* 0 byte file */
            fp->u.bz2.firstbyte = -1;
          } else {
            fp->u.bz2.firstbyte = tmp;
          }
          fp->u.bz2.eof = TRUE;
        } else if (fp->u.bz2.error != BZ_OK) {
          /* Read failed */
          BZ2_bzReadClose(&tmp_err, fp->u.bz2.file);
          fclose(fp->u.bz2.plain);
          free(fp);
          return NULL;
        } else {
          /* Read success and we can continue reading */
          fp->u.bz2.firstbyte = tmp;
          fp->u.bz2.eof = FALSE;
        }
        fp->method = FZ_BZIP2;
        return fp;
      }

      /* Not bzip2 file */
      BZ2_bzReadClose(&tmp_err, fp->u.bz2.file);
      fclose(fp->u.bz2.plain);
    }
#endif

#ifdef HAVE_LIBZ
    method = FZ_ZLIB;
#else
    method = FZ_PLAIN;
#endif
  }

  fp->method = method;

  switch (fp->method) {
#ifdef HAVE_LIBBZ2
  case FZ_BZIP2:
    /*  bz2 files are binary files, so we should add "b" to mode! */
    sz_strlcat(mode,"b");
    fp->u.bz2.plain = fopen(filename, mode);
    if (fp->u.bz2.plain) {
      /*  Open for read handled earlier */
      assert(mode[0] == 'w');
      fp->u.bz2.file = BZ2_bzWriteOpen(&fp->u.bz2.error, fp->u.bz2.plain,
                                       compress_level, 1, 15);
      if (fp->u.bz2.error != BZ_OK) {
        int tmp_err; /* See comments for similar variable
                      * near BZ2_bzReadOpen() */
        BZ2_bzWriteClose(&tmp_err, fp->u.bz2.file, 0, NULL, NULL);
        fp->u.bz2.file = NULL;
      }
    }
    if (!fp->u.bz2.file) {
      if (fp->u.bz2.plain) {
        fclose(fp->u.bz2.plain);
      }
      free(fp);
      fp = NULL;
    }
    break;
#endif
#ifdef HAVE_LIBZ
  case FZ_ZLIB:
    /*  gz files are binary files, so we should add "b" to mode! */
    sz_strlcat(mode,"b");
    if (mode[0] == 'w') {
      cat_snprintf(mode, sizeof(mode), "%d", compress_level);
    }
    fp->u.zlib = gzopen(filename, mode);
    if (!fp->u.zlib) {
      free(fp);
      fp = NULL;
    }
    break;
#endif
  case FZ_PLAIN:
    fp->u.plain = fopen(filename, mode);
    if (!fp->u.plain) {
      free(fp);
      fp = NULL;
    }
    break;
  default:
    /* Should never happen */
    die("Internal error: Bad fz_fromFile method: %d", fp->method);
  }
  return fp;
}

/***************************************************************
  Open uncompressed stream for reading/writing.
***************************************************************/
fz_FILE *fz_from_stream(FILE *stream)
{
  fz_FILE *fp;

  if (!stream) {
    return NULL;
  }

  fp = fc_malloc(sizeof(*fp));
  fp->method = FZ_PLAIN;
  fp->u.plain = stream;
  return fp;
}

/***************************************************************
  Close file, like fclose.
  Returns 0 on success, or non-zero for problems (but don't call
  fz_ferror in that case because the struct has already been
  free'd;  wishlist: better interface for errors?)
  (For FZ_PLAIN returns EOF and could check errno;
  for FZ_ZLIB: returns zlib error number; see zlib.h.)
***************************************************************/
int fz_fclose(fz_FILE *fp)
{
  int retval = 0;
  
  switch(fp->method) {
#ifdef HAVE_LIBBZ2
  case FZ_BZIP2:
    if(fp->mode == 'w') {
      BZ2_bzWriteClose(&fp->u.bz2.error, fp->u.bz2.file, 0, NULL, NULL);
    } else {
      BZ2_bzReadClose(&fp->u.bz2.error, fp->u.bz2.file);
    }
    if(fp->u.bz2.error == BZ_OK) {
      retval = 0;
    } else {
      retval = 1;
    }
    fclose(fp->u.bz2.plain);
    break;
#endif
#ifdef HAVE_LIBZ
  case FZ_ZLIB:
    retval = gzclose(fp->u.zlib);
    if (retval > 0) {
      retval = 0;		/* only negative Z values are errors */
    }
    break;
#endif
  case FZ_PLAIN:
    retval = fclose(fp->u.plain);
    break;
  default:
    /* Should never happen */
    die("Internal error: Bad fz_fclose method: %d", fp->method);
  }
  free(fp);
  return retval;
}

/***************************************************************
  Get a line, like fgets.
  Returns NULL in case of error, or when end-of-file reached
  and no characters have been read.
***************************************************************/
char *fz_fgets(char *buffer, int size, fz_FILE *fp)
{
  char *retval = NULL;
  
  switch (fp->method) {
#ifdef HAVE_LIBBZ2
  case FZ_BZIP2:
    {
      int i = 0;
      int last_read;

      /* See if first byte is already read and stored */
      if (fp->u.bz2.firstbyte >= 0) {
        buffer[0] = fp->u.bz2.firstbyte;
        fp->u.bz2.firstbyte = -1;
        i++;
      } else {
        if (!fp->u.bz2.eof) {
          last_read = BZ2_bzRead(&fp->u.bz2.error, fp->u.bz2.file, buffer + i, 1);
          i += last_read; /* 0 or 1 */
        }
      }
      if (!fp->u.bz2.eof) {
        /* Leave space for trailing zero */
        for (; i < size - 1
               && fp->u.bz2.error == BZ_OK && buffer[i - 1] != '\n' ;
             i += last_read) {
          last_read = BZ2_bzRead(&fp->u.bz2.error, fp->u.bz2.file,
                                 buffer + i, 1);
        }
        if (fp->u.bz2.error != BZ_OK &&
            (fp->u.bz2.error != BZ_STREAM_END ||
             i == 0)) {
          retval = NULL;
        } else {
          retval = buffer;
        } 
        if (fp->u.bz2.error == BZ_STREAM_END) {
          /* EOF reached. Do not BZ2_bzRead() any more. */
          fp->u.bz2.eof = TRUE;
        }
      }
      buffer[i] = '\0';
      break;
    }
#endif
#ifdef HAVE_LIBZ
  case FZ_ZLIB:
    retval = gzgets(fp->u.zlib, buffer, size);
    break;
#endif
  case FZ_PLAIN:
    retval = fgets(buffer, size, fp->u.plain);
    break;
  default:
    /* Should never happen */
    die("Internal error: Bad fz_fgets method: %d", fp->method);
  }
  return retval;
}

/***************************************************************
  Print formated, like fprintf.
  
  Note: zlib doesn't have gzvfprintf, but thats ok because its
  fprintf only does similar to what we do here (print to fixed
  buffer), and in addition this way we get to use our safe
  snprintf.

  Returns number of (uncompressed) bytes actually written, or
  0 on error.
***************************************************************/
int fz_fprintf(fz_FILE *fp, const char *format, ...)
{
  va_list ap;
  int retval = 0;
  
  va_start(ap, format);

  switch (fp->method) {
#ifdef HAVE_LIBBZ2
  case FZ_BZIP2:
    {
      char buffer[65536];
      int num;
      num = my_vsnprintf(buffer, sizeof(buffer), format, ap);
      if (num == -1) {
	  freelog(LOG_ERROR, "Too much data: truncated in fz_fprintf (%lu)",
		  (unsigned long) sizeof(buffer));
      }
      BZ2_bzWrite(&fp->u.bz2.error, fp->u.bz2.file, buffer, strlen(buffer));
      if (fp->u.bz2.error != BZ_OK) {
        retval = 0;
      } else {
        retval = strlen(buffer);
      }
    }
    break;
#endif
#ifdef HAVE_LIBZ
  case FZ_ZLIB:
    {
      char buffer[65536];
      int num;
      num = my_vsnprintf(buffer, sizeof(buffer), format, ap);
      if (num == -1) {
	  freelog(LOG_ERROR, "Too much data: truncated in fz_fprintf (%lu)",
		  (unsigned long) sizeof(buffer));
      }
      retval = gzwrite(fp->u.zlib, buffer, (unsigned int)strlen(buffer));
    }
    break;
#endif
  case FZ_PLAIN:
    retval = vfprintf(fp->u.plain, format, ap);
    break;
  default:
    /* Should never happen */
    die("Internal error: Bad fz_fprintf method: %d", fp->method);
  }
  va_end(ap);
  return retval;
}

/***************************************************************
  Return non-zero if there is an error status associated with
  this stream.  Check fz_strerror for details.
***************************************************************/
int fz_ferror(fz_FILE *fp)
{
  int retval = 0;

  switch (fp->method) {
#ifdef HAVE_LIBBZ2
  case FZ_BZIP2:
    if (fp->u.bz2.error != BZ_OK &&
        fp->u.bz2.error != BZ_STREAM_END) {
      retval = 1;
    } else {
      retval = 0;
    }
    break;
#endif
#ifdef HAVE_LIBZ
  case FZ_ZLIB:
    (void) gzerror(fp->u.zlib, &retval);	/* ignore string result here */
    if (retval > 0) {
      retval = 0;		/* only negative Z values are errors */
    }
    break;
#endif
  case FZ_PLAIN:
    retval = ferror(fp->u.plain);
    break;
  default:
    /* Should never happen */
    die("Internal error: Bad fz_ferror method: %d", fp->method);
  }
  return retval;
}

/***************************************************************
  Return string (pointer to static memory) containing an error
  description associated with the file.  Should only call
  this is you know there is an error (eg, from fz_ferror()).
  Note the error string may be based on errno, so should call
  this immediately after problem, or possibly something else
  might overwrite errno.
***************************************************************/
const char *fz_strerror(fz_FILE *fp)
{
  const char *retval = 0;
  
  switch(fp->method) {
#ifdef HAVE_LIBBZ2
  case FZ_BZIP2:
    {
      static char bzip2error[50];
      char *cleartext = NULL;

      /* Rationale for translating these:
       * - Some of them provide usable information to user
       * - Messages still contain numerical error code for developers
       */
      switch(fp->u.bz2.error) {
       case BZ_OK:
         cleartext = _("OK");
         break;
       case BZ_RUN_OK:
         cleartext = _("Run ok");
         break;
       case BZ_FLUSH_OK:
         cleartext = _("Flush ok");
         break;
       case BZ_FINISH_OK:
         cleartext = _("Finish ok");
         break;
       case BZ_STREAM_END:
         cleartext = _("Stream end");
         break;
       case BZ_CONFIG_ERROR:
         cleartext = _("Config error");
         break;
       case BZ_SEQUENCE_ERROR:
         cleartext = _("Sequence error");
         break;
       case BZ_PARAM_ERROR:
         cleartext = _("Parameter error");
         break;
       case BZ_MEM_ERROR:
         cleartext = _("Mem error");
         break;
       case BZ_DATA_ERROR:
         cleartext = _("Data error");
         break;
       case BZ_DATA_ERROR_MAGIC:
         cleartext = _("Not bzip2 file");
         break;
       case BZ_IO_ERROR:
         cleartext = _("IO error");
         break;
       case BZ_UNEXPECTED_EOF:
         cleartext = _("Unexpected EOF");
         break;
       case BZ_OUTBUFF_FULL:
         cleartext = _("Output buffer full");
         break;
       default:
         break;
      }

      if (cleartext != NULL) {
        my_snprintf(bzip2error, sizeof(bzip2error), _("Bz2: \"%s\" (%d)"),
                    cleartext, fp->u.bz2.error);
      } else {
        my_snprintf(bzip2error, sizeof(bzip2error), _("Bz2 error %d"),
                    fp->u.bz2.error);
      }
      retval = bzip2error;
      break;
    }
#endif
#ifdef HAVE_LIBZ
  case FZ_ZLIB:
    {
      int errnum;
      const char *estr = gzerror(fp->u.zlib, &errnum);
      if (errnum == Z_ERRNO) {
	retval = fc_strerror(fc_get_errno());
      } else {
	retval = estr;
      }
    }
    break;
#endif
  case FZ_PLAIN:
    retval = fc_strerror(fc_get_errno());
    break;
  default:
    /* Should never happen */
    die("Internal error: Bad fz_strerror method: %d", fp->method);
  }
  return retval;
}
