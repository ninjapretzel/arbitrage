# Node Example Output
```
λ node arbitrage.js
Ready. Running.


DONE!
Took 1215ms
Found 493159 profitable cycles!


Best Overall arbitrage:
{
  start: 'Canada',
  at: 'Canada',
  rate: 1.2443321533613452,
  path: [
    'Canada', 'Euro',
    'Pound',  'Yen',
    'Norway', 'Australia',
    'Ruble',  'Sweden',
    'Dollar', 'Canada'
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