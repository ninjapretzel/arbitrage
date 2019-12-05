# Node Example Output
```
λ node arbitrage.js
Ready. Running.


DONE!
Took 36ms
Found 16358 profitable cycles!


Best Overall arbitrage:
{
  start: 'Ruble',
  at: 'Ruble',
  rate: 1.2443321533613452,
  score: 0.027148017040149464,
  path: [
    'Ruble',     'Sweden',
    'Dollar',    'Canada',
    'Euro',      'Pound',
    'Yen',       'Norway',
    'Australia', 'Ruble'
  ]
}


Most efficient arbitrage:
{
  start: 'Yen',
  at: 'Yen',
  rate: 1.12575,
  score: 0.04191666666666668,
  path: [ 'Yen', 'Norway', 'Australia', 'Yen' ]
}
```