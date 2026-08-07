
# FPS-R Visualiser Performance Metrics

### 
Using the FPS-R Visualizer default settings for each algorithm, the following approximate performance metrics were observed.

## HTML Preview in Visual Studio Code

| Algorithm | Approximate Performance |
| :--- | --- |
Legacy Stateful `rand()` | ~2, 3xx k/s |
Stacked Modulo | ~1, 4xx k/s |
Toggled Modulo | ~1, 74x k/s |
Quantised Switching | ~880 k/s |
Bitwise Decode (default 7 streams, blocksize 30) |  ~179 k/s |

### BD Breakdown by Streams and Blocksize
| Streams | Blocksize | Approximate Performance |
| :--- | --- | --- |
| 1 | 30 | ~590 k/s |
|   | 60 | ~580 k/s |
| 2 | 30 | ~444 k/s |
|   | 60 | ~446 k/s |
| 4 | 30 | ~296 k/s |
|   | 60 | ~289 k/s |
| 7 | 30 | ~179 k/s |
|   | 60 | ~151 k/s |
