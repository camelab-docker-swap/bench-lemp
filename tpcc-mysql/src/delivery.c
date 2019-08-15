/*
 * -*-C-*-
 * delivery.pc
 * corresponds to A.4 in appendix A
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "http.h"
#include "spt_proc.h"
#include "tpc.h"

// Defined in main.c
extern char connect_string[DB_STRING_MAX];

#define NNULL ((void *)0)

int delivery(int t_num, int port, int w_id_arg, int o_carrier_id_arg) {
  int w_id = w_id_arg;
  int o_carrier_id = o_carrier_id_arg;
  int d_id;
  int c_id;
  int no_o_id;
  float ol_total;
  char datetime[81];

  int proceed = 0;

  HTTP_PARAM param[4];
  HTTP_PARAM column[1];
  HTTP_HANDLE handle;

  handle.host = connect_string;
  handle.port = port;
  handle.param = param;
  handle.result = column;

  /*EXEC SQL WHENEVER SQLERROR GOTO sqlerr;*/

  gettimestamp(datetime, STRFTIME_FORMAT, TIMESTAMP_LEN);

  /* For each district in warehouse */
  /* printf("W: %d\n", w_id); */

  for (d_id = 1; d_id <= DIST_PER_WARE; d_id++) {
    proceed = 1;
    /*EXEC_SQL SELECT COALESCE(MIN(no_o_id),0) INTO :no_o_id
                    FROM new_orders
                    WHERE no_d_id = :d_id AND no_w_id = :w_id;*/
    handle.sql = 25;

    memset(param, 0, sizeof(HTTP_PARAM) * 2); /* initialize */
    param[0].buffer_type = HTTP_TYPE_INT32;
    param[0].buffer = &d_id;
    param[1].buffer_type = HTTP_TYPE_INT32;
    param[1].buffer = &w_id;

    handle.n_params = 1;

    if (http_request(&handle))
      goto sqlerr;

    memset(column, 0, sizeof(HTTP_PARAM) * 1); /* initialize */
    column[0].buffer_type = HTTP_TYPE_INT32;
    column[0].buffer = &no_o_id;

    handle.n_result = 1;

    if (http_fetch_result(&handle))
      goto sqlerr;

    http_cleanup(&handle);

    if (no_o_id == 0)
      continue;
    proceed = 2;
    /*EXEC_SQL DELETE FROM new_orders WHERE no_o_id = :no_o_id AND no_d_id =
      :d_id AND no_w_id = :w_id;*/
    handle.sql = 26;

    memset(param, 0, sizeof(HTTP_PARAM) * 3); /* initialize */
    param[0].buffer_type = HTTP_TYPE_INT32;
    param[0].buffer = &no_o_id;
    param[1].buffer_type = HTTP_TYPE_INT32;
    param[1].buffer = &d_id;
    param[2].buffer_type = HTTP_TYPE_INT32;
    param[2].buffer = &w_id;

    handle.n_params = 3;

    if (http_request(&handle))
      goto sqlerr;

    http_cleanup(&handle);

    proceed = 3;
    /*EXEC_SQL SELECT o_c_id INTO :c_id FROM orders
                    WHERE o_id = :no_o_id AND o_d_id = :d_id
                    AND o_w_id = :w_id;*/
    handle.sql = 27;

    memset(param, 0, sizeof(HTTP_PARAM) * 3); /* initialize */
    param[0].buffer_type = HTTP_TYPE_INT32;
    param[0].buffer = &no_o_id;
    param[1].buffer_type = HTTP_TYPE_INT32;
    param[1].buffer = &d_id;
    param[2].buffer_type = HTTP_TYPE_INT32;
    param[2].buffer = &w_id;

    handle.n_params = 3;

    if (http_request(&handle))
      goto sqlerr;

    memset(column, 0, sizeof(HTTP_PARAM) * 1); /* initialize */
    column[0].buffer_type = HTTP_TYPE_INT32;
    column[0].buffer = &c_id;

    handle.n_result = 1;

    if (http_fetch_result(&handle))
      goto sqlerr;

    http_cleanup(&handle);

    proceed = 4;
    /*EXEC_SQL UPDATE orders SET o_carrier_id = :o_carrier_id
                    WHERE o_id = :no_o_id AND o_d_id = :d_id AND
                    o_w_id = :w_id;*/
    handle.sql = 28;

    memset(param, 0, sizeof(HTTP_PARAM) * 4); /* initialize */
    param[0].buffer_type = HTTP_TYPE_INT32;
    param[0].buffer = &o_carrier_id;
    param[1].buffer_type = HTTP_TYPE_INT32;
    param[1].buffer = &no_o_id;
    param[2].buffer_type = HTTP_TYPE_INT32;
    param[2].buffer = &d_id;
    param[3].buffer_type = HTTP_TYPE_INT32;
    param[3].buffer = &w_id;

    handle.n_params = 4;

    if (http_request(&handle))
      goto sqlerr;

    http_cleanup(&handle);

    proceed = 5;
    /*EXEC_SQL UPDATE order_line
                    SET ol_delivery_d = :datetime
                    WHERE ol_o_id = :no_o_id AND ol_d_id = :d_id AND
                    ol_w_id = :w_id;*/
    handle.sql = 29;

    memset(param, 0, sizeof(HTTP_PARAM) * 4); /* initialize */
    param[0].buffer_type = HTTP_TYPE_STRING;
    param[0].buffer = datetime;
    param[0].buffer_length = strlen(datetime);
    param[1].buffer_type = HTTP_TYPE_INT32;
    param[1].buffer = &no_o_id;
    param[2].buffer_type = HTTP_TYPE_INT32;
    param[2].buffer = &d_id;
    param[3].buffer_type = HTTP_TYPE_INT32;
    param[3].buffer = &w_id;

    handle.n_params = 4;

    if (http_request(&handle))
      goto sqlerr;

    http_cleanup(&handle);

    proceed = 6;
    /*EXEC_SQL SELECT SUM(ol_amount) INTO :ol_total
                    FROM order_line
                    WHERE ol_o_id = :no_o_id AND ol_d_id = :d_id
                    AND ol_w_id = :w_id;*/
    handle.sql = 30;

    memset(param, 0, sizeof(HTTP_PARAM) * 3); /* initialize */
    param[0].buffer_type = HTTP_TYPE_INT32;
    param[0].buffer = &no_o_id;
    param[1].buffer_type = HTTP_TYPE_INT32;
    param[1].buffer = &d_id;
    param[2].buffer_type = HTTP_TYPE_INT32;
    param[2].buffer = &w_id;

    handle.n_params = 3;

    if (http_request(&handle))
      goto sqlerr;

    memset(column, 0, sizeof(HTTP_PARAM) * 1); /* initialize */
    column[0].buffer_type = HTTP_TYPE_FLOAT32;
    column[0].buffer = &ol_total;

    handle.n_result = 1;

    if (http_fetch_result(&handle))
      goto sqlerr;

    http_cleanup(&handle);

    proceed = 7;
    /*EXEC_SQL UPDATE customer SET c_balance = c_balance + :ol_total ,
                                 c_delivery_cnt = c_delivery_cnt + 1
                    WHERE c_id = :c_id AND c_d_id = :d_id AND
                    c_w_id = :w_id;*/
    handle.sql = 31;

    memset(param, 0, sizeof(HTTP_PARAM) * 4); /* initialize */
    param[0].buffer_type = HTTP_TYPE_FLOAT32;
    param[0].buffer = &ol_total;
    param[1].buffer_type = HTTP_TYPE_INT32;
    param[1].buffer = &c_id;
    param[2].buffer_type = HTTP_TYPE_INT32;
    param[2].buffer = &d_id;
    param[3].buffer_type = HTTP_TYPE_INT32;
    param[3].buffer = &w_id;

    handle.n_params = 4;

    if (http_request(&handle))
      goto sqlerr;

    http_cleanup(&handle);

    /*EXEC_SQL COMMIT WORK;*/

    /* printf("D: %d, O: %d, time: %d\n", d_id, o_id, tad); */
  }
  /*EXEC_SQL COMMIT WORK;*/
  return (1);

sqlerr:
  fprintf(stderr, "delivery %d:%d\n", t_num, proceed);
  /*EXEC SQL WHENEVER SQLERROR GOTO sqlerrerr;*/
  /*EXEC_SQL ROLLBACK WORK;*/
  http_error(&handle);
  http_cleanup(&handle);
sqlerrerr:
  return (0);
}
