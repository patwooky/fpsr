
# FPS-R Test

## Details
**Date**: 22 Sep 2025
**Test No.**: 20250921_02
**Tester**:  Patrick Woo

**Platforms**: 
- [www.programiz.com Online C Compiler](https://www.programiz.com/c-programming/online-compiler/)
- [www.programiz.com Online Python Interpreter](https://www.programiz.com/python-programming/online-compiler/)
- Local PC Python Interpreter:
    - Windows 10
    - Python 3.13.5

## Description
To test the core base algorithm output of SM, TM and QS. Compare the outputs between C `fpsr_algorithm_reference.c` and Python `fpsr_algorithm.py`.
    - check for parity (make sure the output matches)

## Test Screenshots
### SM
SM C output
![img](./sm_c.jpg)
SM Python Output
![img](./sm_py.jpg)
SM Python local Output
![img](./sm_py(local).jpg)
**Notes**:
- same values
- same jump frames

### TM
TM C output
![img](./tm_c.jpg)
TM Python output
![img](./tm_py.jpg)
TM Python local output
![img](./tm_py(local).jpg)
**Notes**:
- same values
- same jump frames

### QS
QS C output
![img](./qs_c.jpg)
QS Python output
![img](./qs_py.jpg)
QS Python local output
![img](./qs_py(local).jpg)
**Notes**:
- same values
- same jump frames

## Observations
- Values and jump frames are now matching and replicated across C and Python implementations 


## Pass / Fail: 
**Pass**