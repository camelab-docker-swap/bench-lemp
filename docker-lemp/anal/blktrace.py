
def main():
    for NUM_CONNECT in ARR_CONNECT:
        for NUM_SCALE in ARR_SCALE:
            for SWAP_TYPE in ARR_SWAP_TYPE:
                FULL_PATH = "/mnt/data/"+ TEST_TYPE +"/swap-" + SWAP_TYPE + \
                "/SCALE" + str(NUM_SCALE) + "-CONNECT" + str(NUM_CONNECT)
                for DEV_ID in range(1, 5):
                    TRACE_PATH = FULL_PATH + 
if __name__ == "__main__":
    main()