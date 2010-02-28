/***********************************************************************
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdarg.h>
#include <string.h>

/* utility */
#include "fcintl.h"
#include "log.h"
#include "mem.h"
#include "shared.h"
#include "support.h"

/* common */
#include "city.h"
#include "game.h"
#include "map.h"
#include "tile.h"
#include "unit.h"

#include "featured_text.h"

#define SEQ_START '<'
#define SEQ_STOP '>'
#define SEQ_END '/'

#define MAX_LEN_STR 32
#define LOG_FEATURED_TEXT LOG_VERBOSE

#define text_tag_list_rev_iterate(tags, ptag) \
  TYPED_LIST_ITERATE_REV(struct text_tag, tags, ptag)
#define text_tag_list_rev_iterate_end  LIST_ITERATE_REV_END

/* The text_tag structure.  See documentation in featured_text.h. */
struct text_tag {
  enum text_tag_type type;              /* The type of the tag. */
  offset_t start_offset;                /* The start offset (in bytes). */
  offset_t stop_offset;                 /* The stop offset (in bytes). */
  union {
    struct {                            /* TTT_COLOR only. */
      char foreground[MAX_LEN_STR];     /* foreground color name. */
      char background[MAX_LEN_STR];     /* background color name. */
    } color;
    struct {                            /* TTT_LINK only. */
      enum text_link_type type;         /* The target type of the link. */
      int id;                           /* The id of linked object. */
      char name[MAX_LEN_STR];           /* A string to indentify the link. */
    } link;
  };
};

enum sequence_type {
  ST_START,     /* e.g. [sequence]. */
  ST_STOP,      /* e.g. [/sequence]. */
  ST_SINGLE     /* e.g. [sequence/]. */
};

/**************************************************************************
  Return the long name of the text tag type.
  See also text_tag_type_short_name().
**************************************************************************/
static const char *text_tag_type_name(enum text_tag_type type)
{
  switch (type) {
  case TTT_BOLD:
    return "bold";
  case TTT_ITALIC:
    return "italic";
  case TTT_STRIKE:
    return "strike";
  case TTT_UNDERLINE:
    return "underline";
  case TTT_COLOR:
    return "color";
  case TTT_LINK:
    return "link";
  };
  /* Don't handle the default case to be warned if a new value was added. */
  return NULL;
}

/**************************************************************************
  Return the name abbreviation of the text tag type.
  See also text_tag_type_name().
**************************************************************************/
static const char *text_tag_type_short_name(enum text_tag_type type)
{
  switch (type) {
  case TTT_BOLD:
    return "b";
  case TTT_ITALIC:
    return "i";
  case TTT_STRIKE:
    return "s";
  case TTT_UNDERLINE:
    return "u";
  case TTT_COLOR:
    return "font";
  case TTT_LINK:
    return "l";
  };
  /* Don't handle the default case to be warned if a new value was added. */
  return NULL;
}

/**************************************************************************
  Return the name of the text tag link target type.
**************************************************************************/
static const char *text_link_type_name(enum text_link_type type)
{
  switch (type) {
  case TLT_CITY:
    return "city";
  case TLT_TILE:
    return "tile";
  case TLT_UNIT:
    return "unit";
  };
  /* Don't handle the default case to be warned if a new value was added. */
  return NULL;
}

/**************************************************************************
  Find inside a sequence the string associated to a particular option name.
  Returns TRUE on success.
**************************************************************************/
static bool find_option(const char *read, const char *option,
                        char *write, size_t write_len)
{
  size_t option_len = strlen(option);

  while (*read != '\0') {
    while (my_isspace(*read) && *read != '\0') {
      read++;
    }

    if (0 == strncasecmp(read, option, option_len)) {
      /* This is this one. */
      read += option_len;

      while ((my_isspace(*read) || *read == '=') && *read != '\0') {
        read++;
      }
      if (*read == '"') {
        /* Quote case. */
        const char *end = strchr(++read, '"');

        if (!end) {
          return FALSE;
        }
        if (end - read + 1 > 0) {
          mystrlcpy(write, read, MIN(end - read + 1, write_len));
        } else {
          *write = '\0';
        }
        return TRUE;
      } else {
        while (my_isalnum(*read) && write_len > 1) {
          *write++ = *read++;
          write_len--;
        }
        *write = '\0';
        return TRUE;
      }
    }
    read++;
  }

  return FALSE;
}

/**************************************************************************
  Initialize a text_tag structure from a string sequence.
  Returns TRUE on success.
**************************************************************************/
static bool text_tag_init_from_sequence(struct text_tag *ptag,
                                        enum text_tag_type type,
                                        offset_t start_offset,
                                        const char *sequence)
{
  ptag->type = type;
  ptag->start_offset = start_offset;
  ptag->stop_offset = OFFSET_UNSET;

  switch (type) {
  case TTT_BOLD:
  case TTT_ITALIC:
  case TTT_STRIKE:
  case TTT_UNDERLINE:
    return TRUE;
  case TTT_COLOR:
    {
      if (!find_option(sequence, "foreground", ptag->color.foreground,
                       sizeof(ptag->color.foreground))
          && !find_option(sequence, "color", ptag->color.foreground,
                          sizeof(ptag->color.foreground))) {
        ptag->color.foreground[0] = '\0';
      }
      if (!find_option(sequence, "background", ptag->color.background,
                       sizeof(ptag->color.background))
          && !find_option(sequence, "bg", ptag->color.background,
                          sizeof(ptag->color.background))) {
        ptag->color.background[0] = '\0';
      }
    }
    return TRUE;
  case TTT_LINK:
    {
      char buf[64];
      const char *name;
      int i;

      if (!find_option(sequence, "target", buf, sizeof(buf))
          && !find_option(sequence, "tgt", buf, sizeof(buf))) {
        freelog(LOG_FEATURED_TEXT,
                "text_tag_init_from_sequence: target link type not set.");
        return FALSE;
      }

      ptag->link.type = -1;
      for (i = 0; (name = text_link_type_name(i)); i++) {
        if (0 == mystrncasecmp(buf, name, strlen(name))) {
          ptag->link.type = i;
          break;
        }
      }
      if (ptag->link.type == -1) {
        freelog(LOG_FEATURED_TEXT, "text_tag_init_from_sequence: "
                "target link type not supported (\"%s\").", buf);
        return FALSE;
      }

      switch (ptag->link.type) {
      case TLT_CITY:
        {
          if (!find_option(sequence, "id", buf, sizeof(buf))) {
            freelog(LOG_FEATURED_TEXT,
                    "text_tag_init_from_sequence: city link without id.");
            return FALSE;
          }
          if (1 != sscanf(buf, "%d", &ptag->link.id)) {
            freelog(LOG_FEATURED_TEXT, "text_tag_init_from_sequence: "
                    "city link without valid id (\"%s\").", buf);
            return FALSE;
          }

          if (!find_option(sequence, "name", ptag->link.name,
                           sizeof(ptag->link.name))) {
            /* Set something as name. */
            my_snprintf(ptag->link.name, sizeof(ptag->link.name),
                        "CITY_ID%d", ptag->link.id);
          }
        }
        return TRUE;
      case TLT_TILE:
        {
          struct tile *ptile;
          int x, y;

          if (!find_option(sequence, "x", buf, sizeof(buf))) {
            freelog(LOG_FEATURED_TEXT, "text_tag_init_from_sequence: "
                    "tile link without x coordinate.");
            return FALSE;
          }
          if (1 != sscanf(buf, "%d", &x)) {
            freelog(LOG_FEATURED_TEXT, "text_tag_init_from_sequence: "
                    "tile link without valid x coordinate (\"%s\").", buf);
            return FALSE;
          }

          if (!find_option(sequence, "y", buf, sizeof(buf))) {
            freelog(LOG_FEATURED_TEXT, "text_tag_init_from_sequence: "
                    "tile link without y coordinate.");
            return FALSE;
          }
          if (1 != sscanf(buf, "%d", &y)) {
            freelog(LOG_FEATURED_TEXT, "text_tag_init_from_sequence: "
                    "tile link without valid y coordinate (\"%s\").", buf);
            return FALSE;
          }

          ptile = map_pos_to_tile(x, y);
          if (!ptile) {
            freelog(LOG_FEATURED_TEXT, "text_tag_init_from_sequence: "
                    "(%d, %d) are not valid coordinates in this game.",
                    x, y);
            return FALSE;
          }
          ptag->link.id = tile_index(ptile);
          my_snprintf(ptag->link.name, sizeof(ptag->link.name),
                      "(%d, %d)", TILE_XY(ptile));
        }
        return TRUE;
      case TLT_UNIT:
        {
          if (!find_option(sequence, "id", buf, sizeof(buf))) {
            freelog(LOG_FEATURED_TEXT,
                    "text_tag_init_from_sequence: unit link without id.");
            return FALSE;
          }
          if (1 != sscanf(buf, "%d", &ptag->link.id)) {
            freelog(LOG_FEATURED_TEXT, "text_tag_init_from_sequence: "
                    "unit link without valid id (\"%s\").", buf);
            return FALSE;
          }

          if (!find_option(sequence, "name", ptag->link.name,
                           sizeof(ptag->link.name))) {
            /* Set something as name. */
            my_snprintf(ptag->link.name, sizeof(ptag->link.name),
                        "UNIT_ID%d", ptag->link.id);
          }
        }
        return TRUE;
      };
    }
  };
  return FALSE;
}

/**************************************************************************
  Initialize a text_tag structure from a va_list.

  What's should be in the va_list:
  - If the text tag type is TTT_BOLD, TTT_ITALIC, TTT_STRIKE or
  TTT_UNDERLINE, there shouldn't be any extra argument.
  - If the text tag type is TTT_COLOR, then there should be 2 arguments of
  type 'const char *'.  The first determines the foreground color, the last
  the background color.  Empty strings and NULL are supported.
  - If the text tag type is TTT_LINK, then there should be 2 extra arguments.
  The first is type 'enum text_link_type' and will determine the type of the
  following argument:
    - If the link type is TLT_CITY, last argument is typed 'struct city *'.
    - If the link type is TLT_TILE, last argument is typed 'struct tile *'.
    - If the link type is TLT_UNIT, last argument is typed 'struct unit *'.

  Returns TRUE on success.
**************************************************************************/
static bool text_tag_initv(struct text_tag *ptag, enum text_tag_type type,
                           offset_t start_offset, offset_t stop_offset,
                           va_list args)
{
  ptag->type = type;
  ptag->start_offset = start_offset;
  ptag->stop_offset = stop_offset;

  switch (type) {
  case TTT_BOLD:
  case TTT_ITALIC:
  case TTT_STRIKE:
  case TTT_UNDERLINE:
    return TRUE;
  case TTT_COLOR:
    {
      const char *foreground = va_arg(args, const char *);
      const char *background = va_arg(args, const char *);

      if ((NULL == foreground || '\0' == foreground[0])
          && (NULL == background || '\0' == background[0])) {
        return FALSE; /* No color at all. */
      }

      if (NULL != foreground && '\0' != foreground[0]) {
        sz_strlcpy(ptag->color.foreground, foreground);
      } else {
        ptag->color.foreground[0] = '\0';
      }

      if (NULL != background && '\0' != background[0]) {
        sz_strlcpy(ptag->color.background, background);
      } else {
        ptag->color.background[0] = '\0';
      }
    }
    return TRUE;
  case TTT_LINK:
    {
      ptag->link.type = va_arg(args, enum text_link_type);
      switch (ptag->link.type) {
      case TLT_CITY:
        {
          struct city *pcity = va_arg(args, struct city *);

          if (!pcity) {
            return FALSE;
          }
          ptag->link.id = pcity->id;
          sz_strlcpy(ptag->link.name, city_name(pcity));
        }
        return TRUE;
      case TLT_TILE:
        {
          struct tile *ptile = va_arg(args, struct tile *);

          if (!ptile) {
            return FALSE;
          }
          ptag->link.id = tile_index(ptile);
          my_snprintf(ptag->link.name, sizeof(ptag->link.name),
                      "(%d, %d)", TILE_XY(ptile));
        }
        return TRUE;
      case TLT_UNIT:
        {
          struct unit *punit = va_arg(args, struct unit *);

          if (!punit) {
            return FALSE;
          }
          ptag->link.id = punit->id;
          sz_strlcpy(ptag->link.name, unit_rule_name(punit));
        }
        return TRUE;
      };
    }
  };
  return FALSE;
}

/**************************************************************************
  Print in a string the start sequence of the tag.
**************************************************************************/
static size_t text_tag_start_sequence(const struct text_tag *ptag,
                                      char *buf, size_t len)
{
  switch (ptag->type) {
  case TTT_BOLD:
  case TTT_ITALIC:
  case TTT_STRIKE:
  case TTT_UNDERLINE:
    return my_snprintf(buf, len, "%c%s%c", SEQ_START,
                       text_tag_type_short_name(ptag->type), SEQ_STOP);
  case TTT_COLOR:
    {
      size_t ret = my_snprintf(buf, len, "%c%s", SEQ_START,
                               text_tag_type_short_name(ptag->type));

      if (ptag->color.foreground[0] != '\0') {
        ret += my_snprintf(buf + ret, len - ret, " color=\"%s\"",
                           ptag->color.foreground);
      }
      if (ptag->color.background[0] != '\0') {
        ret += my_snprintf(buf + ret, len - ret, " bg=\"%s\"",
                           ptag->color.background);
      }
      return ret + my_snprintf(buf + ret, len - ret, "%c", SEQ_STOP);
    }
  case TTT_LINK:
    {
      size_t ret = my_snprintf(buf, len, "%c%s tgt=\"%s\"", SEQ_START,
                               text_tag_type_short_name(ptag->type),
                               text_link_type_name(ptag->link.type));

      switch (ptag->link.type) {
      case TLT_CITY:
        {
          struct city *pcity = game_find_city_by_number(ptag->link.id);

          if (pcity) {
            ret += my_snprintf(buf + ret, len - ret,
                               " id=%d name=\"%s\"",
                               pcity->id, city_name(pcity));
          } else {
            ret += my_snprintf(buf + ret, len - ret,
                               " id=%d", ptag->link.id);
          }
        }
        break;
      case TLT_TILE:
        {
          struct tile *ptile = index_to_tile(ptag->link.id);

          if (ptile) {
            ret += my_snprintf(buf + ret, len - ret,
                               " x=%d y=%d", TILE_XY(ptile));
          } else {
            ret += my_snprintf(buf + ret, len - ret,
                               " id=%d", ptag->link.id);
          }
        }
        break;
      case TLT_UNIT:
        {
          struct unit *punit = game_find_unit_by_number(ptag->link.id);

          if (punit) {
            ret += my_snprintf(buf + ret, len - ret,
                               " id=%d name=\"%s\"",
                               punit->id, unit_rule_name(punit));
          } else {
            ret += my_snprintf(buf + ret, len - ret,
                               " id=%d", ptag->link.id);
          }
        }
        break;
      };

      if (ptag->stop_offset == ptag->start_offset) {
        /* This is a single sequence like [link ... /]. */
        ret += my_snprintf(buf + ret, len - ret, "%c", SEQ_END);
      }

      return ret + my_snprintf(buf + ret, len - ret, "%c", SEQ_STOP);
    }
  };
  return 0;
}

/**************************************************************************
  Print in a string the stop sequence of the tag.
**************************************************************************/
static size_t text_tag_stop_sequence(const struct text_tag *ptag,
                                     char *buf, size_t len)
{
  if (ptag->type == TTT_LINK && ptag->stop_offset == ptag->start_offset) {
    /* Should be already finished. */
    return 0;
  }

  return my_snprintf(buf, len, "%c%c%s%c", SEQ_START, SEQ_END,
                     text_tag_type_short_name(ptag->type), SEQ_STOP);
}

/**************************************************************************
  When the sequence looks like [sequence/] then we insert a string instead.
**************************************************************************/
static size_t text_tag_replace_text(const struct text_tag *ptag,
                                    char *buf, size_t len)
{
  if (ptag->type != TTT_LINK) {
    return 0;
  }

  if (!is_server()) {
    /* The client check if this should be updated or translated. */
    switch (ptag->link.type) {
    case TLT_CITY:
      {
        struct city *pcity = game_find_city_by_number(ptag->link.id);

        if (pcity) {
          return my_snprintf(buf, len, "%s", city_name(pcity));
        }
      }
      break;
    case TLT_TILE:
      break;
    case TLT_UNIT:
      {
        struct unit *punit = game_find_unit_by_number(ptag->link.id);

        if (punit) {
          return my_snprintf(buf, len, "%s", unit_name_translation(punit));
        }
      }
      break;
    };
  }

  if (ptag->link.type == TLT_UNIT) {
    /* Attempt to translate the link name (it should be a unit type name). */
    return my_snprintf(buf, len, "%s", _(ptag->link.name));
  } else {
    return my_snprintf(buf, len, "%s", ptag->link.name);
  }
}

/**************************************************************************
  Returns a new text_tag or NULL on error.

  Prototype:
  - If tag_type is TTT_BOLD, TTT_ITALIC, TTT_STRIKE or TTT_UNDERLINE, there
  shouldn't be any extra argument.
  - If tag_type is TTT_COLOR:
    size_t featured_text_apply_tag(..., const char *foreground,
                                   const char *background);
  - If tag_type is TTT_LINK and you want a city link:
    size_t featured_text_apply_tag(..., TLT_CITY, struct city *pcity);
  - If tag_type is TTT_LINK and you want a tile link:
    size_t featured_text_apply_tag(..., TLT_TILE, struct tile *ptile);
  - If tag_type is TTT_LINK and you want an unit link:
    size_t featured_text_apply_tag(..., TLT_UNIT, struct unit *punit);

  See also comment for text_tag_initv().
**************************************************************************/
struct text_tag *text_tag_new(enum text_tag_type tag_type,
                              offset_t start_offset, offset_t stop_offset,
                              ...)
{
  struct text_tag *ptag = fc_malloc(sizeof(struct text_tag));
  va_list args;
  bool ok;

  va_start(args, stop_offset);
  ok = text_tag_initv(ptag, tag_type, start_offset, stop_offset, args);
  va_end(args);

  if (ok) {
    return ptag;
  } else {
    free(ptag);
    return NULL;
  }
}

/**************************************************************************
  Free a text_tag structure.
**************************************************************************/
void text_tag_destroy(struct text_tag *ptag)
{
  free(ptag);
}

/**************************************************************************
  Return the type of this text tag.
**************************************************************************/
enum text_tag_type text_tag_type(const struct text_tag *ptag)
{
  return ptag->type;
}

/**************************************************************************
  Return the start offset (in bytes) of this text tag.
**************************************************************************/
offset_t text_tag_start_offset(const struct text_tag *ptag)
{
  return ptag->start_offset;
}

/**************************************************************************
  Return the stop offset (in bytes) of this text tag.
**************************************************************************/
offset_t text_tag_stop_offset(const struct text_tag *ptag)
{
  return ptag->stop_offset;
}

/**************************************************************************
  Return the foreground color suggested by this text tag.  This requires
  the tag type to be TTT_COLOR.  Returns NULL on error, "" if unset.
**************************************************************************/
const char *text_tag_color_foreground(const struct text_tag *ptag)
{
  if (ptag->type != TTT_COLOR) {
    freelog(LOG_ERROR, "text_tag_color_foreground: incompatible tag type.");
    return NULL;
  }

  return ptag->color.foreground;
}

/**************************************************************************
  Return the background color suggested by this text tag.  This requires
  the tag type to be TTT_COLOR.  Returns NULL on error, "" if unset.
**************************************************************************/
const char *text_tag_color_background(const struct text_tag *ptag)
{
  if (ptag->type != TTT_COLOR) {
    freelog(LOG_ERROR, "text_tag_color_background: incompatible tag type.");
    return NULL;
  }

  return ptag->color.background;
}

/**************************************************************************
  Return the link target type suggested by this text tag.  This requires
  the tag type to be TTT_LINK.  Returns -1 on error.
**************************************************************************/
enum text_link_type text_tag_link_type(const struct text_tag *ptag)
{
  if (ptag->type != TTT_LINK) {
    freelog(LOG_ERROR, "text_tag_link_type: incompatible tag type.");
    return -1;
  }

  return ptag->link.type;
}

/**************************************************************************
  Return the link target id suggested by this text tag (city id,
  tile index or unit id).  This requires the tag type to be TTT_LINK.
  Returns -1 on error.
**************************************************************************/
int text_tag_link_id(const struct text_tag *ptag)
{
  if (ptag->type != TTT_LINK) {
    freelog(LOG_ERROR, "text_tag_link_id: incompatible tag type.");
    return -1;
  }

  return ptag->link.id;
}

/**************************************************************************
  Clear and free all tags inside the tag list.  It doesn't free the list
  itself.  It just should be used instead of text_tag_list_clear() which
  doesn't free the tags.
**************************************************************************/
void text_tag_list_clear_all(struct text_tag_list *tags)
{
  if (!tags) {
    return;
  }

  text_tag_list_iterate(tags, ptag) {
    text_tag_destroy(ptag);
  } text_tag_list_iterate_end;
  text_tag_list_clear(tags);
}

/**************************************************************************
  This function returns a new pointer to a text_tag_list which is similar
  to the 'tags' argument.
**************************************************************************/
struct text_tag_list *text_tag_list_dup(const struct text_tag_list *tags)
{
  struct text_tag_list *new_tags;

  if (!tags) {
    return NULL;
  }

  new_tags = text_tag_list_new();
  text_tag_list_iterate(tags, ptag) {
    struct text_tag *pnew_tag = fc_malloc(sizeof(struct text_tag));

    *pnew_tag = *ptag;
    text_tag_list_append(new_tags, pnew_tag);
  } text_tag_list_iterate_end;

  return new_tags;
}

/**************************************************************************
  Extract a sequence from a string.  Also, determine the type and the text
  tag type of the sequence.  Return 0 on error.
**************************************************************************/
static size_t extract_sequence_text(const char *featured_text,
                                    char *buf, size_t len,
                                    enum sequence_type *seq_type,
                                    enum text_tag_type *type)
{
  const char *read = featured_text;
  const char *stop = strchr(read, SEQ_STOP);
  const char *end = stop;
  const char *name;
  size_t type_len;
  size_t name_len;
  int i;

  if (!stop) {
    return 0; /* Not valid. */
  }

  /* Check sequence type. */
  for (read++; my_isspace(*read); read++);

  if (*read == SEQ_END) {
    *seq_type = ST_STOP;
    read++;
  } else {
    for (end--; my_isspace(*end); end--);

    if (*end == SEQ_END) {
      *seq_type = ST_SINGLE;

      for (end--; my_isspace(*end); end--);
    } else {
      *seq_type = ST_START;
    }
  }

  while (my_isspace(*read)) {
    read++;
  }

  /* Check the length of the type name. */
  for (name = read; name < stop; name++) {
    if (!my_isalpha(*name)) {
      break;
    }
  }
  type_len = name - read;

  *type = -1;
  for (i = 0; (name = text_tag_type_name(i)); i++) {
    name_len = strlen(name);
    if (name_len == type_len && 0 == mystrncasecmp(name, read, name_len)) {
      read += name_len;
      *type = i;
      break;
    }
  }
  if (*type == -1) {
    /* Try with short names. */
    for (i = 0; (name = text_tag_type_short_name(i)); i++) {
      name_len = strlen(name);
      if (name_len == type_len && 0 == mystrncasecmp(name, read, name_len)) {
        read += name_len;
        *type = i;
        break;
      }
    }
    if (*type == -1) {
      return 0; /* Not valid. */
    }
  }

  while (my_isspace(*read)) {
    read++;
  }

  if (end - read + 2 > 0) {
    mystrlcpy(buf, read, MIN(end - read + 2, len));
  } else {
    buf[0] = '\0';
  }
  return stop - featured_text + 1;
}
                                    

/**************************************************************************
  Separate the text from the text features.  'tags' can be NULL.
**************************************************************************/
size_t featured_text_to_plain_text(const char *featured_text,
                                   char *plain_text, size_t plain_text_len,
                                   struct text_tag_list *tags)
{
  const char *read = featured_text;
  char *write = plain_text;
  size_t write_len = plain_text_len;

  if (tags) {
    text_tag_list_clear_all(tags);
  }

  while (*read != '\0' && write_len > 1) {
    if (SEQ_START == *read) {
      /* Escape sequence... */
      char buf[write_len];
      enum sequence_type seq_type;
      enum text_tag_type type;
      size_t len = extract_sequence_text(read, buf, write_len,
                                         &seq_type, &type);

      if (len > 0) {
        /* Looks a valid sequence. */
        read += len;
        switch (seq_type) {
        case ST_START:
          if (tags) {
            /* Create a new tag. */
            struct text_tag *ptag = fc_malloc(sizeof(struct text_tag));

            if (text_tag_init_from_sequence(ptag, type,
                                            write - plain_text, buf)) {
              text_tag_list_append(tags, ptag);
            } else {
              text_tag_destroy(ptag);
              freelog(LOG_FEATURED_TEXT,
                      "Couldn't create a text tag with \"%s\".", buf);
            }
          }
          break;
        case ST_STOP:
          if (tags) {
            /* Set the stop offset. */
            struct text_tag *ptag = NULL;

            /* Look up on reversed order. */
            text_tag_list_rev_iterate(tags, piter) {
              if (piter->type == type
                  && piter->stop_offset == OFFSET_UNSET) {
                ptag = piter;
                break;
              }
            } text_tag_list_rev_iterate_end;

            if (ptag) {
              ptag->stop_offset = write - plain_text;
            } else {
              freelog(LOG_FEATURED_TEXT, "Extra text tag end for \"%s\".",
                      text_tag_type_name(type));
            }
          }
          break;
        case ST_SINGLE:
          {
            /* In this case, we replace the sequence by some text. */
            struct text_tag tag;

            if (!text_tag_init_from_sequence(&tag, type,
                                             write - plain_text, buf)) {
              freelog(LOG_FEATURED_TEXT,
                      "Couldn't create a text tag with \"%s\".", buf);
            } else {
              len = text_tag_replace_text(&tag, write, write_len);
              write += len;
              write_len -= len;
              if (tags) {
                /* Set it in the list. */
                struct text_tag *ptag = fc_malloc(sizeof(struct text_tag));

                *ptag = tag;
                ptag->stop_offset = write - plain_text;
                text_tag_list_append(tags, ptag);
              }
            }
          }
          break;
        };
      } else {
        *write++ = *read++;
        write_len--;
      }
    } else {
      *write++ = *read++;
      write_len--;
    }
  }
  *write = '\0';
  return plain_text_len - write_len;
}

/**************************************************************************
  Apply a tag to a text.  This text can already containing escape
  sequences.  Returns 0 on error.

  Prototype:
  - If tag_type is TTT_BOLD, TTT_ITALIC, TTT_STRIKE or TTT_UNDERLINE, there
  shouldn't be any extra argument.
  - If tag_type is TTT_COLOR:
    size_t featured_text_apply_tag(..., const char *foreground,
                                   const char *background);
  - If tag_type is TTT_LINK and you want a city link:
    size_t featured_text_apply_tag(..., TLT_CITY, struct city *pcity);
  - If tag_type is TTT_LINK and you want a tile link:
    size_t featured_text_apply_tag(..., TLT_TILE, struct tile *ptile);
  - If tag_type is TTT_LINK and you want an unit link:
    size_t featured_text_apply_tag(..., TLT_UNIT, struct unit *punit);

  See also comment for text_tag_initv().
**************************************************************************/
size_t featured_text_apply_tag(const char *text_source,
                               char *featured_text, size_t featured_text_len,
                               enum text_tag_type tag_type,
                               offset_t start_offset, offset_t stop_offset,
                               ...)
{
  struct text_tag tag;
  size_t len, total_len = 0;
  va_list args;

  if (start_offset == OFFSET_UNSET
      || start_offset > strlen(text_source)
      || (stop_offset != OFFSET_UNSET
          && stop_offset < start_offset)) {
    freelog(LOG_FEATURED_TEXT, "featured_text_apply_tag: invalid offsets.");
    return 0;
  }

  va_start(args, stop_offset);
  if (!text_tag_initv(&tag, tag_type, start_offset, stop_offset, args)) {
    va_end(args);
    return 0;
  }
  va_end(args);

  if (start_offset > 0) {
    /* First part: before the sequence. */
    len = 0;
    while (len < start_offset
           && *text_source != '\0'
           && featured_text_len > 1) {
      *featured_text++ = *text_source++;
      featured_text_len--;
      len++;
    }
    total_len += len;
  }

  /* Start sequence. */
  len = text_tag_start_sequence(&tag, featured_text, featured_text_len);
  total_len += len;
  featured_text += len;
  featured_text_len -= len;

  /* Second part: between the sequences. */
  len = start_offset;
  while (len < stop_offset
         && *text_source != '\0'
         && featured_text_len > 1) {
    *featured_text++ = *text_source++;
    featured_text_len--;
    len++;
  }
  total_len += len;

  /* Stop sequence. */
  len = text_tag_stop_sequence(&tag, featured_text, featured_text_len);
  total_len += len;
  featured_text += len;
  featured_text_len -= len;

  /* Third part: after the sequence. */
  while (*text_source != '\0'
         && featured_text_len > 1) {
    *featured_text++ = *text_source++;
    featured_text_len--;
    total_len++;
  }
  *featured_text = '\0';

  return total_len;
}

/**************************************************************************
  Get a text link to a city.
  N.B.: The returned string is static, so every call to this function
  overwrites the previous.
**************************************************************************/
const char *city_link(const struct city *pcity)
{
  static char buf[128];

  my_snprintf(buf, sizeof(buf), "<a href=\"#\" onclick=\"show_city_dialog_by_id(%d);\">%s</a>",
              pcity->id,
              city_name(pcity));
  return buf;
}

/**************************************************************************
  Get a text link to a tile.
  N.B.: The returned string is static, so every call to this function
  overwrites the previous.
**************************************************************************/
const char *tile_link(const struct tile *ptile)
{
  static char buf[128];

  my_snprintf(buf, sizeof(buf), "%c%s tgt=\"%s\" x=%d y=%d %c%c",
              SEQ_START, text_tag_type_name(TTT_LINK),
              text_link_type_name(TLT_TILE), TILE_XY(ptile),
              SEQ_END, SEQ_STOP);
  return buf;
}

/**************************************************************************
  Get a text link to an unit.
  N.B.: The returned string is static, so every call to this function
  overwrites the previous.
**************************************************************************/
const char *unit_link(const struct unit *punit)
{
  static char buf[128];

  /* We use the rule name of the unit, it will be translated in every
   * local sides in the function text_tag_replace_text(). */
  my_snprintf(buf, sizeof(buf), "%s", unit_rule_name(punit));
  return buf;
}
