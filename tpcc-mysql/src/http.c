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

#include "http.h"

#include <string.h>

#include <curl/curl.h>
#include <json-c/json.h>

static size_t http_writefunc(void *data, size_t size, size_t nmemb,
                             void *context) {
  size_t read = size * nmemb;
  HTTP_HANDLE *handle = (HTTP_HANDLE *)context;

  // First call
  if (handle->raw_response == NULL) {
    handle->raw_response = (char *)calloc(1, read + 1);
  }
  else {
    handle->raw_response =
        (char *)realloc(handle->raw_response, handle->raw_size + read + 1);
  }

  memcpy(handle->raw_response + handle->raw_size, data, read);

  handle->raw_size += read;
  handle->raw_response[handle->raw_size] = NULL;

  return read;
}

int http_request(HTTP_HANDLE *handle) {
  CURL *curl = NULL;
  CURLcode res;
  int result = HTTP_INIT_ERROR;
  char *postbody;
  int i;

  // Clear handle
  handle->inserted_id = 0;
  handle->affected_rows = 0;
  handle->row_to_fetch = 0;
  handle->json = NULL;
  handle->raw_response = NULL;
  handle->raw_size = 0;

  // Generate JSON request from param
  json_object *json;

  json = json_object_new_object();

  json_object *sql = json_object_new_int(handle->sql);
  json_object *array = json_object_new_array();
  json_object **param =
      (json_object **)calloc(handle->n_params, sizeof(json_object *));

  json_object_object_add(json, "id", sql);

  for (i = 0; i < handle->n_params; i++) {
    switch (handle->param[i].buffer_type) {
      case HTTP_TYPE_INT32:
        param[i] = json_object_new_int(*(int32_t *)handle->param[i].buffer);
        break;
      case HTTP_TYPE_INT64:
        param[i] = json_object_new_int64(*(int64_t *)handle->param[i].buffer);
        break;
      case HTTP_TYPE_STRING:
        param[i] = json_object_new_string((char *)handle->param[i].buffer);
        break;
      case HTTP_TYPE_FLOAT32:
        param[i] = json_object_new_double(*(float *)handle->param[i].buffer);
        break;
      case HTTP_TYPE_FLOAT64:
        param[i] = json_object_new_double(*(double *)handle->param[i].buffer);
        break;
    }

    json_object_array_add(array, param[i]);
  }

  json_object_object_add(json, "param", array);

  postbody = json_object_to_json_string_ext(json, JSON_C_TO_STRING_PLAIN);

  // HTTP
  curl = curl_easy_init();

  if (curl) {
    struct curl_slist *hs = NULL;

    hs = curl_slist_append(hs, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, handle->host);
    curl_easy_setopt(curl, CURLOPT_PORT, handle->port);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postbody);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, handle);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_writefunc);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 0);

    // Send request
    res = curl_easy_perform(curl);
    switch (res) {
      case CURLE_OK:
        result = HTTP_OK;
        break;
      case CURLE_OPERATION_TIMEDOUT:
        result = HTTP_TIMEOUT;
        break;
      default:
        result = HTTP_CONNECTION_ERROR;
        break;
    }

    curl_slist_free_all(hs);

    if (handle->raw_response == NULL) {
      result = HTTP_CONNECTION_ERROR;
    }

    if (result == HTTP_OK) {
      json_object *temp;

      // Now we have data in handle
      json_object *response = json_tokener_parse(handle->raw_response);

      // Get result
      if (json_object_object_get_ex(response, "error", &temp)) {
        int error = json_object_get_int(temp);

        switch (error) {
          case 0:
            result = HTTP_OK;
            break;
          default:
            result = HTTP_QUERY_ERROR;
            break;
        }
      }
      if (json_object_object_get_ex(response, "inserted_id", &temp)) {
        handle->inserted_id = json_object_get_int(temp);
      }
      if (json_object_object_get_ex(response, "affected_rows", &temp)) {
        handle->affected_rows = json_object_get_int(temp);
      }

      handle->json = (void *)response;  // Store all object
    }

    curl_easy_cleanup(curl);
  }

  // Free all json objects
  json_object_put(sql);

  for (i = 0; i < handle->n_params; i++) {
    json_object_put(param[i]);
  }

  free(param);
  json_object_put(array);
  json_object_put(json);

  return result;
}

int http_fetch_result(HTTP_HANDLE *handle) {
  int i;

  // We must have result buffer
  if (handle->n_result == 0 || handle->result == NULL || handle->json == NULL) {
    return HTTP_INVALID_PARAM;
  }

  // Get result array
  json_object *result;

  if (json_object_object_get_ex((json_object *)handle->json, "result",
                                &result)) {
    // We have result
    int length = json_object_array_length(result);

    if (handle->row_to_fetch < length) {
      int ret = HTTP_OK;

      json_object *row =
          json_object_array_get_idx(result, handle->row_to_fetch);

      handle->row_to_fetch++;

      // Mapping data with row
      int row_length = json_object_array_length(row);

      for (i = 0; i < row_length; i++) {
        // Handling smaller result buffer
        if (i >= handle->n_result) {
          break;
        }

        // Get item
        json_object *item = json_object_array_get_idx(row, i);
        json_type type = json_object_get_type(item);

        // Validate type and get value
        if (handle->result[i].buffer_type == HTTP_TYPE_INT32 &&
            type == json_type_int) {
          *(int32_t *)handle->result[i].buffer = json_object_get_int(item);
        }
        else if (handle->result[i].buffer_type == HTTP_TYPE_INT64 &&
                 type == json_type_int) {
          *(int64_t *)handle->result[i].buffer = json_object_get_int64(item);
        }
        else if (handle->result[i].buffer_type == HTTP_TYPE_STRING &&
                 type == json_type_string) {
          strcpy((char *)handle->result[i].buffer,
                 json_object_get_string(item));
        }
        else if (handle->result[i].buffer_type == HTTP_TYPE_FLOAT32 &&
                 type == json_type_double) {
          *(float *)handle->result[i].buffer = json_object_get_double(item);
        }
        else if (handle->result[i].buffer_type == HTTP_TYPE_FLOAT32 &&
                 type == json_type_string) {
          *(float *)handle->result[i].buffer =
              strtof(json_object_get_string(item), NULL);
        }
        else if (handle->result[i].buffer_type == HTTP_TYPE_FLOAT64 &&
                 type == json_type_double) {
          *(double *)handle->result[i].buffer = json_object_get_double(item);
        }
        else if (handle->result[i].buffer_type == HTTP_TYPE_FLOAT64 &&
                 type == json_type_string) {
          *(double *)handle->result[i].buffer =
              strtod(json_object_get_string(item), NULL);
        }
        else {
          ret = HTTP_INVALID_PARAM;

          // Keep parsing
        }
      }

      return ret;
    }

    return HTTP_NO_DATA;
  }

  return HTTP_INVALID_PARAM;
}

int http_cleanup(HTTP_HANDLE *handle) {
  // Clean everything
  handle->inserted_id = 0;
  handle->affected_rows = 0;
  handle->row_to_fetch = 0;

  if (handle->raw_response) {
    free(handle->raw_response);

    handle->raw_response = NULL;
    handle->raw_size = 0;
  }

  if (handle->json) {
    json_object_put((json_object *)handle->json);

    handle->json = NULL;
  }
}

// Call this before cleanup
int http_error(HTTP_HANDLE *handle) {
  int i;

  if (handle->json) {
    json_object *temp;

    if (json_object_object_get_ex((json_object *)handle->json, "error",
                                  &temp)) {
      int error = json_object_get_int(temp);

      if (error > 0) {
        fprintf(stderr, "Error: %d: ", error);

        if (json_object_object_get_ex((json_object *)handle->json, "message",
                                      &temp)) {
          fprintf(stderr, "%s\n", json_object_get_string(temp));
        }
      }
    }
    else {
      fprintf(stderr, "Response: %s\n", handle->raw_response);
    }
  }
}