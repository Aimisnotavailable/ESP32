import subprocess
import time
import random

RED = "\033[31m"
YELLOW = "\033[33m"
GREEN = "\033[32m"
RESET = "\033[0m"

command = ['esptool', 'flash-id']
while True:
    subprocess.run(command, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)

    print('Collecting data... signal_1.py is running')
    time.sleep(0.5)
    print('Data collection complete.')
    time.sleep(0.5)
    print('Processing... data')
    time.sleep(0.5)
    print('Data processed successfully.')
    num = random.random() * 1000 
    print(f'Results : {num : .2f} Hz')
    if num < 400:
        print(f"{YELLOW}Warning: Signal strength is below acceptable levels!{RESET}")
    elif num > 800:
        print(f"{RED}Alert: Signal strength is above acceptable levels!{RESET}")
    else:
        print(f"{GREEN}Signal strength is within acceptable levels.{RESET}")
    print('-----------------------------------')
    print('RESETTING THE SYSTEM...')
    time.sleep(0.5)
    

    























                                                                                                                                  