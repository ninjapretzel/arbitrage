# Node Example Output
```
λ node arbitrage.js
Ready. Running.


DONE!
Took 30ms
Found 16358 profitable cycles!


Best Overall arbitrage:
{
  start: 'Ruble',
  at: 'Ruble',
  rate: 1.2443321533613452,
  path: [
    'Ruble',     'Sweden',
    'Dollar',    'Canada',
    'Euro',      'Pound',
    'Yen',       'Norway',
    'Australia', 'Ruble'
  ],
  score: 0.027148017040149464
}


Most efficient arbitrage:
{
  start: 'Yen',
  at: 'Yen',
  rate: 1.12575,
  path: [ 'Yen', 'Norway', 'Australia', 'Yen' ],
  score: 0.04191666666666668
}

```