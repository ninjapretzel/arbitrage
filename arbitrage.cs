using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;

namespace Arbitrage {
	
	/// <summary> Class that holds information about a trace state in the graph </summary>
	class WorkData {
		/// <summary> Starting node </summary>
		public string start;
		/// <summary> Current node </summary>
		public string at;
		/// <summary> Path from starting node to current node </summary>
		public List<string> path;
		/// <summary> Current rate </summary>
		public double rate;
		/// <summary> Score (rate - 1.0) / NumTrades </summary>
		public double score;

		/// <inheritdoc />
		public override string ToString() { return $"Path: [{string.Join(" -> ", path)}]\nRate: {rate}\nScore: {score}"; }

		/// <summary> Creates a string for the output</summary>
		/// <returns> String with the path and resulting rate multiplied by 1,000 </returns>
		public string ShortString() { return $"Path: [ {string.Join(" -> ", path)} ] $1,000 => ${rate*1000.0:#,##0.00}"; }

	}
	/// <summary> Class to hold a graph. Names of every vertex in the graph will be a top-level key.
	/// Top level values are themselves maps describing possible transitions from that vertex. </summary>
	class Graph : Dictionary<string, Dictionary<string, double>> {

		/// <summary> Transition function through the graph </summary>
		/// <param name="data"> Current graph state to transition </param>
		/// <param name="place"> New vertex to transition to </param>
		/// <returns> Results of transition </returns>
		public WorkData Step(WorkData data, string place) {

			// If data is not already at the place we want to transition it to
			if (data.at != place) {
				WorkData next = new WorkData();
				// Started at the same place
				next.start = data.start;
				// Now at the new place
				next.at = place;
				// Apply the rate 
				next.rate = data.rate * this[data.at][place];
				// Score the node based on gain per trades
				next.score = (next.rate - 1.0) / data.path.Count;
				// Copy all elements from the previous path into the new one
				next.path = new List<string>(data.path);
				// Then add the place it is at now
				next.path.Add(place);

				// Return built node
				return next;
			}
			// Already there? Return what we were given
			return data;
		}

	}

	/// <summary> Class holding entry point and helpers </summary>
	class Program {

		/// <summary> Nice little helper that gets the path to the source directory. </summary>
		/// <param name="callerPath"> Autofilled by compiler due to <see cref="CallerFilePathAttribute"/> decoration. Full file path to sourcefile that calls the function. </param>
		/// <returns> The directory name of the source file that calls it. </returns>
		public static string SourceFileDirectory([CallerFilePath] string callerPath = "[NO PATH]") {
			return callerPath.Substring(0, callerPath.Replace('\\', '/').LastIndexOf('/'));
		}

		/// <summary> Entry point </summary>
		/// <param name="args"> Command line args</param>
		static void Main(string[] args) {

			// Read text from file 
			string text = File.ReadAllText(SourceFileDirectory() + "/stuff.dat");
			// Split into lines 
			string[] lines = text.Split('\n');

			// Create data structures
			Graph graph = new Graph();
			Queue<WorkData> queue = new Queue<WorkData>();
			List<WorkData> profitable = new List<WorkData>();

			// Loop over lines in file 
			foreach (string line in lines) {
				// Split each line, and remove empty strings
				string[] parts = line.Split(' ', '\t').Where(it => (it != "")).ToArray();

				// Skip lines that are not exactly 3 long
				if (parts.Length != 3) { continue; }
				// Extract data from line: [ from, to, rate ]
				string from = parts[0];
				string to = parts[1];
				double rate;
				// Skip lines that don't have a number in 3rd element
				if (!double.TryParse(parts[2], out rate)) { continue; }

				// Add new verticies if they don't already exist
				if (!graph.ContainsKey(from)) { 
					graph[from] = new Dictionary<string, double>();
				}
				if (!graph.ContainsKey(to)) {
					graph[to] = new Dictionary<string, double>();
				}

				// Set transition rate into graph
				graph[from][to] = rate;
				graph[to][from] = 1.0 / rate;
			}
			// Loop over key/value pairs in graph
			foreach (var pair in graph) {
				/* // To print out graph (if we want)
				Console.WriteLine(pair.Key + " : {");
				foreach (var pair2 in pair.Value) {
					Console.WriteLine("\t" + pair2.Key + " : " + pair2.Value);
				}
				Console.WriteLine("}");
				//*/

				// Create a new work data that starts at each vertex,
				WorkData data = new WorkData();
				data.start = pair.Key;
				data.at = pair.Key;
				data.rate = 1.0;
				data.score = 0.0;
				data.path = new List<string>();
				data.path.Add(pair.Key);

				// and add it to our queue
				queue.Enqueue(data);
			}
			Console.WriteLine("Ready. Running.");
			// Time how long it takes
			DateTime start = DateTime.UtcNow;
			// While queue has stuff in it...
			while (queue.Count > 0) {
				// Get front thing in queue
				WorkData data = queue.Dequeue();
				// Score this data to see how good it is
				WorkData compare = graph.Step(data, data.start);
				
				// Loop over graph:
				foreach (var pair in graph) {

					// If we haven't been to that vertex yet
					if (!data.path.Contains(pair.Key)) {
						// Take transition to that location 
						WorkData next = graph.Step(data, pair.Key);
						// Score the new data to see how good it is 
						WorkData final = graph.Step(next, data.start);

						// See if it is profitable, then add it to the list of profitable trades
						if (final.rate > 1) {
							profitable.Add(final);
						}

						// Ignore anywhere we lose money, even if it's still profitable
						if (final.rate >= compare.rate) {
							queue.Enqueue(next);
						}

					}

				}

			}
			// Finished! Take the time
			DateTime end = DateTime.UtcNow;
			TimeSpan diff = end - start;

			// Sort our data in Ascending order 
			profitable.Sort((a, b) => (a.rate - b.rate) < 0 ? -1 : 1 );
			// Effecient output building (lots of text to append, avoid using += )
			StringBuilder output = new StringBuilder();

			// loop over all profitable paths
			foreach (var profit in profitable) {
				// Console.WriteLine(profit.ShortString());
				output.Append(profit.ShortString());
				output.Append('\n');
			}
			
			Console.WriteLine("\n\nFinished in " + diff.TotalMilliseconds + "ms");
			output.Append("\n\nFinished in " + diff.TotalMilliseconds + "ms");

			// Sort our data in Decending order
			profitable.Sort((a, b) => (a.rate - b.rate) > 0 ? -1 : 1 );
			Console.WriteLine("\n\nBest Overall Arbitrage: \n" + profitable[0]);
			output.Append("\n\nBest Overall Arbitrage: \n" + profitable[0]);

			// Sort our data in Decending order by scores instead:
			profitable.Sort((a, b) => (a.score - b.score) > 0 ? -1 : 1 );
			Console.WriteLine("\n\nMost Efficient Arbitrage: \n" + profitable[0]);
			output.Append("\n\nMost Efficient Arbitrage: \n" + profitable[0]);

			// Dump output to file
			File.WriteAllText(SourceFileDirectory()+"/outputcs.txt", output.ToString());

			// Pause console
			Console.WriteLine("\nPress Enter to continue...");
			Console.Read();
		}
	}
}
