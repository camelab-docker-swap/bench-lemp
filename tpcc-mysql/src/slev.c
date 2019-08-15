/*
 * -*-C-*-
 * slev.pc
 * corresponds to A.5 in appendix A
 */

#include <stdio.h>
#include <string.h>

#include "http.h"
#include "spt_proc.h"
#include "tpc.h"

// Defined in main.c
extern char connect_string[DB_STRING_MAX];

/*
 * the stock level transaction
 */
int slev(int t_num, int port, int w_id_arg, /* warehouse id */
         int d_id_arg,                      /* district id */
         int level_arg                      /* stock level */
) {
  int w_id = w_id_arg;
  int d_id = d_id_arg;
  int level = level_arg;
  int d_next_o_id;
  int i_count;
  int ol_i_id;

  HTTP_PARAM param[4];
  HTTP_PARAM column[1];
  HTTP_HANDLE handle;

  handle.host = connect_string;
  handle.port = port;
  handle.param = param;
  handle.result = column;

  HTTP_PARAM param2[3];
  HTTP_PARAM column2[1];
  HTTP_HANDLE handle2;

  handle2.host = connect_string;
  handle2.port = port;
  handle2.param = param2;
  handle2.result = column2;

  /*EXEC SQL WHENEVER NOT FOUND GOTO sqlerr;*/
  /*EXEC SQL WHENEVER SQLERROR GOTO sqlerr;*/

  /* find the next order id */
#ifdef DEBUG
  printf("select 1\n");
#endif
  /*EXEC_SQL SELECT d_next_o_id
                  INTO :d_next_o_id
                  FROM district
                  WHERE d_id = :d_id
                  AND d_w_id = :w_id;*/
  handle.sql = 32;

  memset(param, 0, sizeof(HTTP_PARAM) * 2); /* initialize */
  param[0].buffer_type = HTTP_TYPE_INT32;
  param[0].buffer = &d_id;
  param[1].buffer_type = HTTP_TYPE_INT32;
  param[1].buffer = &w_id;

  handle.n_params = 2;

  if (http_request(&handle))
    goto sqlerr;

  memset(column, 0, sizeof(HTTP_PARAM) * 1); /* initialize */
  column[0].buffer_type = HTTP_TYPE_INT32;
  column[0].buffer = &d_next_o_id;

  handle.n_result = 1;

  if (http_fetch_result(&handle))
    goto sqlerr;

  http_cleanup(&handle);

  /* find the most recent 20 orders for this district */
  /*EXEC_SQL DECLARE ord_line CURSOR FOR
                  SELECT DISTINCT ol_i_id
                  FROM order_line
                  WHERE ol_w_id = :w_id
                  AND ol_d_id = :d_id
                  AND ol_o_id < :d_next_o_id
                  AND ol_o_id >= (:d_next_o_id - 20);

  EXEC_SQL OPEN ord_line;

  EXEC SQL WHENEVER NOT FOUND GOTO done;*/
  handle.sql = 33;

  memset(param, 0, sizeof(HTTP_PARAM) * 4); /* initialize */
  param[0].buffer_type = HTTP_TYPE_INT32;
  param[0].buffer = &w_id;
  param[1].buffer_type = HTTP_TYPE_INT32;
  param[1].buffer = &d_id;
  param[2].buffer_type = HTTP_TYPE_INT32;
  param[2].buffer = &d_next_o_id;
  param[3].buffer_type = HTTP_TYPE_INT32;
  param[3].buffer = &d_next_o_id;

  handle.n_params = 4;

  if (http_request(&handle))
    goto sqlerr;

  memset(column, 0, sizeof(HTTP_PARAM) * 1); /* initialize */
  column[0].buffer_type = HTTP_TYPE_INT32;
  column[0].buffer = &ol_i_id;

  handle.n_result = 1;

  for (;;) {
#ifdef DEBUG
    printf("fetch 1\n");
#endif
    /*EXEC_SQL FETCH ord_line INTO :ol_i_id;*/
    switch (http_fetch_result(&handle)) {
      case 0:  // SUCCESS
        break;
      case HTTP_NO_DATA:  // NO MORE DATA
        http_cleanup(&handle);
        goto done;
      case 1:  // ERROR
      default:
        http_cleanup(&handle);
        goto sqlerr;
    }

#ifdef DEBUG
    printf("select 2\n");
#endif

    /*EXEC_SQL SELECT count(*) INTO :i_count
            FROM stock
            WHERE s_w_id = :w_id
            AND s_i_id = :ol_i_id
            AND s_quantity < :level;*/
    handle2.sql = 34;

    memset(param2, 0, sizeof(HTTP_PARAM) * 3); /* initialize */
    param2[0].buffer_type = HTTP_TYPE_INT32;
    param2[0].buffer = &w_id;
    param2[1].buffer_type = HTTP_TYPE_INT32;
    param2[1].buffer = &ol_i_id;
    param2[2].buffer_type = HTTP_TYPE_INT32;
    param2[2].buffer = &level;

    handle2.n_params = 3;

    if (http_request(&handle2))
      goto sqlerr2;

    memset(column2, 0, sizeof(HTTP_PARAM) * 1); /* initialize */
    column2[0].buffer_type = HTTP_TYPE_INT32;
    column2[0].buffer = &i_count;

    handle2.n_result = 1;

    if (http_fetch_result(&handle2))
      goto sqlerr2;

    http_cleanup(&handle2);
  }

done:
  /*EXEC_SQL CLOSE ord_line;*/
  /*EXEC_SQL COMMIT WORK;*/

  return (1);

sqlerr:
  fprintf(stderr, "slev\n");
  /*EXEC SQL WHENEVER SQLERROR GOTO sqlerrerr;*/
  /*EXEC_SQL ROLLBACK WORK;*/
  http_error(&handle);
  http_cleanup(&handle);

  return (0);
sqlerr2:
  fprintf(stderr, "slev2\n");
  /*EXEC SQL WHENEVER SQLERROR GOTO sqlerrerr;*/
  /*EXEC_SQL ROLLBACK WORK;*/
  http_error(&handle2);
  http_cleanup(&handle2);
  return (0);
}
