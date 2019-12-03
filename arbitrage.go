// I wrote this arbitrage solution in go, Saad was interested if go was faster than js.
// It is much faster. Completes everything in about a second, instead of minutes.
// Insanely fast. Yay.
// Note: golang tools autoformats stuff. Yeah.
package main

import (
	"fmt"
	"io/ioutil"
	"regexp"
	"sort"
	"strconv"
	"strings"
	"time"
)

// Helper for panicing if an error is present.
func check(e error) {
	if e != nil {
		panic(e)
	}
}

// helper for string[] contains, suprisingly there are no builtins for this.
func contains(arr []string, str string) bool {
	for _, a := range arr {
		if a == str {
			return true
		}
	}
	return false
}

// Stuct to store info about work to do
type workData struct {
	at    string   // Current location in graph
	start string   // Starting location in graph
	path  []string // All nodes that have been visited to reach the current node
	rate  float64  // Current exchange rate - 1.0 for break even, above for profit! below for losses >:^(
	score float64  // Current efficiency score, effectively %gain per trade. (rate-1.0)/(#ofTrades)
}

// typedef Interface for sorting workdata by rate
type byRate []workData

func (a byRate) Len() int           { return len(a) }
func (a byRate) Swap(i, j int)      { a[i], a[j] = a[j], a[i] }
func (a byRate) Less(i, j int) bool { return a[i].rate > a[j].rate }

// typedef Interface for sorting workdata by efficiency score
type byScore []workData

func (a byScore) Len() int           { return len(a) }
func (a byScore) Swap(i, j int)      { a[i], a[j] = a[j], a[i] }
func (a byScore) Less(i, j int) bool { return a[i].score > a[j].score }

// typedef for a single node in the graph- stores costs of transitions to other nodes */
type graphNode map[string]float64

// typedef for entire graph, just a map of string to graph nodes */
type graph map[string]graphNode

// Our graph, as a global variable
var gr graph //ics

// Takes a file and loads it into a graph
func loadFile(filename string) graph {
	fmt.Println("Loading file...")
	// Create a new graph
	gr := make(graph)
	// Read data from disk
	dat, err := ioutil.ReadFile(filename)
	check(err)

	// Turn data into a string
	str := string(dat)
	// fmt.Println(str)

	// Unfortunately, go's split behaves similar to javascripts and leaves garbage
	whitespace, err := regexp.Compile("\\s+")
	check(err)

	// Split file into lines
	lines := strings.Split(str, "\n")
	for i := 0; i < len(lines); i++ {
		line := lines[i]
		//fmt.Println(line)
		parts := whitespace.Split(line, -1)
		// fmt.Println(len(parts))

		// if len(parts) != 3 { // HEY- if this doesn't work, try toggling comments between these two lines
		if len(parts) != 4 { // HEY- if this doesn't work, try toggling comments between these two lines
			// Explanation: windows leaves an extra empty-string element at the end of the slice the slice
			// Saad didn't have this problem on his mac.
			// this is janky stuff that makes cross-platform code harder to achieve,
			// but it's not entirely  dealbreaker entirely for go
			continue
		}

		// VVV See what this line prints out to see if you need to change ^^^
		// fmt.Println(parts, len(parts))

		from := parts[0]                             // Starting currency
		to := parts[1]                               // Ending currency
		val, err := strconv.ParseFloat(parts[2], 64) // Conversion rate
		if err != nil {
			continue
		}

		// If we don't have nodes in the graph yet, make them
		if _, ok := gr[from]; !ok {
			gr[from] = make(graphNode)
		}
		if _, ok := gr[to]; !ok {
			gr[to] = make(graphNode)
		}

		// Then assign the value to the from -> to transition
		gr[from][to] = val
		// and the inverse to the to -> from transition
		gr[to][from] = 1.0 / val
	}
	// For the curious, print it out if you wish
	// fmt.Println(gr)
	fmt.Println("Done loading")
	return gr
}

// Steps a workData from where it is to some other node in the graph.
// Follow the transition, applies conversion rate, and creates a new workData
func stepTo(data workData, to string) workData {
	// If we're already there, just return current data.
	if data.at == to {
		return data
	}
	// Otherwise, create space for a new workData
	var next workData
	rate := gr[data.at][to] // (get rate from graph transition from where we are to destination)

	// And fill it out
	next.at = to            // Move to new node
	next.start = data.start // We still started at the new node
	// This bit's a bit trickier- we need to explicitly make a new []string
	next.path = make([]string, len(data.path)+1)
	// Copy the old path into the first bit, in js as easy as [...data.path]
	copy(next.path[0:], data.path[0:])
	// then we set the last thing in the []string to be the new transition
	next.path[len(next.path)-1] = to

	// Apply the rate
	next.rate = data.rate * rate
	// Calculate new efficiency score (rate - 1.0) / (numTrades)
	// This is more accurate, giving us % gain per trade
	next.score = (next.rate - 1.0) / float64(len(next.path)-1)

	// Return the assembled workData
	return next
}

// Another helper, steps the workData from wherever to where it started, normalizing the score.
func stepToStart(data workData) workData {
	return stepTo(data, data.start)
}

// Entry point.
func main() {

	// Load the graph from the file
	gr = loadFile("./stuff.dat")

	// Create our data structures:
	// area for a workQueue
	workQueue := make([]workData, 0)
	// area for finalized nodes (chains back to start)
	finished := make([]workData, 0)
	// and a nice []string of the keys in the graph.
	keys := make([]string, 0)

	// Maps are weird, and change the order of their keys for some reason.
	for k := range gr {
		// So we build a []string of all the keys in the graph
		keys = append(keys, k)
	}

	// Yes, we will start at all of the keys- this is fine.
	// go is just fast. Really fast. Really, really, really fast.
	//*
	for _, k := range keys {
		var it workData                        // space for workData
		it.start = k                           // start at k
		it.at = k                              // currently at k
		it.rate = 1.0                          // 1.0 starting rate
		it.path = append(make([]string, 0), k) // and a path of our starting node's name
		workQueue = append(workQueue, it)      // finally, append our workData to the workQueue
	}
	//*/

	// The following would work if you only wanted to do one starting point.
	// This would find the same 9-step chain, but
	/*firstKey := keys[0]
	var firstData workData
	firstData.start = firstKey
	firstData.at = firstKey
	firstData.rate = 1.0
	firstData.path = append(make([]string, 0), firstKey)
	workQueue = append(workQueue, firstData)
	//*/

	// fmt.Println("Starting queue:\n", workQueue)
	// Start time!
	fmt.Println("\n\nOkay ready set go-")
	start := time.Now()
	// go does not have while, only for.
	for len(workQueue) > 0 {
		// Read first item in queue
		data := workQueue[0]
		// Remove first thing from queue, this [a:b] range thing is called 'reslicing'.
		workQueue = workQueue[1:len(workQueue)]

		// Loop over keys, discard the index with _
		for _, k := range keys {
			// If we don't have the thing in our path yet...
			if !contains(data.path, k) {

				// Step to the new node
				next := stepTo(data, k)
				// normalize the score by stepping to start
				final := stepToStart(next)

				// If we made profit, add it to the finished list
				if final.rate > 1 {
					finished = append(finished, final)
				}
				// And append the next one to our work queue
				workQueue = append(workQueue, next)
			}
		}
	}
	// End time!
	end := time.Now()
	// Difference!
	diff := end.Sub(start)

	// Finally, done! Even prints out the difference nicely.
	fmt.Println("Done! took", diff, "!")
	fmt.Println("got", len(finished), "things")

	// Sort our stuff by total rate
	sort.Sort(byRate(finished))
	fmt.Println("\n\nBest overall is:") // Will be of length 9 (124.43% total, but only +2.7148% per trade...)
	printData(finished[0])
	// Sort our stuff by efficiency score
	sort.Sort(byScore(finished))
	fmt.Println("\n\nMost efficient is:") // Will Be NOR -> AUS -> YEN -> NOR cycle (only  112.575% total, but a whopping +4.191666% per trade!!!)
	printData(finished[0])

}

func printData(data workData) {
	fmt.Println("Path taken      : ", data.path)
	fmt.Println("Total Earnings  : ", data.rate)
	fmt.Println("Earned Per Trade: ", data.score)
}
