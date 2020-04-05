# Initial values

|-------|---------------|
|date	| 12.17		|
|kernel	| 4.4.0		|
|ucode	| pre-spectre	|
|SGX SDK| 1.8		|

|     |                                               | SGX mem | VNL mem | NTV    | VNL     | nnI     | CnI    | CFI    |
|-----|-----------------------------------------------|---------|---------|--------|---------|---------|--------|--------|
| 100 | 1000000 INSERTs into table with no index      | 65      | 70      | 2.033  | 2.808   | 2.709   | 2.69   | 2.704  |
| 110 | 1000000 ordered INSERTS with one index/PK     | 144     | 154     | 2.584  | 3.681   | 3.571   | 3.566  | 3.5    |
| 120 | 1000000 unordered INSERTS with one index/PK   | 222     | 237     | 3.147  | 6.947   | 7.027   | 6.599  | 6.923  |
| 130 | 25 SELECTS, numeric BETWEEN, unindexed        | 222     | 237     | 1.451  | 2.317   | 1.581   | 1.684  | 1.641  |
| 140 | 10 SELECTS, LIKE, unindexed                   | 222     | 237     | 1.787  | 2.183   | 2.07    | 2.04   | 2.039  |
| 145 | 10 SELECTS w/ORDER BY and LIMIT, unindexed    | 222     | 237     | 2.063  | 2.484   | 2.401   | 2.404  | 2.368  |
| 150 | CREATE INDEX five times                       | 425     | 453     | 3.236  | 12.214  | 14.898  | 15.91  | 15.267 |
| 160 | 200000 SELECTS, numeric BETWEEN, indexed      | 425     | 453     | 7.941  | 11.709  | 12.715  | 12.123 | 12.42  |
| 161 | 200000 SELECTS, numeric BETWEEN, PK           | 425     | 453     | 7.816  | 11.489  | 12.483  | 11.826 | 12.602 |
| 170 | 200000 SELECTS, text BETWEEN, indexed         | 425     | 453     | 7.893  | 22.286  | 17.737  | 13.74  | 12.651 |
| 180 | 1000000 INSERTS with three indexes            | 581     | 621     | 3.396  | 15.069  | 14.8    | 10.801 | 14.859 |
| 190 | DELETE and REFILL one table                   | 589     | 629     | 3.57   | 15.557  | 17.758  | 15.96  | 17.959 |
| 210 | ALTER TABLE ADD COLUMN, and query             | 589     | 629     | 0.079  | 0.306   | 0.078   | 0.126  | 0.08   |
| 230 | 200000 UPDATES, numeric BETWEEN, indexed      | 601     | 642     | 9.271  | 31.311  | 16.643  | 16.189 | 16.733 |
| 240 | 1000000 UPDATES of individual rows            | 601     | 642     | 3.845  | 33.243  | 17.729  | 14.306 | 17.699 |
| 250 | One big UPDATE of the whole 1000000-row table | 601     | 642     | 0.173  | 1.487   | 2.529   | 2.716  | 2.481  |
| 260 | Query added column after filling              | 601     | 642     | 0.072  | 0.416   | 0.088   | 0.317  | 0.092  |
| 270 | 200000 DELETEs, numeric BETWEEN, indexed      | 601     | 642     | 3.84   | 23.315  | 17.744  | 15.517 | 17.657 |
| 280 | 1000000 DELETEs of individual rows            | 601     | 642     | 4.12   | 37.209  | 19.441  | 17.441 | 19.531 |
| 290 | Refill two 1000000-row tables using REPLACE   | 601     | 642     | 5.569  | 30.291  | 40.44   | 22.883 | 41.001 |
| 300 | Refill a 1000000-row table using (b&1)==(a&1) | 601     | 642     | 3.603  | 14.117  | 18.511  | 15.368 | 18.975 |
| 310 | 200000 four-ways joins                        | 601     | 642     | 10.003 | 153.536 | 43.687  | 34.715 | 44.156 |
| 320 | subquery in result set                        | 601     | 642     | 41.026 | 63.724  | 43.735  | 41.717 | 43.862 |
| 980 | PRAGMA integrity_check                        | 601     | 642     | 7.527  | 45.235  | 42.314  | 23.733 | 42.793 |
| 990 | ANALYZE                                       | 601     | 642     | 0.475  | 2.04    | 0.634   | 0.839  | 0.637  |
|     | *TOTAL                                         |         |         | 136.52 | 544.974 | 373.323 | 305.21 | 370.63* |
