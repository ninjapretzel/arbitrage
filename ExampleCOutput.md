# C Example Output
```
λ gcc -g3 -c arbitrage.c -o arbitrage.o && gcc -g3 -o arbitrage.exe arbitrage.o && arbitrage
Done loading graph, there are 9 currencies.Graph seeded with 9 queued work.
Going now...


Done!
Took 21.969ms
Found 16358 profitable cycles!

Best Overall Arbitrage: {
        Rate: 1.244,
        Score: 0.027,
        Path: [ Ruble -> Sweden -> Dollar -> Canada -> Euro -> Pound -> Yen -> Norway -> Australia -> Ruble ]
}

Most Efficient Arbitrage: {
        Rate: 1.126,
        Score: 0.042,
        Path: [ Yen -> Norway -> Australia -> Yen ]
}
```