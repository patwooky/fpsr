
# FPS-R Visualiser Performance Metrics

Using the FPS-R Visualizer default settings for each algorithm, the following approximate performance metrics were observed.

---
## PC Specifications
| Specification | Details |
| :--- | --- |
| Processor | AMD Ryzen 9 3900X 12-Core, 3.80 GHz |
| Installed RAM | 64.0 GB |
| Graphics Card | NVIDIA GeForce RTX 2080 Ti (11 GB) |
| System Type | 64-bit operating system, x64-based processor |
| Operating System | Windows 10 Home |

---
## HTML Preview in Visual Studio Code
### VS Code Version Information
| Specification | Details |
| :--- | --- |
| Version | 1.99.2 (user setup) |
| Electron | 34.3.2 |
| ElectronBuildId | 11161073 |
| Chromium | 132.0.6834.210 |
| Node.js | 20.18.3 |
| V8 | 13.2.152.41-electron.0 |
| OS | Windows_NT x64 10.0.19045 |

### Algorithm Performance Metrics
| Algorithm | Approximate Performance |
| :--- | --- |
Legacy Stateful `rand()` | ~2, 300 k/s |
Stacked Modulo | ~1, 400 k/s |
Toggled Modulo | ~1, 740 k/s |
Quantised Switching | ~880 k/s |
Bitwise Decode (default 3 streams, blocksize 64) | ~333 k/s |

### BD Breakdown by Streams and Blocksize
| Streams | Blocksize | Approximate Performance |
| :--- | --- | --- |
| 1 | 30 | ~590 k/s |
|   | 60 | ~580 k/s |
| 2 | 30 | ~444 k/s |
|   | 60 | ~446 k/s |
| 3 | 30 | ~340 k/s |
|   | 60 | ~333 k/s |
| 4 | 30 | ~296 k/s |
|   | 60 | ~289 k/s |
| 7 | 30 | ~179 k/s |
|   | 60 | ~151 k/s |

---
## Chrome Browser 
### Chrome Version Information
Version 151.0.7922.34 (Official Build) (64-bit)

### Algorithm Performance Metrics
| Algorithm | Approximate Performance |
| :--- | --- |
| Legacy Stateful `rand()` | ~2, 800 k/s |
| Stacked Modulo | ~1, 580 k/s |
| Toggled Modulo | ~2, 130 k/s |
| Quantised Switching | ~920 k/s |
| Bitwise Decode (default 3 streams, blocksize 64) | ~388 k/s |

### BD Breakdown by Streams and Blocksize
| Streams | Blocksize | Approximate Performance |
| :--- | --- | --- |
| 1 | 30 | ~705 k/s |
|   | 60 | ~700 k/s |
| 2 | 30 | ~505 k/s |
|   | 60 | ~504 k/s |
| 3 | 30 | ~385 k/s |
|   | 60 | ~388 k/s |
| 4 | 30 | ~325 k/s |
|   | 60 | ~324 k/s |
| 7 | 30 | ~192 k/s |
|   | 60 | ~179 k/s |

---
## Firefox Browser
### Firefox Version Information
Version 153.0.3 (64-bit)

### Algorithm Performance Metrics
| Algorithm | Approximate Performance |
| :--- | --- |
| Legacy Stateful `rand()` | ~7, 600 k/s |
| Stacked Modulo | ~1, 060 k/s |
| Toggled Modulo | ~1, 750 k/s |
| Quantised Switching | ~660 k/s |
| Bitwise Decode (default 3 streams, blocksize 64) | ~340 k/s |

### BD Breakdown by Streams and Blocksize
| Streams | Blocksize | Approximate Performance |
| :--- | --- | --- |
| 1 | 30 | ~646 k/s |
|   | 60 | ~605 k/s |
| 2 | 30 | ~435 k/s |
|   | 60 | ~440 k/s |
| 3 | 30 | ~328 k/s |
|   | 60 | ~340 k/s |
| 4 | 30 | ~295 k/s |
|   | 60 | ~283 k/s |
| 7 | 30 | ~170 k/s |
|   | 60 | ~157 k/s |

---
## Android Phone Specifications
| Specification | Details |
| :--- | --- |
| Samsung | Galaxy S23 Ultra 5G |
| Model | SM-S918B/DS |
| Processor | Qualcomm Snapdragon 8 Gen 2 |
| Memory | 12 GB |
| System Type | 64-bit operating system, arm64-based processor |
| Android Version | 16 |

## Android Chrome Browser
### Chrome Version Information
Version 151.0.7922.83

### Algorithm Performance Metrics
| Algorithm | Approximate Performance |
| :--- | --- |
| Legacy Stateful `rand()` | ~2, 930 k/s |
| Stacked Modulo | ~1, 830 k/s |
| Toggled Modulo | ~2, 280 k/s |
| Quantised Switching | ~1, 080 k/s |
| Bitwise Decode (default 3 streams, blocksize 64) | ~548 k/s |

### BD Breakdown by Streams and Blocksize
| Streams | Blocksize | Approximate Performance |
| :--- | --- | --- |
| 1 | 30 | ~830 k/s |
|   | 60 | ~820 k/s |
| 2 | 30 | ~680 k/s |
|   | 60 | ~682 k/s |
| 3 | 30 | ~540 k/s |
|   | 60 | ~548 k/s |
| 4 | 30 | ~375 k/s |
|   | 60 | ~390 k/s |
| 7 | 30 | ~210 k/s |
|   | 60 | ~200 k/s |