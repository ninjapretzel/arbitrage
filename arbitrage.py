import datetime

# Open file and read all lines
with open("stuff.dat", "r") as file: #{
	lines = file.read().split('\n')
#}

# Create data structures for stuff
# graph is a Dictionary of string: Dictionary
# Outer dictionary stores all valid vertex names as keys
# Inner dictionaries store transition costs to other verticies 
graph = { }
# We'll use a array as a queue
queue = [ ]
# And another for the ones that proiftable
profitable = []

for line in lines: #{
	# Split line into segments separated by whitespace
	parts = line.split()
	
	# Skip any that aren't 3 long
	if len(parts) != 3: #{ continue; }
		continue
	#} 
	
	currencyFrom = parts[0]
	currencyTo = parts[1]
	# Skip any that don't have a number in the last place
	try:#{ rate = float(parts[2]); } except: { continue; }
		rate = float(parts[2])
	#}
	except: #{
		continue
	#}
	
	# If we don't have verticies in our graph, create them
	if currencyFrom not in graph: #{ graph[currencyFrom] = {}; }
		graph[currencyFrom] = {}
	#}
	if currencyTo not in graph: #{ graph[currencyTo] = {}; }
		graph[currencyTo] = {}
	#}
	
	# Add transitions, from -> to is rate, to -> from is reciprocal
	graph[currencyFrom][currencyTo] = rate
	graph[currencyTo][currencyFrom] = 1.0/rate
	
#} # end loading file

# print(graph)

# Transition function 
def step(data, place): #{
	# If data isn't at the place...
	if data["at"] != place: #{
		# Create a version of data transitioned there.
		# Calculate rate
		nextRate = data["rate"] * graph[data["at"]][place]
		
		return {
			# Move to place 
			"at": place,
			# Same starting location 
			"start": data["start"],
			# Use calculated rate 
			"rate": nextRate,
			# Score node based on relative gains per trade
			# (rate - 1.0) / numTrades
			"score": (nextRate - 1.0) / len(data["path"]),
			# Add place to our path
			"path": [ *data["path"], place ]
		}
	#}
	# Otherwise, just return the data we built
	return data
#} end step
# Helper, transition a node back to it's starting location 
def stepToStart(data): #{ return step(data, data["start"]); }
	return step(data, data["start"])
#} end stepToStart

# Seed queue by starting at every single vert in the graph
for k in graph: #{
	data = {
		"at": k,
		"start": k,
		"rate": 1.0,
		"score": 0.0, 
		"path": [ k ]
	}
	queue.append(data)
#} end seeding queue

# print(queue)

# Time it because why not?
start = datetime.datetime.now()
# while there's work
while (len(queue) > 0): #{
	# Grab + remove next item from queue
	current = queue[0]
	queue.remove(current)
	
	# Step that item back to the start for a normalized comparison
	compare = stepToStart(current)
	
	# Loop over all keys (verts) in graph
	for k in graph: #{
		# Skip repeating any places we've already been
		if (k not in current["path"]): #{
			# Step current to destination 
			next = step(current, k)
			# Step that back to start to normalize gains
			scored = stepToStart(next)
			
			# If the thing is profitable, add it to our profitable list
			if (scored["rate"] > 1): #{ profitable.append(scored); }
				profitable.append(scored)
			#}
			
			# Bellman ford's part: discard negative cash flow
			# by not traversing negative possibilities
			if (scored["rate"] >= compare["rate"]): #{ queue.append(next); }
				# Add more work to queue if it looks good
				queue.append(next)
			#}
		#}
	#} End graph lop
#} End work loop

# Take end time
end = datetime.datetime.now()
# Find how long it took
diff = end - start
# Open output file
fout = open("outputpy.txt", "w")

# sorting (unfortunately no standardized lambda sort)
def byRate(a): #{ return a.rate; }
	return a["rate"]
			
def byScore(a): #{ return a.score; }
	return a["score"]

# Sort Ascending by rate for file printout
sortedbyrate = sorted(profitable, key=byRate, reverse=False)
i = 0
for data in sortedbyrate: #{
	i += 1
	fout.write("Path " + str(i) + ": " + (" => ".join(data["path"])) + ", $1000.00 =>" + str(data["rate"] * 1000.0)+"\n")
#}

# Be cheeky about how long it took
print("Wow that was slow, it took " + str(diff.total_seconds() * 1000.0) + "ms")
fout.write("Wow that was slow, it took " + str(diff.total_seconds() * 1000.0) + "ms\n")
print("found " + str(len(profitable)) + " profitable things")
fout.write("found " + str(len(profitable)) + " profitable things\n")

# Print out highest return rate after trading with no revisits
fout.write("\n\nBest overall path:\n")
print("\n\nBest overall path:")
sortedbyrate = sorted(profitable, key=byRate, reverse=True)
fout.write(str(sortedbyrate[0]) + "\n")
print(sortedbyrate[0])

# Print out most efficient tight loop
fout.write("\n\nMost Efficient path:\n")
print("\n\nMost Efficient path:")
sortedbyscore = sorted(profitable, key=byScore, reverse=True)
fout.write(str(sortedbyscore[0]) + "\n")
print(sortedbyscore[0])

fout.close()