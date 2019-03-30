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
#include <fc_config.h>
#endif

#include <curl/curl.h>

#ifdef FREECIV_MSWINDOWS
#include <windows.h>
#endif

/* utility */
#include "fcintl.h"
#include "ioz.h"
#include "mem.h"
#include "rand.h"
#include "registry.h"

#include "netfile.h"

struct netfile_post {
  struct curl_httppost *first;
  struct curl_httppost *last;
};

typedef size_t (*netfile_write_cb)(char *ptr, size_t size, size_t nmemb, void *userdata);

static char error_buf_curl[CURL_ERROR_SIZE];

/*******************************************************************//**
  Set handle to usable state.
***********************************************************************/
static CURL *netfile_init_handle(void)
{
  /* Consecutive transfers can use same handle for better performance */
  static CURL *handle = NULL;

  if (handle == NULL) {
    handle = curl_easy_init();
  } else {
    curl_easy_reset(handle);
  }

  error_buf_curl[0] = '\0';
  curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, error_buf_curl);

  return handle;
}

/*******************************************************************//**
  Curl write callback to store received file to memory.
***********************************************************************/
static size_t netfile_memwrite_cb(char *ptr, size_t size, size_t nmemb, void *userdata)
{
  struct netfile_write_cb_data *data = (struct netfile_write_cb_data *)userdata;

  if (size > 0) {
    data->mem = fc_realloc(data->mem, data->size + size * nmemb);
    memcpy(data->mem + data->size, ptr, size * nmemb);
    data->size += size * nmemb;
  }

  return size * nmemb;
}

/*******************************************************************//**
  Fetch file from given URL to given file stream. This is core
  function of netfile module.
***********************************************************************/
static bool netfile_download_file_core(const char *URL, FILE *fp,
                                       struct netfile_write_cb_data *mem_data,
                                       nf_errmsg cb, void *data)
{
  CURLcode curlret;
  struct curl_slist *headers = NULL;
  static CURL *handle;
  bool ret = TRUE;

  handle = netfile_init_handle();

  headers = curl_slist_append(headers,"User-Agent: Freeciv/" VERSION_STRING);

  curl_easy_setopt(handle, CURLOPT_URL, URL);
  if (mem_data != NULL) {
    mem_data->mem = NULL;
    mem_data->size = 0;
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, netfile_memwrite_cb);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, mem_data);
  } else {
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, fp);
  }
  curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(handle, CURLOPT_FAILONERROR, 1);

  curlret = curl_easy_perform(handle);

  curl_slist_free_all(headers);

  if (curlret != CURLE_OK) {
    if (cb != NULL) {
      char buf[2048 + CURL_ERROR_SIZE];

      fc_snprintf(buf, sizeof(buf),
                  /* TRANS: first %s is URL, second is Curl error message
                   * (not in Freeciv translation domain) */
                  _("Failed to fetch %s: %s"), URL,
                  error_buf_curl[0] != '\0' ? error_buf_curl : curl_easy_strerror(curlret));
      cb(buf, data);
    }

    ret = FALSE;
  }

  return ret;
}

/*******************************************************************//**
  Fetch section file from net
***********************************************************************/
struct section_file *netfile_get_section_file(const char *URL,
                                              nf_errmsg cb, void *data)
{
  bool success;
  struct section_file *out = NULL;
  struct netfile_write_cb_data mem_data;
  fz_FILE *file;

  success = netfile_download_file_core(URL, NULL, &mem_data, cb, data);

  if (success) {
    file = fz_from_memory(mem_data.mem, mem_data.size, TRUE);

    out = secfile_from_stream(file, TRUE);
  }

  return out;
}

/*******************************************************************//**
  Fetch file from given URL and save as given filename.
***********************************************************************/
bool netfile_download_file(const char *URL, const char *filename,
                           nf_errmsg cb, void *data)
{
  bool success;
  FILE *fp;

  fp = fc_fopen(filename, "w+b");

  if (fp == NULL) {
    if (cb != NULL) {
      char buf[2048];

      fc_snprintf(buf, sizeof(buf),
                  _("Could not open %s for writing"), filename);
      cb(buf, data);
    }
    return FALSE;
  }

  success = netfile_download_file_core(URL, fp, NULL, cb, data);

  fclose(fp);

  return success;
}

/*******************************************************************//** 
  Allocate netfile_post
***********************************************************************/
struct netfile_post *netfile_start_post(void)
{
  return fc_calloc(1, sizeof(struct netfile_post));
}

/*******************************************************************//** 
  Add one entry to netfile post form
***********************************************************************/
void netfile_add_form_str(struct netfile_post *post,
                          const char *name, const char *val)
{
  curl_formadd(&post->first, &post->last,
               CURLFORM_COPYNAME, name,
               CURLFORM_COPYCONTENTS, val,
               CURLFORM_END);
}

/*******************************************************************//** 
  Add one integer entry to netfile post form
***********************************************************************/
void netfile_add_form_int(struct netfile_post *post,
                          const char *name, const int val)
{
  char buf[50];

  fc_snprintf(buf, sizeof(buf), "%d", val);
  netfile_add_form_str(post, name, buf);
}

/*******************************************************************//** 
  Free netfile_post resources
***********************************************************************/
void netfile_close_post(struct netfile_post *post)
{
  curl_formfree(post->first);
  FC_FREE(post);
}

/*******************************************************************//** 
  Dummy write callback used only to make sure curl's default write
  function does not get used as we don't want reply to stdout
***********************************************************************/
static size_t dummy_write(void *buffer, size_t size, size_t nmemb, void *userp)
{
  return size * nmemb;
}

/*******************************************************************//** 
  Send HTTP POST
***********************************************************************/
bool netfile_send_post(const char *URL, struct netfile_post *post,
                       FILE *reply_fp, struct netfile_write_cb_data *mem_data,
                       const char *addr)
{
  CURLcode curlret;
  long http_resp;
  struct curl_slist *headers = NULL;
  static CURL *handle;

  handle = netfile_init_handle();

  headers = curl_slist_append(headers,"User-Agent: Freeciv/" VERSION_STRING);

  curl_easy_setopt(handle, CURLOPT_URL, URL);
  curl_easy_setopt(handle, CURLOPT_HTTPPOST, post->first);
  if (mem_data != NULL) {
    mem_data->mem = NULL;
    mem_data->size = 0;
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, netfile_memwrite_cb);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, mem_data);
  } else if (reply_fp == NULL) {
    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, dummy_write);
  } else {
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, reply_fp);
  }
  if (addr != NULL) {
    curl_easy_setopt(handle, CURLOPT_INTERFACE, addr);
  }
  curl_easy_setopt(handle, CURLOPT_HTTPHEADER, headers);

  curlret = curl_easy_perform(handle);

  curl_slist_free_all(headers);

  if (curlret != CURLE_OK) {
    return FALSE;
  }

  curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &http_resp);

  if (http_resp != 200) {
    return FALSE;
  }

  return TRUE;
}
