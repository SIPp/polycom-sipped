/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Copyright (C) 2003 - The Authors
 *
 *  Author : Richard GAYRAUD - 04 Nov 2003
 *           From Hewlett Packard Company.
 */

/*
 * Mini xml parser:
 *
 * WARNING 1: Only supports printable
 * ASCII characters in xml files. '\0'
 * is not a valid character. Returned string are
 * NULL-terminated.
 *
 * WARNING 2: Does not supports multithreading. Works
 * with static buffer, no memory allocation.
 */

/*******************  Include files *********************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <xp_parser.h>

/************* Constants and Global variables ***********/

#define XP_MAX_NAME_LEN   256
#define XP_MAX_FILE_LEN   65536
#define XP_MAX_STACK_LEN  256

char   xp_file     [XP_MAX_FILE_LEN + 1];
char * xp_position [XP_MAX_STACK_LEN];
int    xp_stack    = 0;

/****************** Internal routines ********************/
int xp_replace(char *source, char *dest, const char *search, const char *replace)
{
  char *position;
  char *occurances;
  int number = 0;

  if (!source || !dest || !search || !replace) {
    return -1;
  }
  dest[0] = '\0';
  position = source;
  occurances = strstr(position, search);
  while (occurances) {
    strncat(dest, position, occurances - position);
    strcat(dest, replace); 
    position = occurances + strlen(search);
    occurances = strstr(position, search);
    number++;
  }
  strcat(dest, position);
  return number;
}

/* This finds the end of something like <send foo="bar">, and does not recurse
 * into other elements. */
char * xp_find_start_tag_end(char *ptr)
{
  while(*ptr) {
    if (*ptr == '<') {
      if ((strstr(ptr,"<!--") == ptr)) {
        char * comment_end = strstr(ptr, "-->");
        if(!comment_end) return NULL;
        ptr = comment_end + 3;
      } else {
	return NULL;
      }
    } else  if((*ptr == '/') && (*(ptr+1) == '>')) {
      return ptr;
    } else if (*ptr == '"') {
      ptr++;
      while(*ptr) {
	if (*ptr == '\\') {
	  ptr += 2;
	} else if (*ptr=='"') {
	  ptr++;
	  break;
	} else {
	  ptr++;
	}
      }
    } else if (*ptr == '>') {
      return ptr;
    } else {
      ptr++;
    }
  }
  return ptr;
}

char * xp_find_local_end()
{
  char * ptr = xp_position[xp_stack];
  int level = 0;
  
  while(*ptr) {
    if (*ptr == '<') {
      if ((*(ptr+1) == '!') && 
          (*(ptr+2) == '[') &&
          (strstr(ptr,"<![CDATA[") == ptr)) {
        char * cdata_end = strstr(ptr, "]]>");
        if(!cdata_end) return NULL;
        ptr = cdata_end + 3;
      } else if ((*(ptr+1) == '!') && 
          (*(ptr+2) == '-') &&
          (strstr(ptr,"<!--") == ptr)) {
        char * comment_end = strstr(ptr, "-->");
        if(!comment_end) return NULL;
        ptr = comment_end + 3;
      } else if(*(ptr+1) == '/') {
        level--;
        if(level < 0) return ptr;
      } else {
        level ++;
      }
    } else  if((*ptr == '/') && (*(ptr+1) == '>')) {
      level --;
      if(level < 0) return ptr;
    } else if (*ptr == '"') {
      ptr++;
      while(*ptr) {
	if (*ptr == '\\') {
	  ptr ++; /* Skip the slash. */
	} else if (*ptr=='"') {
	  break;
	}
	ptr++;
      }
    }
    ptr++;
  }
  return ptr;
}

/********************* Interface routines ********************/

int xp_set_xml_buffer_from_string(char * str, int dumpxml)
{
  size_t len = strlen(str);

  if(len > XP_MAX_FILE_LEN) {
    return 0;
  }
  strcpy(xp_file, str);
  xp_stack = 0;
  xp_position[xp_stack] = xp_file;
  
  if (dumpxml) 
    printf("%s", &xp_file);

  if(strstr(xp_position[xp_stack], "<?xml") != xp_position[xp_stack]) return 0;
  if(!strstr(xp_position[xp_stack], "?>")) return 0;
  xp_position[xp_stack] = xp_position[xp_stack] + 2;

  return 1;
}

// return index where path ends (and file spec begins) or 0 if it is missing
// so filename[end] will be at first letter of filename
int xp_get_start_index_of_filename(char * filename)
{
  int end = strlen(filename);
  for (; ((end>0) && (filename[end-1] != '/') && (filename[end-1] != '\\')); end--);
  return end;
}

#define DEBUG(x, ...) { if (0) { printf(x, ##__VA_ARGS__); } }

// return 1 on success, 0 on error 
int xp_open_and_buffer_file(char * filename, char * path, int *index, unsigned *sub_list, unsigned sub_length)
{
  int include_index = 0;
  const char* include_tag = "<xi:include href=\"";
  int include_tag_length = strlen(include_tag);
  int dialog_index = 0;
  const char* dialog_param = "dialog=\"";
  int dialog_param_length = strlen(dialog_param);
  int replace_index = 0;
  const char* replace_param = "dialogs=\"";
  int replace_param_length = strlen(replace_param);
  const int XP_MAX_LOCAL_SUB_LIST_LEN = 26;
  unsigned local_sub_list[XP_MAX_LOCAL_SUB_LIST_LEN];
  unsigned local_sub_length = 0;

  char include_file_name[XP_MAX_NAME_LEN];
  char new_path[XP_MAX_NAME_LEN];
  char path_and_filename[XP_MAX_NAME_LEN];

  // combine path and filename for file open if open without fails; 
  // add file path to existing path (or replace if absolute) for recursive opens
  if ((filename[0] != '\\') && (filename[0] != '/') )
    snprintf(path_and_filename, XP_MAX_NAME_LEN, "%s%s", path, filename);
  else
    strcpy(path_and_filename, filename);    
  int new_path_length = xp_get_start_index_of_filename(path_and_filename);
  strncpy(new_path, path_and_filename, new_path_length);
  new_path[new_path_length] = 0;
  DEBUG("filename = '%s', path = '%s', path_and_filename = '%s', new_path = '%s'\n", filename, path, path_and_filename, new_path);

  int c;
  FILE * f = fopen(filename, "rb");
  if (!f) {
    if ((filename[0] != '\\') && (filename[0] != '/') && (strlen(path)) ) {
      f = fopen(path_and_filename, "rb");
        if(!f) { 
          printf("Unable to open file name '%s' directly or in '%s'\n", filename, new_path);
          return 0; 
        }
    }
    else {
      printf("Unable to open file name '%s'\n", filename);
      return 0; 
    }
  } // if !f

  while((c = fgetc(f)) != EOF) {
    // Handle match for include file
    if (c == include_tag[include_index]) {
      include_index++;
      if (include_index >= include_tag_length) {
        // match: move index to spot to overwrite, read out the filename, recursively call to place inline
        *index = *index - (include_tag_length-1); // index not bumped for final quote yet

        // read up until '">' (if EOF or EOLN before its an error)
        include_index = 0; // use to track file name
        while((c = fgetc(f)) != '"') {
          if (c == EOF || c == '\r' || c == '\n') {
            printf("xi:include tag must be formatted exactly '<xi:include href=\"filename\"/>'  No EOF or EOLN permitted.\n");
            fclose(f);
            return 0;
          }
          include_file_name[include_index] = c;
          include_index++;
        } // while fgetc != '"'

        c = getc(f);
        while ((c == ' ') || (c == '\t')) c = fgetc(f); // skip whitespace  

        if (c != '/') { // if not closing tab, specifying dialogs="1,2,3"
          DEBUG("dialogs parameter expected.\n");

          // match 'dialogs="'
          replace_index = 0;
          while ( (replace_index < replace_param_length) && (c == replace_param[replace_index]) ) {
            c = fgetc(f);
            replace_index++;
          }
          if (replace_index != replace_param_length) { 
            printf("Error: only valid option to xi:include is 'dialogs=\"1,2,3\"'.\n");
            fclose(f);
            return 0;
          }
          // extract n1,n2,n3 until " is encountered
          DEBUG("dialogs tag matched, extracting list.\n");
          char number_str[8];
          int number_len = 0;
          do {
            if ((c >= '0' && c <= '9') || (toupper(c) >= 'A' && toupper(c) <= 'Z')) {
              number_str[number_len] = c;
              number_len++;
              if (number_len > 6) {
                number_str[number_len] = 0;
                printf("Error in '%s': dialogs substitution string %s is too long. dialogs tag value must be comma separated numbers.\n", filename, number_str);
                fclose(f);
                return 0;
              }
            }
            else if (c == ',' || c == '"') {
              if (number_len > 0) {
                // add numbers / letters
                int d;
                if (local_sub_length >= XP_MAX_LOCAL_SUB_LIST_LEN) {
                  printf("Error in '%s': Too many substitution parameters. Maximum is %d.\n", filename, XP_MAX_LOCAL_SUB_LIST_LEN);
                  fclose(f);
                  return 0;
                }
                number_str[number_len] = 0;
                if (number_str[0] >= '0' && number_str[0] <= '9') {
                  d = atoi(number_str);
                  if (d > 99) {
                    printf("Error in '%s': Maximum dialog ID for substitution is 99.\n", filename);
                    fclose(f);
                    return 0;
                  }
                }
                else if ((toupper(number_str[0]) >= 'A') && (toupper(number_str[0]) <= 'Z') && 
                  (toupper(number_str[1]) == toupper(number_str[0])) && (number_len == 2)) {
                  d = number_str[0] - (int)'A';
                  if (d >= sub_length) {
                    printf("'Error in '%s': %s' => %d is greater than largest substitution available of %d.\n", filename, number_str, d+1, sub_length);
                    fclose(f);
                    return 0;
                  }
                  DEBUG("number_str = %s => %d. sub_list[%d] = %d\n", number_str, d, d, sub_list[d]);
                  d = sub_list[d];
                }
                else {
                  printf("Invalid dialogs parameter value '%s'.\n", number_str);
                  fclose(f);
                  return 0;
                }

                DEBUG("Adding %d to local_sub_list[%d].\n", d, local_sub_length);
                local_sub_list[local_sub_length] = d;
                local_sub_length++;
                number_len = 0;
              }
              if (c == '"') 
                break;
            } // if c== ',' || '"'
            else {
              printf("Error, xi:include tag's dialogs list must contain numbers separated with commas and no whitespace.\n");
              fclose(f);
              return 0;
            }
            c = fgetc(f);
          } while (1);

          c = fgetc(f);
          while ((c == ' ') || (c == '\t')) c = fgetc(f); // skip whitespace          
         
        } // if dialogs option specified


        if (c != '/' || fgetc(f) != '>') {
          printf("xi:include tag must be formatted exactly '<xi:include href=\"filename\" dialogs=\"1,2,3\"] />'  '/>' must be together.\n");
          fclose(f);
          return 0;
        }
        include_file_name[include_index] = 0;      
       
        if (!xp_open_and_buffer_file(include_file_name, new_path, index, local_sub_list, local_sub_length)) 
          return 0;  
        include_index = 0;
        local_sub_length = 0;
        c = '\r'; // don't add this letter if returning from recursive parse.
      } // if include_index >= include_tag_length
    } // if c == include_tag[include_index])
    else
      include_index = 0; // not include tag: reset search

    // Handle parsing file into xp_file
    if(c == '\r') continue;
    xp_file[(*index)++] = c;
    // check for dialog="nn" substitution
    if (sub_list) {
      // check 'dialog="' part
      if (dialog_index < dialog_param_length) {
        if (dialog_param[dialog_index] == c)
          dialog_index++;
        else 
          dialog_index = 0;
      } // dialog_index < dialog_param_length
      // check for 1st letter
      else if (dialog_index == dialog_param_length) {
        if ((toupper(c) >= 'A') && (toupper(c) <= 'Z')) 
          dialog_index++;
        else
          dialog_index = 0;
      }
      // check 1st & second letters match each other (ie AA not AB)
      else if (dialog_index == dialog_param_length+1) {
        if (xp_file[(*index)-2] == c) 
          dialog_index++;
        else
          dialog_index = 0;
      }
      // final quote: substitute number
      // Note this implementation replaces 2 letters with 2 digits, but algorithm could
      // easily be extended to accomodate variable-length substitution as replacement 
      // is performed inline.
      else if (dialog_index == dialog_param_length+2) {
        if (c == '"') {
          // convert to integer s = [AA => 0, BB => 1, etc]
          // Note: index is new spot; i-1 is ", i-2 & i-3 are the letters to replace
          int sub_idx = toupper(xp_file[(*index)-2] ) - (int)'A';
          if (sub_idx >= sub_length) {
            printf("Error in '%s': not enough include parameters while attempting to substitute '%s'. This requires at least %d dialog id(s) specified with the include, but there were only %d.\n", filename, (char *) &xp_file[(*index)-(dialog_param_length+4)], sub_idx+1, sub_length);
            fclose(f);
            return 0; 
          }

          char str[3];
          snprintf(str, 3, "%.2d", sub_list[sub_idx]);
          xp_file[(*index)-3] = str[0];
          xp_file[(*index)-2] = str[1];
          // (if not in index, error)
        }
        dialog_index = 0;
      }

      if (dialog_index == 0 && dialog_param[0] == c)
        dialog_index = 1;
    } // if sub_list != 0

    if(*index >= XP_MAX_FILE_LEN) {
      printf("Error: XML definition too long.\n");
      fclose(f);
      return 0; 
    }
  }
  fclose(f);

  return 1;
} // xp_open_and_buffer_file


// return 1 on success, 0 on error 
int xp_set_xml_buffer_from_file(char * filename, int dumpxml)
{
  int index = 0;

  int result = xp_open_and_buffer_file(filename, "", &index, 0, 0);
  if (dumpxml)
    printf("%s", &xp_file);

  xp_file[index++] = 0;
  xp_stack = 0;
  xp_position[xp_stack] = xp_file;

  if (!result)
    return 0;
  
  if(strstr(xp_position[xp_stack], "<?xml") != xp_position[xp_stack]) return 0;
  if(!strstr(xp_position[xp_stack], "?>")) return 0;
  xp_position[xp_stack] = xp_position[xp_stack] + 2;

  return 1;
}

char * xp_open_element(int index)
{
  return xp_open_element_skip_control(index, 1);
}

char * debug_buffer(char * ptr)
{
    static char          buf[50];
    int i=0;

    if (ptr) {
      for (; (i<49&&ptr[i]!=0); i++) {
        buf[i] = ptr[i];
      }
    }
  buf[i] = 0;
  return buf;
}

char * xp_open_element_skip_control(int index, int skip_scenario)
{
  char * ptr = xp_position[xp_stack];
  int level = 0;
  static char name[XP_MAX_NAME_LEN];

  DEBUG("xp_open_element: index = %d, skip_scenario = %d, xp_stack = %d, ptr = '%s'\n", index, skip_scenario, xp_stack, debug_buffer(ptr));

  while(*ptr) {
    if (*ptr == '<') {
      if ((*(ptr+1) == '!') && 
        (*(ptr+2) == '[') &&
        (strstr(ptr,"<![CDATA[") == ptr)) {
          char * cdata_end = strstr(ptr, "]]>");
          if(!cdata_end) return NULL;
          ptr = cdata_end + 3;
      } else if ((*(ptr+1) == '!') && 
        (*(ptr+2) == '-') &&
        (strstr(ptr,"<!--") == ptr)) {
          char * comment_end = strstr(ptr, "-->");
          if(!comment_end) return NULL;
          ptr = comment_end + 3;
      } else if (strstr(ptr,"<?xml") == ptr) {
        char * xml_end = strstr(ptr, "?>");
        if(!xml_end) return NULL;
        ptr = xml_end + 2;
      } else if (strstr(ptr,"<!DOCTYPE") == ptr) {
        char * doctype_end = strstr(ptr, ">");
        if(!doctype_end) return NULL;
        ptr = doctype_end + 1;
      } else if ((skip_scenario) && (strstr(ptr,"<scenario") == ptr) ) {
        char * scenario_end = strstr(ptr, ">");
        DEBUG("  Skipping over embedded <scenario> tag (%d bytes); level = %d, index = %d\n", scenario_end-ptr, level, index);
        if(!scenario_end) return NULL;
        ptr = scenario_end + 1;
      } else if (*(ptr+1) == '/') {
        if ((*(ptr+2) == 's') && (*(ptr+3) == 'c') && (strstr(ptr,"</scenario") == ptr) ) {
          char * scenario_end = strstr(ptr, ">");
          DEBUG("  Skipping over embedded </scenario> tag (%d bytes); level = %d, index = %d\n", scenario_end-ptr, level, index);
          if(!scenario_end) return NULL;
          ptr = scenario_end + 1;
        }
        else {
          level--;
          if(level < 0) return NULL;
        }
      } else {
        // It's a < but not special
        if(level==0) {
          if (index) {
            index --;
            DEBUG("  < found, level=0, index>0: decrementing index; level = %d, index = %d, ptr='%s'\n", level, index, debug_buffer(ptr));
          } else {
            char * end = xp_find_start_tag_end(ptr + 1);
            char * p;
            if(!end) return NULL;

            p = strchr(ptr, ' ');
            if(p && (p < end))  { end = p; }
            p = strchr(ptr, '\t');
            if(p && (p < end))  { end = p; }
            p = strchr(ptr, '\r');
            if(p && (p < end))  { end = p; }
            p = strchr(ptr, '\n');
            if(p && (p < end))  { end = p; }
            p = strchr(ptr, '/');
            if(p && (p < end))  { end = p; }

            memcpy(name, ptr + 1, end-ptr-1);
            name[end-ptr-1] = 0;

            xp_position[++xp_stack] = end;
            DEBUG("  Returning element '%s'; level = %d, index = %d, xp_stack = %d, ptr='%s'\n", name, level, index, xp_stack, debug_buffer(ptr));
            return name;
          }
        }

        /* We want to skip over this particular element .*/
        DEBUG("  We want to skip over this particular element, calling xp_find_start_tag_end; level = %d, index = %d, ptr='%s'\n", level, index, debug_buffer(ptr));
        ptr = xp_find_start_tag_end(ptr + 1);
        if (ptr) ptr--;
        level ++;
      }
    } else if((*ptr == '/') && (*(ptr+1) == '>')) {
      level --;
      DEBUG("  Found />; level<0 => return null; level = %d, index = %d, ptr='%s'\n", level, index, debug_buffer(ptr));
      if(level < 0) return NULL;
    }
    ptr++;
  }

  DEBUG("  Reached end of file, returning NULL; level = %d, index = %d\n", level, index);
  return NULL;
}

void xp_close_element()
{
  if(xp_stack) {
    xp_stack--;
  }
}

void xp_root()
{
  xp_stack = 0;
}

char * xp_get_value(const char * name)
{
  int         index = 0;
  static char buffer[XP_MAX_FILE_LEN + 1]; 
  char      * ptr, *end, *check;
  
  end = xp_find_start_tag_end(xp_position[xp_stack] + 1);
  if(!end) return NULL;

  ptr = xp_position[xp_stack];
  
  while(*ptr) {
    ptr = strstr(ptr, name);

    if(!ptr) return NULL;
    if(ptr > end) return NULL;
    // FIXME: potential BUG in parser: we must retrieve full word,
    // so the use of strstr as it is is not enough.
    // we should check that the retrieved word is not a piece of another one.
    check = ptr-1;
    if(check >= xp_position[xp_stack])
    {
      if((*check != '\r') && 
         (*check != '\n') && 
         (*check != '\t') && 
         (*check != ' ' )) { ptr += strlen(name); continue; }
    }
    else
      return(NULL);

    ptr += strlen(name);
    while((*ptr == '\r') || 
          (*ptr == '\n') || 
          (*ptr == '\t') || 
          (*ptr == ' ' )    ) { ptr ++; }
    if(*ptr != '=') continue;
    ptr ++;
    while((*ptr == '\r') || 
          (*ptr == '\n') || 
          (*ptr == '\t') || 
          (*ptr ==  ' ')    ) { ptr ++; }
    ptr++;
    if(*ptr) {
      while(*ptr) {
	if (*ptr == '\\') {
	  ptr++;
	  switch(*ptr) {
	    case '\\':
	      buffer[index++] = '\\';
	      break;
	    case '"':
	      buffer[index++] = '"';
	      break;
	    case 'n':
	      buffer[index++] = '\n';
	      break;
	    case 't':
	      buffer[index++] = '\t';
	      break;
	    case 'r':
	      buffer[index++] = '\r';
	      break;
	    default:
	      buffer[index++] = '\\';
	      buffer[index++] = *ptr;
	      break;
	  }
	  ptr++;
	} else if (*ptr=='"') {
	  break;
	} else {
	  buffer[index++] = *ptr++;
	}
        if(index > XP_MAX_FILE_LEN) return NULL;
      }
      buffer[index] = 0;
      return buffer;
    }
  }
  return NULL;
}

char * xp_get_cdata()
{
  static char buffer[XP_MAX_FILE_LEN + 1]; 
  char      * end = xp_find_local_end();
  char      * ptr;
  
  ptr = strstr(xp_position[xp_stack],"<![CDATA[");
  if(!ptr) { return NULL; }
  ptr += 9;
  if(ptr > end) return NULL;
  end = strstr(ptr, "]]>");
  if(!end) { return NULL; }
  if((end -ptr) > XP_MAX_FILE_LEN) return NULL;
  memcpy(buffer, ptr, (end-ptr));
  buffer[end-ptr] = 0;
  return buffer;
}

int xp_get_content_length(char * P_buffer) 
{
  char * L_ctl_hdr;
  int    L_content_length = -1 ; 
  unsigned char   short_form;

  short_form = 0;

  L_ctl_hdr = strstr(P_buffer, "\nContent-Length:");
  if(!L_ctl_hdr) {L_ctl_hdr = strstr(P_buffer, "\nContent-length:"); }
  if(!L_ctl_hdr) {L_ctl_hdr = strstr(P_buffer, "\ncontent-Length:"); }
  if(!L_ctl_hdr) {L_ctl_hdr = strstr(P_buffer, "\ncontent-length:"); }
  if(!L_ctl_hdr) {L_ctl_hdr = strstr(P_buffer, "\nCONTENT-LENGTH:"); }
  if(!L_ctl_hdr) {L_ctl_hdr = strstr(P_buffer, "\nl:"); short_form = 1;}

  if( L_ctl_hdr ){
    if (short_form) {
      L_ctl_hdr += 3;
    } else {
      L_ctl_hdr += 16;
    }
    while(isspace(*L_ctl_hdr)) L_ctl_hdr++;
    sscanf(L_ctl_hdr, "%d", &L_content_length);
  } 
  // L_content_length = -1 the message does not contain content-length
  return (L_content_length); 
}

