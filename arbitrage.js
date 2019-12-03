// Wrote this for some students to demonstrate how to step through the graphs
// File system libray
const fs = require("fs");

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
let graph = {}
// work queue, holds stuff we still need to do
let queue = []
// List of profitable finished paths.
// We don't need to care about unprofitable paths
// Unprofitable paths are just the reverse of profitable ones
var finished = []


// Loop over each line
for (let i = 0; i < lines.length; i++) {
	let line = lines[i];
	// Ignore comment lines ( actually doesn't matter tho... )
	if (line.indexOf("//") == 0) { 
		continue; 
	}
	
	// Split line on any whitespace
	let parts = line.split(/\s+/);
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

/* 
// Javascript is really slow and can't keep up with 9 of these for some reason.
Object.keys(graph).forEach((k) => {
	queue.push( { 
		start: k, // Remember where we start
		at: k, // current location is where we are starting initially
		rate: 1, // Start with a rate of 1.0
		path: [ k ],  // Remember everywhere we have been
	} );
})
//*/

//* 
// This is the a faster way to seed the queue
// This will find the same cycles regardless of starting point
// Can pick any point- 
// here I choose a random name of a currency within the graph
let first = Object.keys(graph)[Math.floor(Math.random() * Object.keys(graph).length)]

queue.push( {
	start: first, // Remember where we started
	at: first,  // We are at where we start
	rate: 1,  // start with a rate of 1.0
	path: [first], // Remember everywhere we have been
})
//*/

// Measure the score for a node in the same units it started in 
// Transition it back to the starting point and get a score
function stepToStart(data) {
	// Step a node back to starts
	return stepTo(data, data.start);
}
// transition a node to place in the graph
function stepTo(data, place) {
	// If this node is not already there:
	if (data.at !== place) {
		// Return the adjusted data
		return { // Object literal
			// Make the transition to the destination place 
			start: data.start, // Same starting location
			at: place,			// Now at that place 
			rate: data.rate * graph[data.at][place], // Apply rate change 
			path: [ ... data.path, place ] // update path with new place 
			// ... is the spread operator, it takes everything data.path,
			// and 'spreads' those values in order in the new array.
		}
	}
	// if we're already there, just return the data.
	return data;
}
// Let's time how long it takes
let startDate = new Date();
console.log("Ready. Running.");

// Continue while there are things in the queue
while (queue.length > 0) {
	// Pop a data off the queue
	let data = queue.pop();
	
	// Loop over all vertex names in our graph
	Object.keys(graph).forEach((k) => {
		// If we haven't been there yet...
		// (skips revisiting nodes, or transitioning to start)
		if (!data.path.includes(k)) {
			console.log(data.path + " does not have " + k);
			// Take transition to node k
			let trace = stepTo(data, k);
			// Step the transitioned node back to its start
			let final = stepToStart(trace);
			
			// Check that node's score, if it's profitable, add to list.
			if (final.rate > 1) {
				finished.push(final)
			}
			
			// Ad the transitioned node back to the queue.
			// unshift adds to opposite end of queue
			// so pop() doesn't remove it immediately
			queue.unshift(trace);
		} else {
			// If we have been there, skip it.
		}
	});
		
}
// Take end timestamp
let endDate = new Date();
let diff = endDate.getTime() - startDate.getTime();
	
console.log("\n\nDONE!");
console.log("Took " + diff + "ms");	

// Sort the data.
// Comparison function is the parameter
finished.sort( (a,b) => { return b.rate - a.rate; } );

// Print the best one!
console.log("\n\nBest arbitrage:");
console.log(finished[0]);
