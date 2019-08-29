from __future__ import print_function
from scipy.stats import norm
import warnings
warnings.filterwarnings('ignore')
import sys
import os
import re
import math
import numpy as np
import matplotlib.pyplot as plt
from bisect import bisect_left
from matplotlib.lines import Line2D

regexPHP = re.compile('200\s(\d+(\.\d*)?|\.\d+)ms')
regexNGNX = re.compile('\"rt=(\d+(\.\d*)?|\.\d+)')
regexDB = re.compile('#\sQuery_time\:\s(\d+(\.\d+)?)')

ARR_SWAP_TYPE=["single", "multiple", "private"]
# ARR_SWAP_TYPE=["multiple"]
ARR_SCALE = [4]
ARR_CONNECT = [100]
NUM_PHP = 5
TEST_TYPE="bench-lemp"

def switch_color(DEV_ID):
    return {
        1: "g",
        2: "k",
        3: "b",
        4: "r"       
    }[DEV_ID]

class discrete_cdf:
    def __init__(self, data):
        self._data = data # must be sorted
        self._data_len = float(len(data))

    def __call__(self, point):
        return (len(self._data[:bisect_left(self._data, point)]) / 
                self._data_len)

def calcdf(data):
    cdf = discrete_cdf(np.sort(data))
    xvalues = range(0, int(math.ceil(max(data))))
    yvalues = [cdf(point) for point in xvalues]
    return xvalues, yvalues

def mysql_parser(FULL_PATH, NUM_SCALE):
    for DEV_ID in range(1, 5):
        MYSQL_MERGE_PATH=FULL_PATH + "/NS" + str(DEV_ID) + "-mysql.log"
        for SCALE_ID in range(1, NUM_SCALE + 1):
            PHP_ID = SCALE_ID + NUM_SCALE * (DEV_ID - 1)
            MYSQL_PATH=FULL_PATH + "/NS" + str(DEV_ID) + "-mysql" + str(SCALE_ID) + ".log"
            if os.path.isfile(MYSQL_PATH):
                os.remove(MYSQL_PATH)

            fo = open(MYSQL_PATH,"w+")

            checker = 0
            with open(MYSQL_MERGE_PATH) as f:
                f.seek(0,0)
                for line in f:
                    if checker == 1:
                        fo.write(line)
                    if "nginx" + str(PHP_ID) in line:
                        checker = 1
                    else:
                        checker = 0
                    
def get_val(LOG_PATH):   
    arr_val = []
    if os.path.isfile(LOG_PATH):
        with open(LOG_PATH) as f:
            f.seek(0)
            for line in f:
                if "php" in LOG_PATH:
                    match = regexPHP.search(line)
                    if match:
                        arr_val.append(float(match.group(1))) # ms
                elif "nginx" in LOG_PATH:
                    match = regexNGNX.search(line)
                    if match:
                        arr_val.append(float(match.group(1)) * 1000) # ms
                else:
                    match = regexDB.search(line)
                    if match:
                        arr_val.append(float(match.group(1)) * 1000) # ms

    return arr_val

def init(NUM_SCALE, SWAP_TYPE, NUM_CONNECT):
    for DEV_ID in range(1, 5):
        for SCALE_ID in range(1, NUM_SCALE + 1):
            FULL_PATH = "/mnt/data/"+ TEST_TYPE +"/swap-" + SWAP_TYPE + \
                "/SCALE" + str(NUM_SCALE) + "-CONNECT" + str(NUM_CONNECT)
            MYSQL_PATH = FULL_PATH + "/NS" + str(DEV_ID) + "-mysql" + str(SCALE_ID) + ".log"
            if os.path.isfile(MYSQL_PATH):
                os.remove(MYSQL_PATH)

def main():
    print("total-duration nginx php mysql")
    for NUM_CONNECT in ARR_CONNECT:
        for NUM_SCALE in ARR_SCALE:
            print("SCALE"+str(NUM_SCALE))
            for SWAP_TYPE in ARR_SWAP_TYPE:
                init(NUM_SCALE, SWAP_TYPE, NUM_CONNECT)
                
                PATH = "SWAP-" + SWAP_TYPE + "/SCALE" + str(NUM_SCALE) + "-CONNECT" + str(NUM_CONNECT)
                fig, axes = plt.subplots(1, 3, figsize=(15,5))
                for ax in axes.flat:
                    ax.set_xscale('symlog')

                fig.suptitle(PATH)
                axes[0].set_title('NGINX')
                axes[1].set_title('PHP')
                axes[2].set_title('MYSQL')

                for DEV_ID in range(1, 5):
                    arr_php_dev, arr_nginx_dev, arr_mysql_dev = [], [], []
                    for SCALE_ID in range(1, NUM_SCALE + 1):
                        arr_php, arr_nginx, arr_mysql = [], [], []
                        FULL_PATH = "/mnt/data/"+ TEST_TYPE +"/swap-" + SWAP_TYPE + \
                            "/SCALE" + str(NUM_SCALE) + "-CONNECT" + str(NUM_CONNECT)

                        MYSQL_PATH = FULL_PATH + "/NS" + str(DEV_ID) + "-mysql" + str(SCALE_ID) + ".log"
                        if not os.path.isfile(MYSQL_PATH):
                            mysql_parser(FULL_PATH, NUM_SCALE)
                        
                        NGNX_PATH = FULL_PATH + "/NS" + str(DEV_ID) + "-nginx" + str(SCALE_ID) + ".log"

                        arr_nginx = get_val(NGNX_PATH)
                        arr_mysql = get_val(MYSQL_PATH)

                        arr_nginx_dev = arr_nginx_dev + arr_nginx
                        arr_mysql_dev = arr_mysql_dev + arr_mysql

                        for PHP_ID in range(1, NUM_PHP + 1):
                            PHP_PATH = FULL_PATH + "/NS" + str(DEV_ID) + "-nginx" + str(SCALE_ID) + "-php" + str(PHP_ID) + ".log"
                            arr_php = get_val(PHP_PATH)
                            arr_php_dev = arr_php_dev + arr_php

                    color = switch_color(DEV_ID)
                    xvalues, yvalues = calcdf(arr_nginx_dev)
                    axes[0].plot(xvalues, yvalues, color)

                    xvalues, yvalues = calcdf(arr_php_dev)
                    axes[1].plot(xvalues, yvalues, color)

                    xvalues, yvalues = calcdf(arr_mysql_dev)
                    axes[2].plot(xvalues, yvalues, color)

                #### Plot figure
                FIG_PATH = "/mnt/data/"+ TEST_TYPE +"/swap-" + SWAP_TYPE + \
                        "/SCALE" + str(NUM_SCALE) + "-CONNECT" + str(NUM_CONNECT) + "/detail.png"

                custom_lines = [Line2D([0], [0], color='g', lw=4),
                                Line2D([0], [0], color='k', lw=4),
                                Line2D([0], [0], color='b', lw=4),
                                Line2D([0], [0], color='r', lw=4)]

                line_labels = ["1", "2", "3", "4"]
                fig.legend(custom_lines,
                            labels=line_labels,
                            loc="center right",
                            borderaxespad=0.1,
                            title="Namespace ID")
                plt.subplots_adjust(right=0.85)
                fig.savefig(FIG_PATH)
                plt.close(fig)

if __name__ == "__main__":
    main()
