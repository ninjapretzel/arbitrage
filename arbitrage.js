// Wrote this for some students to demonstrate how to step through the graphs
// File system libray
const fs = require("fs");

// Helper class for a performant queue
// I tried switching pop/unshift to shift/push, and it still behaved very poorly,
// So instead I figured I'd make a little queue that mimiced what I thought the go version was doing
// Turns out that this is really, really, really fast.
function Queue() {
	this.stuff = [];
	this.added = 0;
	this.removed = 0;
	this.put = (thing)=>{ this.stuff[this.added++] = thing; }
	this.take = ()=>{ let it = this.stuff[this.removed]; this.stuff[this.removed++] = undefined; return it; }
	this.count = ()=>{ return this.added - this.removed; }
}

// Load the file as a text file
const text = fs.readFileSync("stuff.dat", 'utf-8');
// console.log(text);

// Split the file into lines
let lines = text.split('\n')
// Load the file into an object to represent the graph
// keys will be node names, values will be objects
//		inner objects will hold transitions
// once loaded, 
//		graph["Canada"]["Dollar"] will hold conversion rate for CND$ to US$
//		graph["Dollar"]["Canada"] will hold conversion rate for US$ to CND$

// Create our data structures...
let graph = {}
// work queue, holds stuff we still need to do
let queue = new Queue()
// List of profitable finished paths.
// We don't need to care about unprofitable paths
// Unprofitable paths are just the reverse of profitable ones
var profitable = []


// Loop over each line
for (let i = 0; i < lines.length; i++) {
	let line = lines[i];
	// Ignore comment lines ( actually doesn't matter tho... )
	if (line.indexOf("//") == 0) { continue; }
	
	// Split line on any whitespace
	let parts = line.split(/\s+/);
	// Just in case, remove any empty strings
	parts = parts.filter( (it) => (it != "") )
	// console.log(parts)
	// Skip any line with any number other than 3 things on it
	if (parts.length != 3) { continue; }
	// Skip lines that don't have a number in the third column
	let from = parts[0];
	let to = parts[1];
	let rate = parts[2];
	try { // Try to convert 3rd thing to a number, skip if it is not.
		rate = Number(rate);
	} catch (err) { continue; }
	// console.log(`${from} -> ${to} @${rate}`);
	
	// Add the entry to the graph.
	// If we don't have the objects in the graph, add new objects for them
	if (!graph[from]) { graph[from] = {}; }
	if (!graph[to]) { graph[to] = {}; }
	
	// Plain rate for from -> to
	graph[from][to] = rate;
	// Reciprocal rate for to -> from
	graph[to][from] = 1.0 / rate;
}

// console.log(graph);

// Measure the score for a node in the same units it started in 
// Transition it back to the starting point and get a score
// Step a node back to starts
function stepToStart(data) { return stepTo(data, data.start); }

// transition a node to place in the graph
function stepTo(data, place) {
	// If this node is not already there:
	if (data.at !== place) {
		// Return the adjusted data
		var nextRate = data.rate * graph[data.at][place];
		return { // Object literal
			// Make the transition to the destination place 
			start: data.start, // Same starting location
			at: place,			// Now at that place 
			rate: nextRate, // Apply rate change 
			// And this was an addition, score the nodes based off of the gains per trade.
			score: (nextRate - 1.0) / (data.path.length),
			path: [ ... data.path, place ], // update path with new place 
			// ... is the spread operator, it takes everything data.path,
			// and 'spreads' those values in order in the new array.
		}
	}
	// if we're already there, just return the data.
	return data;
}


//* This will find all possible cycles, and score them properly.
for (key in graph) {
	queue.put( { 
		start: key, // Remember where we start
		at: key, // current location is where we are starting initially
		rate: 1, // Start with a rate of 1.0
		score: 0, // No score because no trades/gains.
		path: [ key ],  // Remember everywhere we have been
	} );
}
//*/

/* 
// This will find the same 9-cycles regardless of starting point
// Can pick any point- 
// here I choose a random name of a currency within the graph
let first = Object.keys(graph)[Math.floor(Math.random() * Object.keys(graph).length)]
queue.put({
	start: first, // Remember where we started
	at: first,  // We are at where we start
	rate: 1,  // start with a rate of 1.0
	path: [first], // Remember everywhere we have been
})
//*/
// Let's time how long it takes
let startDate = new Date();
console.log("Ready. Running.");

// Continue while there are things in the queue
//while (queue.length > 0) {
// Custom Queue version:
while (queue.count() > 0) {
	// Pop a data off the queue
	// let data = queue.pop();
	// Custom Queue version:
	let data = queue.take();
	let compare = stepToStart(data);
	// Loop over all vertex names in our graph
	for (key in graph) {
		// If we haven't been there yet...
		// (skips revisiting nodes, or transitioning to start)
		if (!data.path.includes(key)) {
			//console.log(data.path + " does not have " + k);
			// Take transition to node k
			let trace = stepTo(data, key);
			// Step the transitioned node back to its start
			let final = stepToStart(trace);
			
			// Check that node's score, if it's profitable, add to list.
			if (final.rate > 1) { profitable.push(final) }
			
			// Check if taking the step to key makes us better-
			// if it does, then add it into the queue to revisit later.
			if (final.rate >= compare.rate) { queue.put(trace); }
		} else { /* If we have been there, skip it. */ }
	} // End Graphloop
} // End workloop 

// Take end timestamp
const endDate = new Date();
const diff = endDate.getTime() - startDate.getTime();

// Sort the data ascending.
// Comparison function is the parameter
profitable.sort( (a,b) => { return a.rate - b.rate; } );

// Fast output building, we use an array.
// the Array.map() function takes the array, 
//		creates another array of the same length,
//		with the given transformation applied to each element.
// In this case, we have an array of printouts of the profitable paths.
const output = profitable.map(it => `Path: ${it.path}, $1,000 => $${(it.rate * 1000).toFixed(2)} `)

console.log("\n\nDONE!");
console.log("Took " + diff + "ms");	
// Also to output file
output[output.length] = "Took " + diff + "ms";
console.log("Found " + profitable.length + " profitable cycles!");
output[output.length] = "Found " + profitable.length + " profitable cycles!";

// Sort the data descending.
profitable.sort( (a,b) => { return b.rate - a.rate; } );

// Print the best one!
console.log("\n\nBest Overall arbitrage:");
console.log(profitable[0]);
// Also to output file
output[output.length] = "\n\nBest Overall arbitrage:"
// Have to manually stringify, console.log does this automatically. 
output[output.length] = JSON.stringify(profitable[0], null, '\t') 

// Sort the data descending by score
profitable.sort( (a,b) => { return b.score - a.score; } );

// Print the actually best one!
console.log("\n\nMost efficient arbitrage:");
console.log(profitable[0]);
// Also to output file
output[output.length] = "\n\nMost efficient arbitrage:"
// Have to manually stringify, console.log does this automatically. 
output[output.length] = JSON.stringify(profitable[0], null, '\t')

// Write output to file
fs.writeFileSync("./outputjs.txt", output.join("\n"))
