from __future__ import print_function
import warnings
warnings.filterwarnings('ignore')
import os
import re
import numpy as np

CTITLE = '\033[6;30;42m'
CRED = '\033[91m'
CEND = '\033[0m'

ARR_SWAP_TYPE=["single", "multiple", "private"]
# ARR_SWAP_TYPE=["private"]
ARR_SCALE = [1]
ARR_CONNECT = [100]
NUM_PHP = 5
TEST_TYPE="bench-lemp"

regexRT = re.compile('avg_rt:\s(\d+(\.\d+)?)')
regexTpmC = re.compile('\s*(\d+(\.\d+)?)\sTpmC')

def print_data(NUM_SCALE, NUM_CONNECT, arr_2d):
    print(CRED + "SCALE"+str(NUM_SCALE)+"-CONNECT"+str(NUM_CONNECT)+" NS1 NS2 NS3 NS4" + CEND)
    for SWAP_ID in range(len(ARR_SWAP_TYPE)):
        print(ARR_SWAP_TYPE[SWAP_ID], end=' ')      
        print(str(arr_2d[SWAP_ID][0])+" "+str(arr_2d[SWAP_ID][1])+" "+str(arr_2d[SWAP_ID][2])+" "+str(arr_2d[SWAP_ID][3]))

def main():
    for NUM_CONNECT in ARR_CONNECT:
        for NUM_SCALE in ARR_SCALE:
            ### tpmC: transaction per minute 
            ### That is, bigger is better
            # print(CTITLE + "First line is TpmC; bigger is better" + CEND)
            # print(CTITLE + "Other lines are response time; lower is better" + CEND)
            swap_arr_tpmc_dev, swap_arr_rt0_dev, swap_arr_rt1_dev, swap_arr_rt2_dev, swap_arr_rt3_dev, swap_arr_rt4_dev = [], [], [], [], [], []
            for SWAP_TYPE in ARR_SWAP_TYPE:
                arr_tpmc_dev, arr_rt0_dev, arr_rt1_dev, arr_rt2_dev, arr_rt3_dev, arr_rt4_dev = [], [], [], [], [], []
                for DEV_ID in range(1, 5):
                    arr_tpmc, arr_rt0, arr_rt1, arr_rt2, arr_rt3, arr_rt4 = [], [], [], [], [], []
                    for SCALE_ID in range(1, NUM_SCALE + 1):
                        FULL_PATH = "/mnt/data-seed/"+ TEST_TYPE +"/swap-" + SWAP_TYPE + \
                            "/SCALE" + str(NUM_SCALE) + "-CONNECT" + str(NUM_CONNECT)
                        TPCC_PATH = FULL_PATH + "/NS" + str(DEV_ID) + "-SCALE" + str(SCALE_ID) + ".tpcc"
                        if os.path.isfile(TPCC_PATH):
                            with open(TPCC_PATH) as f:
                                f.seek(0)
                                for line in f:
                                    match1 = regexTpmC.search(line)
                                    match2 = regexRT.search(line)
                                    if match1:
                                        arr_tpmc.append(float(match1.group(1)))
                                    if match2:
                                        if '[0]' in line:
                                            arr_rt0.append(float(match2.group(1)))
                                        elif '[1]' in line:
                                            arr_rt1.append(float(match2.group(1)))
                                        elif '[2]' in line:
                                            arr_rt2.append(float(match2.group(1)))
                                        elif '[3]' in line:
                                            arr_rt3.append(float(match2.group(1)))
                                        elif '[4]' in line:
                                            arr_rt4.append(float(match2.group(1)))
                                        else:
                                            arr_rt5.append(float(match2.group(1)))

                    arr_tpmc_dev.append(np.mean(arr_tpmc))
                    arr_rt0_dev.append(np.mean(arr_rt0))
                    arr_rt1_dev.append(np.mean(arr_rt1))
                    arr_rt2_dev.append(np.mean(arr_rt2))
                    arr_rt3_dev.append(np.mean(arr_rt3))
                    arr_rt4_dev.append(np.mean(arr_rt4))

                swap_arr_tpmc_dev.append(arr_tpmc_dev)
                swap_arr_rt0_dev.append(arr_rt0_dev)
                swap_arr_rt1_dev.append(arr_rt1_dev)
                swap_arr_rt2_dev.append(arr_rt2_dev)
                swap_arr_rt3_dev.append(arr_rt3_dev)
                swap_arr_rt4_dev.append(arr_rt4_dev)

            print_data(NUM_SCALE, NUM_CONNECT, swap_arr_tpmc_dev)
            # print_data(NUM_SCALE, NUM_CONNECT, swap_arr_rt0_dev)
            # print_data(NUM_SCALE, NUM_CONNECT, swap_arr_rt1_dev)
            # print_data(NUM_SCALE, NUM_CONNECT, swap_arr_rt2_dev)
            # print_data(NUM_SCALE, NUM_CONNECT, swap_arr_rt3_dev)
            # print_data(NUM_SCALE, NUM_CONNECT, swap_arr_rt4_dev)

if __name__ == "__main__":
    main()
