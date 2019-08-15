/*
 * Copyright (C) 2017 CAMELab
 *
 * This file is part of SimpleSSD.
 *
 * SimpleSSD is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SimpleSSD is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with SimpleSSD.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#ifndef __HTTP_H__
#define __HTTP_H__

#include <inttypes.h>
#include <stdlib.h>

typedef enum {
  HTTP_TYPE_INT32,
  HTTP_TYPE_INT64,
  HTTP_TYPE_STRING,
  HTTP_TYPE_FLOAT32,
  HTTP_TYPE_FLOAT64,
} HTTP_PARAM_TYPE;

typedef struct {
  HTTP_PARAM_TYPE buffer_type;
  void *buffer;
  int buffer_length;  // Only valid when type is string or wstring
} HTTP_PARAM;

typedef enum {
  HTTP_OK,
  HTTP_INIT_ERROR,
  HTTP_CONNECTION_ERROR,
  HTTP_TIMEOUT,
  HTTP_QUERY_ERROR,
  HTTP_NO_DATA,
  HTTP_JSON_ERROR,
  HTTP_INVALID_PARAM,
} HTTP_RESPONSE;

typedef struct {
  char *host;
  int port;

  // Input
  int sql;
  int n_params;
  HTTP_PARAM *param;

  // Internal data
  // DO NOT ACCESS OUTSIDE OF HTTP MODULE
  char *raw_response;
  size_t raw_size;
  int inserted_id;
  int affected_rows;
  int row_to_fetch;
  void *json;

  // Output pointers
  int n_result;
  HTTP_PARAM *result;
} HTTP_HANDLE;

int http_request(HTTP_HANDLE *handle);
int http_fetch_result(HTTP_HANDLE *handle);
int http_cleanup(HTTP_HANDLE *handle);
int http_error(HTTP_HANDLE *handle);

#endif
