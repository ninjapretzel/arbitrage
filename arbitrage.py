import datetime

with open("stuff.dat", "r") as file:
	lines = file.read().split('\n')
	#data = file.read()

graph = { }
queue = []
profitable = []
for line in lines:
	parts = line.split()
	
	if len(parts) != 3:
		continue
	
	currencyFrom = parts[0]
	currencyTo = parts[1]
	try:
		rate = float(parts[2])
	except:
		continue
	
	if currencyFrom not in graph:
		graph[currencyFrom] = {}
	if currencyTo not in graph:
		graph[currencyTo] = {}
	
	graph[currencyFrom][currencyTo] = rate
	graph[currencyTo][currencyFrom] = 1.0/rate
	
# end loading file
# print(graph)


def step(data, place):
	if data["at"] != place:
		nextRate = data["rate"] * graph[data["at"]][place]
		
		return {
			"at": place,
			"start": data["start"],
			"rate": nextRate,
			# relative gains per trade
			# (rate - 1.0) / numTrades
			"score": (nextRate - 1.0) / len(data["path"]),
			"path": [ *data["path"], place ]
		}
	return data
def stepToStart(data):
	return step(data, data["start"])

start = datetime.datetime.now()
for k in graph:
	data = {
		"at": k,
		"start": k,
		"rate": 1.0,
		"score": 0.0, 
		"path": [ k ]
	}
	queue.append(data)

# print(queue)

while (len(queue) > 0):
	current = queue[0]
	queue.remove(current)
	# 
	compare = stepToStart(current)
	for k in graph:
		if (k not in current["path"]):
			next = step(current, k)
			scored = stepToStart(next)
			
			if (scored["rate"] > 1):
				profitable.append(scored)
			
			# Bellman ford, discard negative cash flow
			# by not traversing negative possibilities
			if (scored["rate"] >= compare["rate"]):
				queue.append(next)
end = datetime.datetime.now()
diff = end - start
print("It only took " + str(diff) + "s")
print("found " + str(len(profitable)) + " profitable things")

def byRate(a):
	return a["rate"]
			
def byScore(a):
	return a["score"]
	
print("\n\n\nBest overall path:")
sortedbyrate = sorted(profitable, key=byRate, reverse=True)
print(sortedbyrate[0])

print("\n\n\nMost Efficient path:")
sortedbyscore = sorted(profitable, key=byScore, reverse=True)
print(sortedbyscore[0])

