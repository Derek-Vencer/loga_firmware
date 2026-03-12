# LOGA 
## Version & Date

V0014.14 - 2026/02/12

## New Feature

Change the trigger logic from a fixed ADC value to a button percentage.

## Bug Fixes

Based on the changes in the triggering logic, several aspects need to be fixed, including the Rappy Snappy mode, SOCD mode, and calibration function.

* Set SOCD-0x010B
  * [0] = index
  * [1] = primary layer
  * [2] = primary row
  * [3] = primary column
  * [4] = secondary layer
  * [5] = secondary row
  * [6] = secondary column
  * [7] = mode
* Get SOCD-0x018B
  * [0] = index;
  * [1] & buf[2] = primary KC
  * [3] & buf[4] = secondary KC
  * [5] = mode;

Fix four apis 
* Set all trigger level
* Get all trigger level
* Set all release level
* Get all release level

## Known Issue