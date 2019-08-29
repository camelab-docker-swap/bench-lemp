/*
 * main.pc
 * driver for the tpcc transactions
 */

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <curl/curl.h>

#include "rthist.h"
#include "sb_percentile.h"
#include "sequence.h"
#include "spt_proc.h"
#include "tpc.h"
#include "trans_if.h"

int silent_flag = 0;
int init_flag = 1;
char connect_string[DB_STRING_MAX];
char db_host[DB_STRING_MAX];
char report_file[DB_STRING_MAX] = "";
FILE *freport_file = NULL;
char trx_file[DB_STRING_MAX] = "";
FILE *ftrx_file = NULL;
char seed_file[DB_STRING_MAX] = "";
FILE *fseed_file = NULL:

int num_ware;
int num_conn;
int lampup_time;
int measure_time;
int seed_method;

int num_node; /* number of servers that consists of cluster i.e. RAC (0:normal
                 mode)*/
#define NUM_NODE_MAX 8
char node_string[NUM_NODE_MAX][DB_STRING_MAX];

int time_count;
int PRINT_INTERVAL = 10;
int multi_schema = 0;
int multi_schema_offset = 0;

int success[5];
int late[5];
int retry[5];
int failure[5];

int *success2[5];
int *late2[5];
int *retry2[5];
int *failure2[5];

int success2_sum[5];
int late2_sum[5];
int retry2_sum[5];
int failure2_sum[5];

int prev_s[5];
int prev_l[5];

double max_rt[5];
double total_rt[5];
double cur_max_rt[5];

double prev_total_rt[5];

#define RTIME_NEWORD 5
#define RTIME_PAYMENT 5
#define RTIME_ORDSTAT 5
#define RTIME_DELIVERY 80
#define RTIME_SLEV 20

int rt_limit[5] = {RTIME_NEWORD, RTIME_PAYMENT, RTIME_ORDSTAT, RTIME_DELIVERY,
                   RTIME_SLEV};

sb_percentile_t local_percentile;

int activate_transaction;
int counting_on;

long clk_tck;

int valuable_flg = 0; /* "1" mean valuable ratio */

typedef struct {
  int number;
  int port;
} thread_arg;
int thread_main(thread_arg *);

void alarm_handler(int signum);
void alarm_dummy();

int main(int argc, char *argv[]) {
  int i, k, t_num, arg_offset, c;
  long j;
  float f;
  pthread_t *t;
  thread_arg *thd_arg;
  timer_t timer;
  struct itimerval itval;
  struct sigaction sigact;
  int port = 3306;
  int fd, seed;

  if (!init_flag) {
    printf("***************************************\n");
    printf("*** ###easy### TPC-C Load Generator ***\n");
    printf("***************************************\n");
  }

  /* initialize */
  hist_init();
  activate_transaction = 1;
  counting_on = 0;

  for (i = 0; i < 5; i++) {
    success[i] = 0;
    late[i] = 0;
    retry[i] = 0;
    failure[i] = 0;

    prev_s[i] = 0;
    prev_l[i] = 0;

    prev_total_rt[i] = 0.0;
    max_rt[i] = 0.0;
    total_rt[i] = 0.0;
  }

  /* dummy initialize*/
  num_ware = 1;
  num_conn = 10;
  lampup_time = 10;
  measure_time = 20;
  seed_method = 0;

  /* number of node (default 0) */
  num_node = 0;
  arg_offset = 0;

  clk_tck = sysconf(_SC_CLK_TCK);

  /* Parse args */

  while ((c = getopt(argc, argv, "s:h:P:w:c:r:l:d:i:f:t:m:o:0:1:2:3:4:")) != -1) {
    switch (c) {
	     case 's':
	    	silent_flag = 1;
	    	break;
      case 'h':
        if (!init_flag)
          printf("option h with value '%s'\n", optarg);
        strncpy(connect_string, optarg, DB_STRING_MAX);
        break;
      case 'f':
        if (!init_flag)
          printf("option f with value '%s'\n", optarg);
        strncpy(report_file, optarg, DB_STRING_MAX);
        break;
      case 't':
        if (!init_flag)
          printf("option t with value '%s'\n", optarg);
        strncpy(trx_file, optarg, DB_STRING_MAX);
        break;
      case 'w':
        if (!init_flag)
          printf("option w with value '%s'\n", optarg);
        num_ware = atoi(optarg);
        break;
      case 'c':
        if (!init_flag)
          printf("option c with value '%s'\n", optarg);
        num_conn = atoi(optarg);
        break;
      case 'r':
        if (!init_flag)
          printf("option r with value '%s'\n", optarg);
        lampup_time = atoi(optarg);
        break;
      case 'l':
        if (!init_flag)
          printf("option l with value '%s'\n", optarg);
        measure_time = atoi(optarg);
        break;
      case 'd':
        if (!init_flag)
          printf("option d with value '%s'\n", optarg);
        strncpy(seed_file, optarg, DB_STRING_MAX);
        seed_method = 1;
        break;
      case 'm':
        if (!init_flag)
          printf("option m (multiple schemas) with value '%s'\n", optarg);
        multi_schema = atoi(optarg);
        break;
      case 'o':
        if (!init_flag)
          printf("option o (multiple schemas offset) with value '%s'\n", optarg);
        multi_schema_offset = atoi(optarg);
        break;
      case 'i':
        if (!init_flag)
          printf("option i with value '%s'\n", optarg);
        PRINT_INTERVAL = atoi(optarg);
        break;
      case 'P':
        if (!init_flag)
          printf("option P with value '%s'\n", optarg);
        port = atoi(optarg);
        break;
      case '0':
        if (!init_flag)
          printf("option 0 (response time limit for transaction 0) '%s'\n",optarg);
        rt_limit[0] = atoi(optarg);
        break;
      case '1':
        if (!init_flag)
          printf("option 1 (response time limit for transaction 1) '%s'\n",optarg);
        rt_limit[1] = atoi(optarg);
        break;
      case '2':
        if (!init_flag)
          printf("option 2 (response time limit for transaction 2) '%s'\n",optarg);
        rt_limit[2] = atoi(optarg);
        break;
      case '3':
        if (!init_flag)
          printf("option 3 (response time limit for transaction 3) '%s'\n",optarg);
        rt_limit[3] = atoi(optarg);
        break;
      case '4':
        if (!init_flag)
          printf("option 4 (response time limit for transaction 4) '%s'\n",optarg);
        rt_limit[4] = atoi(optarg);
        break;
      case '?':
        printf("Usage: tpcc_start -h server_host -P port -w warehouses -c "
               "connections -r warmup_time -l running_time -i report_interval "
               "-f report_file -t trx_file\n");
        exit(0);
      default:
        printf("?? getopt returned character code 0%o ??\n", c);
    }
  }
  if (optind < argc) {
    printf("non-option ARGV-elements: ");
    while (optind < argc)
      printf("%s ", argv[optind++]);
    printf("\n");
  }

  if (valuable_flg == 1) {
    if ((atoi(argv[9 + arg_offset]) < 0) || (atoi(argv[10 + arg_offset]) < 0) ||
        (atoi(argv[11 + arg_offset]) < 0) ||
        (atoi(argv[12 + arg_offset]) < 0) ||
        (atoi(argv[13 + arg_offset]) < 0)) {
      fprintf(stderr, "\n expecting positive number of ratio parameters\n");
      exit(1);
    }
  }

  if (num_node > 0) {
    if (num_ware % num_node != 0) {
      fprintf(stderr, "\n [warehouse] value must be devided by [num_node].\n");
      exit(1);
    }
    if (num_conn % num_node != 0) {
      fprintf(stderr, "\n [connection] value must be devided by [num_node].\n");
      exit(1);
    }
  }

  if (strlen(report_file) > 0) {
    freport_file = fopen(report_file, "w+");
  }

  if (strlen(trx_file) > 0) {
    ftrx_file = fopen(trx_file, "w+");
  }

  // For getopt and curl
  int prefix = 0;

  while (1) {
    if (connect_string[prefix] == ' ') {
      prefix++;
    }
    else {
      break;
    }
  }

  if (prefix > 0) {
    char buf[DB_STRING_MAX];

    strcpy(buf, connect_string);
    strcpy(connect_string, buf + prefix);
  }

  if (!silent_flag) {
    printf("<Parameters>\n");
    printf("     [server]: ");
    printf("%s", connect_string);
    printf("\n");
    printf("     [port]: %d\n", port);
    printf("  [warehouse]: %d\n", num_ware);
    printf(" [connection]: %d\n", num_conn);
    printf("     [rampup]: %d (sec.)\n", lampup_time);
    printf("    [measure]: %d (sec.)\n", measure_time);
  

    if (valuable_flg == 1) {
      printf("      [ratio]: %d:%d:%d:%d:%d\n", atoi(argv[9 + arg_offset]),
            atoi(argv[10 + arg_offset]), atoi(argv[11 + arg_offset]),
            atoi(argv[12 + arg_offset]), atoi(argv[13 + arg_offset]));
    }
  }

  /* alarm initialize */
  time_count = 0;
  itval.it_interval.tv_sec = PRINT_INTERVAL;
  itval.it_interval.tv_usec = 0;
  itval.it_value.tv_sec = PRINT_INTERVAL;
  itval.it_value.tv_usec = 0;
  sigact.sa_handler = alarm_handler;
  sigact.sa_flags = 0;
  sigemptyset(&sigact.sa_mask);

  /* setup handler&timer */
  if (sigaction(SIGALRM, &sigact, NULL) == -1) {
    fprintf(stderr, "error in sigaction()\n");
    exit(1);
  }

  fd = open("/dev/urandom", O_RDONLY);
  if (fd == -1) {
    fd = open("/dev/random", O_RDONLY);
    if (fd == -1) {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      if (seed_method)
        fseed_file = fopen(seed_file, "r");
        fscanf(fseed_file, "%d", seed);
      else
        fseed_file = fopen(seed_file, "w+");
        seed = (tv.tv_sec ^ tv.tv_usec) * tv.tv_sec * tv.tv_usec ^ tv.tv_sec;
        if (fseed_file != NULL) {
          fprintf(fseed_file, "%d\n", seed);
        }

    }
    else {
      read(fd, &seed, sizeof(seed));
      close(fd);
    }
  }
  else {
    read(fd, &seed, sizeof(seed));
    close(fd);
  }
  SetSeed(seed);

  if (valuable_flg == 0) {
    seq_init(10, 10, 1, 1, 1); /* normal ratio */
  }
  else {
    seq_init(atoi(argv[9 + arg_offset]), atoi(argv[10 + arg_offset]),
             atoi(argv[11 + arg_offset]), atoi(argv[12 + arg_offset]),
             atoi(argv[13 + arg_offset]));
  }

  /* set up each counter */
  for (i = 0; i < 5; i++) {
    success2[i] = malloc(sizeof(int) * num_conn);
    late2[i] = malloc(sizeof(int) * num_conn);
    retry2[i] = malloc(sizeof(int) * num_conn);
    failure2[i] = malloc(sizeof(int) * num_conn);
    for (k = 0; k < num_conn; k++) {
      success2[i][k] = 0;
      late2[i][k] = 0;
      retry2[i][k] = 0;
      failure2[i][k] = 0;
    }
  }

  if (sb_percentile_init(&local_percentile, 100000, 1.0, 1e13))
    return NULL;

  /* set up threads */

  t = malloc(sizeof(pthread_t) * num_conn);
  if (t == NULL) {
    fprintf(stderr, "error at malloc(pthread_t)\n");
    exit(1);
  }
  thd_arg = malloc(sizeof(thread_arg) * num_conn);
  if (thd_arg == NULL) {
    fprintf(stderr, "error at malloc(thread_arg)\n");
    exit(1);
  }

  if (curl_global_init(CURL_GLOBAL_SSL)) {
    fprintf(stderr, "could not initialize curl library\n");
    exit(1);
  }

  /* EXEC SQL WHENEVER SQLERROR GOTO sqlerr; */

  for (t_num = 0; t_num < num_conn; t_num++) {
    thd_arg[t_num].port = port;
    thd_arg[t_num].number = t_num;
    pthread_create(&t[t_num], NULL, (void *)thread_main,
                   (void *)&(thd_arg[t_num]));
  }

  if (!silent_flag) {
    printf("\nRAMP-UP TIME.(%d sec.)\n", lampup_time);
    fflush(stdout);
  }
  sleep(lampup_time);
  if (!silent_flag) {
    printf("\nMEASURING START.\n\n");
    fflush(stdout);
  }

  /* sleep(measure_time); */
  /* start timer */

#ifndef _SLEEP_ONLY_
  if (setitimer(ITIMER_REAL, &itval, NULL) == -1) {
    fprintf(stderr, "error in setitimer()\n");
  }
#endif

  counting_on = 1;
  /* wait signal */
  for (i = 0; i < (measure_time / PRINT_INTERVAL); i++) {
#ifndef _SLEEP_ONLY_
    pause();
#else
    sleep(PRINT_INTERVAL);
    alarm_dummy();
#endif
  }
  counting_on = 0;

#ifndef _SLEEP_ONLY_
  /* stop timer */
  itval.it_interval.tv_sec = 0;
  itval.it_interval.tv_usec = 0;
  itval.it_value.tv_sec = 0;
  itval.it_value.tv_usec = 0;
  if (setitimer(ITIMER_REAL, &itval, NULL) == -1) {
    fprintf(stderr, "error in setitimer()\n");
  }
#endif

  printf("\nSTOPPING THREADS");
  activate_transaction = 0;

  /* wait threads' ending and close connections*/
  for (i = 0; i < num_conn; i++) {
    pthread_join(t[i], NULL);
  }

  printf("\n");

  free(t);
  free(thd_arg);

  // hist_report();
  if (freport_file != NULL)
    fclose(freport_file);

  if (ftrx_file != NULL)
    fclose(ftrx_file);

  if (fseed_file != NULL)
    fclose(fseed_file);

  if (!silent_flag) {
  printf("\n<Raw Results>\n");
    for (i = 0; i < 5; i++) {
      printf("  [%d] sc:%d lt:%d  rt:%d  fl:%d avg_rt: %.1f (%d)\n", i,
            success[i], late[i], retry[i], failure[i],
            total_rt[i] / (success[i] + late[i]), rt_limit[i]);
    }
    printf(" in %d sec.\n", (measure_time / PRINT_INTERVAL) * PRINT_INTERVAL);

    printf("\n<Raw Results2(sum ver.)>\n");
    for (i = 0; i < 5; i++) {
      success2_sum[i] = 0;
      late2_sum[i] = 0;
      retry2_sum[i] = 0;
      failure2_sum[i] = 0;
      for (k = 0; k < num_conn; k++) {
        success2_sum[i] += success2[i][k];
        late2_sum[i] += late2[i][k];
        retry2_sum[i] += retry2[i][k];
        failure2_sum[i] += failure2[i][k];
      }
    }
    for (i = 0; i < 5; i++) {
      printf("  [%d] sc:%d  lt:%d  rt:%d  fl:%d \n", i, success2_sum[i],
            late2_sum[i], retry2_sum[i], failure2_sum[i]);
    }

    printf(
        "\n<Constraint Check> (all must be [OK])\n [transaction percentage]\n");
    for (i = 0, j = 0; i < 5; i++) {
      j += (success[i] + late[i]);
    }

    f = 100.0 * (float)(success[1] + late[1]) / (float)j;
    printf("        Payment: %3.2f%% (>=43.0%%)", f);
    if (f >= 43.0) {
      printf(" [OK]\n");
    }
    else {
      printf(" [NG] *\n");
    }
    f = 100.0 * (float)(success[2] + late[2]) / (float)j;
    printf("   Order-Status: %3.2f%% (>= 4.0%%)", f);
    if (f >= 4.0) {
      printf(" [OK]\n");
    }
    else {
      printf(" [NG] *\n");
    }
    f = 100.0 * (float)(success[3] + late[3]) / (float)j;
    printf("       Delivery: %3.2f%% (>= 4.0%%)", f);
    if (f >= 4.0) {
      printf(" [OK]\n");
    }
    else {
      printf(" [NG] *\n");
    }
    f = 100.0 * (float)(success[4] + late[4]) / (float)j;
    printf("    Stock-Level: %3.2f%% (>= 4.0%%)", f);
    if (f >= 4.0) {
      printf(" [OK]\n");
    }
    else {
      printf(" [NG] *\n");
    }

    printf(" [response time (at least 90%% passed)]\n");
    f = 100.0 * (float)success[0] / (float)(success[0] + late[0]);
    printf("      New-Order: %3.2f%% ", f);
    if (f >= 90.0) {
      printf(" [OK]\n");
    }
    else {
      printf(" [NG] *\n");
    }
    f = 100.0 * (float)success[1] / (float)(success[1] + late[1]);
    printf("        Payment: %3.2f%% ", f);
    if (f >= 90.0) {
      printf(" [OK]\n");
    }
    else {
      printf(" [NG] *\n");
    }
    f = 100.0 * (float)success[2] / (float)(success[2] + late[2]);
    printf("   Order-Status: %3.2f%% ", f);
    if (f >= 90.0) {
      printf(" [OK]\n");
    }
    else {
      printf(" [NG] *\n");
    }
    f = 100.0 * (float)success[3] / (float)(success[3] + late[3]);
    printf("       Delivery: %3.2f%% ", f);
    if (f >= 90.0) {
      printf(" [OK]\n");
    }
    else {
      printf(" [NG] *\n");
    }
    f = 100.0 * (float)success[4] / (float)(success[4] + late[4]);
    printf("    Stock-Level: %3.2f%% ", f);
    if (f >= 90.0) {
      printf(" [OK]\n");
    }
    else {
      printf(" [NG] *\n");
    }

    printf("\n<TpmC>\n");
    f = (float)(success[0] + late[0]) * 60.0 /
        (float)((measure_time / PRINT_INTERVAL) * PRINT_INTERVAL);
    printf("                 %.3f TpmC\n", f);
  }

  curl_global_cleanup();

  exit(0);

sqlerr:
  fprintf(stdout, "error at main\n");
  curl_global_cleanup();
  exit(1);
}

void alarm_handler(int signum) {
  int i;
  int s[5], l[5];
  double rt90[5];
  double trt[5];
  double percentile_val;
  double percentile_val99;

  for (i = 0; i < 5; i++) {
    s[i] = success[i];
    l[i] = late[i];
    trt[i] = total_rt[i];
    // rt90[i] = hist_ckp(i);
  }

  time_count += PRINT_INTERVAL;
  percentile_val = sb_percentile_calculate(&local_percentile, 95);
  percentile_val99 = sb_percentile_calculate(&local_percentile, 99);
  sb_percentile_reset(&local_percentile);
  //  printf("%4d, %d:%.3f|%.3f(%.3f), %d:%.3f|%.3f(%.3f), %d:%.3f|%.3f(%.3f),
  //  %d:%.3f|%.3f(%.3f), %d:%.3f|%.3f(%.3f)\n",
  if (!silent_flag) {
    printf("%4d, trx: %d, 95%: %.3f, 99%: %.3f, max_rt: %.3f, %d|%.3f, %d|%.3f, "
          "%d|%.3f, %d|%.3f\n",
          time_count, (s[0] + l[0] - prev_s[0] - prev_l[0]), percentile_val,
          percentile_val99, (double)cur_max_rt[0],
          (s[1] + l[1] - prev_s[1] - prev_l[1]), (double)cur_max_rt[1],
          (s[2] + l[2] - prev_s[2] - prev_l[2]), (double)cur_max_rt[2],
          (s[3] + l[3] - prev_s[3] - prev_l[3]), (double)cur_max_rt[3],
          (s[4] + l[4] - prev_s[4] - prev_l[4]), (double)cur_max_rt[4]);
    fflush(stdout);
  }

  for (i = 0; i < 5; i++) {
    prev_s[i] = s[i];
    prev_l[i] = l[i];
    prev_total_rt[i] = trt[i];
    cur_max_rt[i] = 0.0;
  }
}

void alarm_dummy() {
  int i;
  int s[5], l[5];
  float rt90[5];

  for (i = 0; i < 5; i++) {
    s[i] = success[i];
    l[i] = late[i];
    rt90[i] = hist_ckp(i);
  }

  time_count += PRINT_INTERVAL;
  if (!silent_flag) {
    printf(
        "%4d, %d(%d):%.2f, %d(%d):%.2f, %d(%d):%.2f, %d(%d):%.2f, %d(%d):%.2f\n",
        time_count, (s[0] + l[0] - prev_s[0] - prev_l[0]), (l[0] - prev_l[0]),
        rt90[0], (s[1] + l[1] - prev_s[1] - prev_l[1]), (l[1] - prev_l[1]),
        rt90[1], (s[2] + l[2] - prev_s[2] - prev_l[2]), (l[2] - prev_l[2]),
        rt90[2], (s[3] + l[3] - prev_s[3] - prev_l[3]), (l[3] - prev_l[3]),
        rt90[3], (s[4] + l[4] - prev_s[4] - prev_l[4]), (l[4] - prev_l[4]),
        rt90[4]);
    fflush(stdout);
  }

  for (i = 0; i < 5; i++) {
    prev_s[i] = s[i];
    prev_l[i] = l[i];
  }
}

int thread_main(thread_arg *arg) {
  int r;

  r = driver(arg->number, arg->port);

  printf(".");
  fflush(stdout);

  return (r);
}
